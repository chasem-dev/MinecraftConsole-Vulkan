#include "4JLibs/inc/4J_Render.h"

#ifndef S_OK
#define S_OK ((HRESULT)0L)
#endif
#ifndef E_FAIL
#define E_FAIL ((HRESULT)0x80004005L)
#endif
#ifndef ERROR_SUCCESS
#define ERROR_SUCCESS 0L
#endif

#define STB_IMAGE_IMPLEMENTATION
#define STBI_ONLY_PNG
#define STBI_NO_STDIO
#include "Apple/stb_image.h"

#include "VulkanBootstrapApp.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <unordered_map>
#include <vector>

namespace
{
VulkanBootstrapApp g_vulkanBackend;

struct EngineVertex
{
  float x;
  float y;
  float z;
  float u;
  float v;
  uint32_t color;
  uint32_t normal;
  uint32_t extra;
};

using Matrix = std::array<float, 16>;

struct RenderStateTracker
{
  std::array<float, 4> colour {1.0f, 1.0f, 1.0f, 1.0f};
  bool blendEnabled = false;
  int blendSrc = 0;
  int blendDst = 0;
  float blendFactorAlpha = 1.0f;
  bool alphaTestEnabled = false;
  float alphaReference = 0.1f;
  bool depthTestEnabled = true;
  bool depthWriteEnabled = true;
  bool cullEnabled = false;
  bool cullClockwise = true;
  bool fogEnabled = false;
  int textureLevels = 1;
};

struct RecordedDrawCommand
{
  C4JRender::ePrimitiveType primitiveType = C4JRender::PRIMITIVE_TYPE_TRIANGLE_LIST;
  int count = 0;
  C4JRender::eVertexType vertexType = C4JRender::VERTEX_TYPE_PF3_TF2_CB4_NB4_XW1;
  C4JRender::ePixelShaderType pixelShaderType = C4JRender::PIXEL_SHADER_TYPE_STANDARD;
  std::vector<uint8_t> vertexData;
};

struct RecordedCommandBuffer
{
  std::vector<RecordedDrawCommand> draws;
  size_t bytes = 0;
};

RenderStateTracker g_renderState;
std::unordered_map<int, RecordedCommandBuffer> g_commandBuffers;
int g_nextCommandBufferIndex = 1;
int g_activeCommandBufferIndex = -1;
bool g_replayingCommandBuffer = false;
C4JRender &getRenderManager();

Matrix identityMatrix()
{
  return Matrix {
    1.0f, 0.0f, 0.0f, 0.0f,
    0.0f, 1.0f, 0.0f, 0.0f,
    0.0f, 0.0f, 1.0f, 0.0f,
    0.0f, 0.0f, 0.0f, 1.0f
  };
}

Matrix multiplyMatrix(const Matrix &lhs, const Matrix &rhs)
{
  Matrix result {};
  for (int column = 0; column < 4; ++column)
  {
    for (int row = 0; row < 4; ++row)
    {
      result[column * 4 + row] =
        lhs[0 * 4 + row] * rhs[column * 4 + 0] +
        lhs[1 * 4 + row] * rhs[column * 4 + 1] +
        lhs[2 * 4 + row] * rhs[column * 4 + 2] +
        lhs[3 * 4 + row] * rhs[column * 4 + 3];
    }
  }
  return result;
}

Matrix translationMatrix(float x, float y, float z)
{
  Matrix result = identityMatrix();
  result[12] = x;
  result[13] = y;
  result[14] = z;
  return result;
}

Matrix scaleMatrix(float x, float y, float z)
{
  Matrix result = identityMatrix();
  result[0] = x;
  result[5] = y;
  result[10] = z;
  return result;
}

Matrix rotationMatrix(float angleDegrees, float x, float y, float z)
{
  const float length = std::sqrt(x * x + y * y + z * z);
  if (length == 0.0f)
  {
    return identityMatrix();
  }

  x /= length;
  y /= length;
  z /= length;

  const float angleRadians = angleDegrees * 3.1415926535f / 180.0f;
  const float c = std::cos(angleRadians);
  const float s = std::sin(angleRadians);
  const float omc = 1.0f - c;

  Matrix result = identityMatrix();
  result[0] = x * x * omc + c;
  result[1] = y * x * omc + z * s;
  result[2] = x * z * omc - y * s;
  result[4] = x * y * omc - z * s;
  result[5] = y * y * omc + c;
  result[6] = y * z * omc + x * s;
  result[8] = x * z * omc + y * s;
  result[9] = y * z * omc - x * s;
  result[10] = z * z * omc + c;
  return result;
}

Matrix perspectiveMatrix(float fovyDegrees, float aspect, float zNear, float zFar)
{
  const float fovyRadians = fovyDegrees * 3.1415926535f / 180.0f;
  const float f = 1.0f / std::tan(fovyRadians * 0.5f);

  Matrix result {};
  result[0] = f / aspect;
  result[5] = f;
  result[10] = zFar / (zNear - zFar);
  result[11] = -1.0f;
  result[14] = (zFar * zNear) / (zNear - zFar);
  return result;
}

Matrix orthographicMatrix(float left, float right, float bottom, float top, float zNear, float zFar)
{
  Matrix result = identityMatrix();
  result[0] = 2.0f / (right - left);
  result[5] = 2.0f / (top - bottom);
  result[10] = -1.0f / (zFar - zNear);
  result[12] = -(right + left) / (right - left);
  result[13] = -(top + bottom) / (top - bottom);
  result[14] = -zNear / (zFar - zNear);
  return result;
}

VulkanBootstrapApp::Vertex convertVertex(const EngineVertex &vertex)
{
  VulkanBootstrapApp::Vertex result {};
  result.position[0] = vertex.x;
  result.position[1] = vertex.y;
  result.position[2] = vertex.z;
  result.texCoord[0] = vertex.u;
  result.texCoord[1] = vertex.v;

  const float sourceColor[4] = {
    static_cast<float>((vertex.color >> 24) & 0xff) / 255.0f,
    static_cast<float>((vertex.color >> 16) & 0xff) / 255.0f,
    static_cast<float>((vertex.color >> 8) & 0xff) / 255.0f,
    static_cast<float>(vertex.color & 0xff) / 255.0f
  };

  result.color[0] = sourceColor[0] * g_renderState.colour[0];
  result.color[1] = sourceColor[1] * g_renderState.colour[1];
  result.color[2] = sourceColor[2] * g_renderState.colour[2];
  result.color[3] = sourceColor[3] * g_renderState.colour[3];
  if (g_renderState.blendEnabled &&
      g_renderState.blendSrc == GL_CONSTANT_ALPHA &&
      g_renderState.blendDst == GL_ONE_MINUS_CONSTANT_ALPHA)
  {
    result.color[3] *= g_renderState.blendFactorAlpha;
  }

  return result;
}

VulkanBootstrapApp::BlendMode determineBlendMode()
{
  if (!g_renderState.blendEnabled)
  {
    return VulkanBootstrapApp::BlendMode::Opaque;
  }

  if (g_renderState.blendDst == GL_ONE)
  {
    return VulkanBootstrapApp::BlendMode::Additive;
  }

  return VulkanBootstrapApp::BlendMode::Alpha;
}

VulkanBootstrapApp::ShaderVariant determineShaderVariant(int textureIndex)
{
  if (textureIndex < 0)
  {
    return VulkanBootstrapApp::ShaderVariant::ColorOnly;
  }
  if (g_renderState.fogEnabled)
  {
    return VulkanBootstrapApp::ShaderVariant::TexturedFog;
  }
  if (g_renderState.alphaTestEnabled)
  {
    return VulkanBootstrapApp::ShaderVariant::TexturedAlphaTest;
  }
  return VulkanBootstrapApp::ShaderVariant::Textured;
}

size_t getVertexStride(C4JRender::eVertexType vertexType)
{
  switch (vertexType)
  {
  case C4JRender::VERTEX_TYPE_PF3_TF2_CB4_NB4_XW1:
  case C4JRender::VERTEX_TYPE_PF3_TF2_CB4_NB4_XW1_LIT:
  case C4JRender::VERTEX_TYPE_PF3_TF2_CB4_NB4_XW1_TEXGEN:
    return sizeof(EngineVertex);
  default:
    return 0;
  }
}

bool dispatchDrawVertices(
  C4JRender::ePrimitiveType primitiveType,
  int count,
  void *dataIn,
  C4JRender::eVertexType vertexType,
  C4JRender::ePixelShaderType pixelShaderType)
{
  if (dataIn == nullptr || count <= 0)
  {
    return false;
  }
  if (vertexType != C4JRender::VERTEX_TYPE_PF3_TF2_CB4_NB4_XW1 &&
      vertexType != C4JRender::VERTEX_TYPE_PF3_TF2_CB4_NB4_XW1_LIT &&
      vertexType != C4JRender::VERTEX_TYPE_PF3_TF2_CB4_NB4_XW1_TEXGEN)
  {
    return false;
  }

  (void)pixelShaderType;

  const EngineVertex *vertices = static_cast<const EngineVertex *>(dataIn);
  Matrix projection {};
  Matrix modelView {};
  std::memcpy(projection.data(), getRenderManager().MatrixGet(GL_PROJECTION), sizeof(float) * projection.size());
  std::memcpy(modelView.data(), getRenderManager().MatrixGet(GL_MODELVIEW), sizeof(float) * modelView.size());
  const Matrix mvp = multiplyMatrix(projection, modelView);

  std::vector<VulkanBootstrapApp::Vertex> converted;
  converted.reserve(static_cast<size_t>(count) * 2);

  if (primitiveType == C4JRender::PRIMITIVE_TYPE_QUAD_LIST)
  {
    for (int index = 0; index + 3 < count; index += 4)
    {
      const VulkanBootstrapApp::Vertex v0 = convertVertex(vertices[index + 0]);
      const VulkanBootstrapApp::Vertex v1 = convertVertex(vertices[index + 1]);
      const VulkanBootstrapApp::Vertex v2 = convertVertex(vertices[index + 2]);
      const VulkanBootstrapApp::Vertex v3 = convertVertex(vertices[index + 3]);
      converted.push_back(v0);
      converted.push_back(v1);
      converted.push_back(v2);
      converted.push_back(v0);
      converted.push_back(v2);
      converted.push_back(v3);
    }
  }
  else if (primitiveType == C4JRender::PRIMITIVE_TYPE_TRIANGLE_LIST)
  {
    for (int index = 0; index < count; ++index)
    {
      converted.push_back(convertVertex(vertices[index]));
    }
  }
  else if (primitiveType == C4JRender::PRIMITIVE_TYPE_TRIANGLE_STRIP)
  {
    for (int index = 0; index + 2 < count; ++index)
    {
      const VulkanBootstrapApp::Vertex a = convertVertex(vertices[index + 0]);
      const VulkanBootstrapApp::Vertex b = convertVertex(vertices[index + 1]);
      const VulkanBootstrapApp::Vertex c = convertVertex(vertices[index + 2]);
      if ((index & 1) == 0)
      {
        converted.push_back(a);
        converted.push_back(b);
        converted.push_back(c);
      }
      else
      {
        converted.push_back(b);
        converted.push_back(a);
        converted.push_back(c);
      }
    }
  }
  else if (primitiveType == C4JRender::PRIMITIVE_TYPE_TRIANGLE_FAN)
  {
    const VulkanBootstrapApp::Vertex origin = convertVertex(vertices[0]);
    for (int index = 1; index + 1 < count; ++index)
    {
      converted.push_back(origin);
      converted.push_back(convertVertex(vertices[index]));
      converted.push_back(convertVertex(vertices[index + 1]));
    }
  }
  else
  {
    return false;
  }

  const int currentTexture = g_vulkanBackend.getCurrentTexture();
  VulkanBootstrapApp::RenderState renderState {};
  renderState.blendMode = determineBlendMode();
  renderState.depthTestEnabled = g_renderState.depthTestEnabled;
  renderState.depthWriteEnabled = g_renderState.depthTestEnabled && g_renderState.depthWriteEnabled;
  renderState.cullEnabled = g_renderState.cullEnabled;
  renderState.cullClockwise = g_renderState.cullClockwise;
  renderState.textureIndex = currentTexture;

  const VulkanBootstrapApp::ShaderVariant variant = determineShaderVariant(currentTexture);
  g_vulkanBackend.submitVertices(
    VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
    converted.data(),
    converted.size(),
    variant,
    mvp.data(),
    renderState);
  return !converted.empty();
}

bool decodeTextureBytes(
  const unsigned char *buffer,
  int length,
  D3DXIMAGE_INFO *imageInfo,
  int **pixelDataOut,
  const char *debugName)
{
  if (buffer == nullptr || length <= 0 || imageInfo == nullptr || pixelDataOut == nullptr)
  {
    return false;
  }

  int width = 0;
  int height = 0;
  int channels = 0;
  unsigned char *pixels = stbi_load_from_memory(buffer, length, &width, &height, &channels, 4);
  if (pixels == nullptr)
  {
#ifdef _DEBUG
    std::fprintf(
      stderr,
      "[mce_vulkan_boot] stb_image failed for %s: %s\n",
      debugName != nullptr ? debugName : "<memory>",
      stbi_failure_reason());
#endif
    return false;
  }

  int *argbPixels = new int[width * height];
  for (int index = 0; index < width * height; ++index)
  {
    const int r = pixels[index * 4 + 0];
    const int g = pixels[index * 4 + 1];
    const int b = pixels[index * 4 + 2];
    const int a = pixels[index * 4 + 3];
    argbPixels[index] = (a << 24) | (r << 16) | (g << 8) | b;
  }

  stbi_image_free(pixels);
  imageInfo->Width = width;
  imageInfo->Height = height;
  *pixelDataOut = argbPixels;

#ifdef _DEBUG
  std::fprintf(
    stderr,
    "[mce_vulkan_boot] Decoded texture %s (%dx%d)\n",
    debugName != nullptr ? debugName : "<memory>",
    width,
    height);
#endif

  return true;
}
}

C4JRender RenderManager;

namespace
{
C4JRender &getRenderManager()
{
  return RenderManager;
}
}

void C4JRender::Initialise(ID3D11Device *, IDXGISwapChain *)
{
}

void C4JRender::InitialiseForWindow(GLFWwindow *window)
{
  window_ = window;
  const Matrix identity = identityMatrix();
  modelViewStack_.assign(1, identity);
  projectionStack_.assign(1, identity);
  textureStack_.assign(1, identity);
  currentMatrixMode_ = GL_MODELVIEW;
  std::copy(identity.begin(), identity.end(), identityMatrix_);

  g_renderState = RenderStateTracker {};
  g_vulkanBackend.attachToWindow(window_);
  g_vulkanBackend.setClearColour(clearColour_);
}

void C4JRender::Shutdown()
{
  g_vulkanBackend.shutdownRenderer();
  window_ = nullptr;
}

void C4JRender::Tick() {}
void C4JRender::UpdateGamma(unsigned short) {}
void C4JRender::MatrixMode(int type) { currentMatrixMode_ = type; }

void C4JRender::MatrixSetIdentity()
{
  const Matrix identity = identityMatrix();
  if (currentMatrixMode_ == GL_PROJECTION)
  {
    projectionStack_.back() = identity;
  }
  else if (currentMatrixMode_ == GL_TEXTURE)
  {
    textureStack_.back() = identity;
  }
  else
  {
    modelViewStack_.back() = identity;
  }
}

void C4JRender::MatrixTranslate(float x, float y, float z)
{
  const Matrix matrix = translationMatrix(x, y, z);
  if (currentMatrixMode_ == GL_PROJECTION)
  {
    projectionStack_.back() = multiplyMatrix(projectionStack_.back(), matrix);
  }
  else if (currentMatrixMode_ == GL_TEXTURE)
  {
    textureStack_.back() = multiplyMatrix(textureStack_.back(), matrix);
  }
  else
  {
    modelViewStack_.back() = multiplyMatrix(modelViewStack_.back(), matrix);
  }
}

void C4JRender::MatrixRotate(float angle, float x, float y, float z)
{
  const Matrix matrix = rotationMatrix(angle, x, y, z);
  if (currentMatrixMode_ == GL_PROJECTION)
  {
    projectionStack_.back() = multiplyMatrix(projectionStack_.back(), matrix);
  }
  else if (currentMatrixMode_ == GL_TEXTURE)
  {
    textureStack_.back() = multiplyMatrix(textureStack_.back(), matrix);
  }
  else
  {
    modelViewStack_.back() = multiplyMatrix(modelViewStack_.back(), matrix);
  }
}

void C4JRender::MatrixScale(float x, float y, float z)
{
  const Matrix matrix = scaleMatrix(x, y, z);
  if (currentMatrixMode_ == GL_PROJECTION)
  {
    projectionStack_.back() = multiplyMatrix(projectionStack_.back(), matrix);
  }
  else if (currentMatrixMode_ == GL_TEXTURE)
  {
    textureStack_.back() = multiplyMatrix(textureStack_.back(), matrix);
  }
  else
  {
    modelViewStack_.back() = multiplyMatrix(modelViewStack_.back(), matrix);
  }
}

void C4JRender::MatrixPerspective(float fovy, float aspect, float zNear, float zFar)
{
  const Matrix matrix = perspectiveMatrix(fovy, aspect, zNear, zFar);
  if (currentMatrixMode_ == GL_PROJECTION)
  {
    projectionStack_.back() = multiplyMatrix(projectionStack_.back(), matrix);
  }
  else
  {
    modelViewStack_.back() = multiplyMatrix(modelViewStack_.back(), matrix);
  }
}

void C4JRender::MatrixOrthogonal(float left, float right, float bottom, float top, float zNear, float zFar)
{
  const Matrix matrix = orthographicMatrix(left, right, bottom, top, zNear, zFar);
  if (currentMatrixMode_ == GL_PROJECTION)
  {
    projectionStack_.back() = multiplyMatrix(projectionStack_.back(), matrix);
  }
  else
  {
    modelViewStack_.back() = multiplyMatrix(modelViewStack_.back(), matrix);
  }
}

void C4JRender::MatrixPop()
{
  auto *stack = &modelViewStack_;
  if (currentMatrixMode_ == GL_PROJECTION)
  {
    stack = &projectionStack_;
  }
  else if (currentMatrixMode_ == GL_TEXTURE)
  {
    stack = &textureStack_;
  }
  if (stack->size() > 1)
  {
    stack->pop_back();
  }
}

void C4JRender::MatrixPush()
{
  auto *stack = &modelViewStack_;
  if (currentMatrixMode_ == GL_PROJECTION)
  {
    stack = &projectionStack_;
  }
  else if (currentMatrixMode_ == GL_TEXTURE)
  {
    stack = &textureStack_;
  }
  stack->push_back(stack->back());
}

void C4JRender::MatrixMult(float *mat)
{
  Matrix matrix {};
  std::memcpy(matrix.data(), mat, sizeof(float) * 16);
  if (currentMatrixMode_ == GL_PROJECTION)
  {
    projectionStack_.back() = multiplyMatrix(projectionStack_.back(), matrix);
  }
  else if (currentMatrixMode_ == GL_TEXTURE)
  {
    textureStack_.back() = multiplyMatrix(textureStack_.back(), matrix);
  }
  else
  {
    modelViewStack_.back() = multiplyMatrix(modelViewStack_.back(), matrix);
  }
}

const float *C4JRender::MatrixGet(int type)
{
  if (type == GL_PROJECTION_MATRIX || type == GL_PROJECTION)
  {
    return projectionStack_.back().data();
  }
  if (type == GL_TEXTURE)
  {
    return textureStack_.back().data();
  }
  return modelViewStack_.back().data();
}

void C4JRender::Set_matrixDirty() {}
void C4JRender::InitialiseContext() {}
void C4JRender::StartFrame() { g_vulkanBackend.beginFrame(); }
void C4JRender::DoScreenGrabOnNextPresent() {}
void C4JRender::Present() { g_vulkanBackend.tickFrame(); }
void C4JRender::Clear(int, D3D11_RECT *) {}

void C4JRender::SetClearColour(const float colourRGBA[4])
{
  std::copy(colourRGBA, colourRGBA + 4, clearColour_);
  g_vulkanBackend.setClearColour(clearColour_);
}

bool C4JRender::IsWidescreen() { return true; }
bool C4JRender::IsHiDef() { return true; }
void C4JRender::CaptureThumbnail(ImageFileBuffer *) {}
void C4JRender::CaptureScreen(ImageFileBuffer *, XSOCIAL_PREVIEWIMAGE *) {}
void C4JRender::BeginConditionalSurvey(int) {}
void C4JRender::EndConditionalSurvey() {}
void C4JRender::BeginConditionalRendering(int) {}
void C4JRender::EndConditionalRendering() {}

void C4JRender::DrawVertices(
  ePrimitiveType primitiveType,
  int count,
  void *dataIn,
  eVertexType vType,
  ePixelShaderType psType)
{
  if (g_activeCommandBufferIndex >= 0 && !g_replayingCommandBuffer)
  {
    const size_t stride = getVertexStride(vType);
    if (stride == 0 || dataIn == nullptr || count <= 0)
    {
      return;
    }

    RecordedDrawCommand command {};
    command.primitiveType = primitiveType;
    command.count = count;
    command.vertexType = vType;
    command.pixelShaderType = psType;
    command.vertexData.resize(stride * static_cast<size_t>(count));
    std::memcpy(command.vertexData.data(), dataIn, command.vertexData.size());

    RecordedCommandBuffer &buffer = g_commandBuffers[g_activeCommandBufferIndex];
    buffer.draws.push_back(std::move(command));
    buffer.bytes += buffer.draws.back().vertexData.size();
    return;
  }
  dispatchDrawVertices(primitiveType, count, dataIn, vType, psType);
}

void C4JRender::DrawVertexBuffer(ePrimitiveType, int, ID3D11Buffer *, eVertexType, ePixelShaderType) {}
void C4JRender::CBuffLockStaticCreations() {}
int C4JRender::CBuffCreate(int count)
{
  if (count <= 0)
  {
    return 0;
  }

  const int firstIndex = g_nextCommandBufferIndex;
  for (int offset = 0; offset < count; ++offset)
  {
    g_commandBuffers[firstIndex + offset] = RecordedCommandBuffer {};
  }
  g_nextCommandBufferIndex += count;
  return firstIndex;
}

void C4JRender::CBuffDelete(int first, int count)
{
  for (int offset = 0; offset < count; ++offset)
  {
    g_commandBuffers.erase(first + offset);
  }
  if (g_activeCommandBufferIndex >= first && g_activeCommandBufferIndex < first + count)
  {
    g_activeCommandBufferIndex = -1;
  }
}

void C4JRender::CBuffStart(int index, bool)
{
  RecordedCommandBuffer &buffer = g_commandBuffers[index];
  buffer.draws.clear();
  buffer.bytes = 0;
  g_activeCommandBufferIndex = index;
}

void C4JRender::CBuffClear(int index)
{
  auto it = g_commandBuffers.find(index);
  if (it == g_commandBuffers.end())
  {
    return;
  }

  it->second.draws.clear();
  it->second.bytes = 0;
}

int C4JRender::CBuffSize(int index)
{
  if (index < 0)
  {
    size_t totalBytes = 0;
    for (const auto &entry : g_commandBuffers)
    {
      totalBytes += entry.second.bytes;
    }
    return static_cast<int>(totalBytes);
  }

  const auto it = g_commandBuffers.find(index);
  if (it == g_commandBuffers.end())
  {
    return 0;
  }

  return static_cast<int>(it->second.bytes);
}

void C4JRender::CBuffEnd()
{
  g_activeCommandBufferIndex = -1;
}

bool C4JRender::CBuffCall(int index, bool)
{
  const auto it = g_commandBuffers.find(index);
  if (it == g_commandBuffers.end() || it->second.draws.empty())
  {
    return false;
  }

  g_replayingCommandBuffer = true;
  for (const RecordedDrawCommand &command : it->second.draws)
  {
    dispatchDrawVertices(
      command.primitiveType,
      command.count,
      const_cast<uint8_t *>(command.vertexData.data()),
      command.vertexType,
      command.pixelShaderType);
  }
  g_replayingCommandBuffer = false;
  return true;
}
void C4JRender::CBuffTick() {}
void C4JRender::CBuffDeferredModeStart() {}
void C4JRender::CBuffDeferredModeEnd() {}

int C4JRender::TextureCreate()
{
  return g_vulkanBackend.allocateTextureSlot();
}

void C4JRender::TextureFree(int idx)
{
  g_vulkanBackend.freeTextureSlot(idx);
}

void C4JRender::TextureBind(int idx)
{
  g_vulkanBackend.setCurrentTexture(idx);
}

void C4JRender::TextureBindVertex(int) {}

void C4JRender::TextureSetTextureLevels(int levels)
{
  g_renderState.textureLevels = std::max(levels, 1);
}

int C4JRender::TextureGetTextureLevels()
{
  return g_renderState.textureLevels;
}

void C4JRender::TextureData(int width, int height, void *data, int level, eTextureFormat)
{
  if (level != 0 || data == nullptr)
  {
    return;
  }

  const int currentTexture = g_vulkanBackend.getCurrentTexture();
  if (currentTexture < 0)
  {
    return;
  }

  g_vulkanBackend.uploadTextureData(
    currentTexture,
    static_cast<uint32_t>(width),
    static_cast<uint32_t>(height),
    data);
}

void C4JRender::TextureDataUpdate(int xoffset, int yoffset, int width, int height, void *data, int level)
{
  if (level != 0 || data == nullptr)
  {
    return;
  }

  const int currentTexture = g_vulkanBackend.getCurrentTexture();
  if (currentTexture < 0)
  {
    return;
  }

  g_vulkanBackend.updateTextureData(
    currentTexture,
    xoffset,
    yoffset,
    static_cast<uint32_t>(width),
    static_cast<uint32_t>(height),
    data);
}

void C4JRender::TextureSetParam(int param, int value)
{
  const int currentTexture = g_vulkanBackend.getCurrentTexture();
  if (currentTexture < 0)
  {
    return;
  }

  switch (param)
  {
  case GL_TEXTURE_MIN_FILTER:
  case GL_TEXTURE_MAG_FILTER:
    if (value == GL_LINEAR)
    {
      g_vulkanBackend.setTextureLinearFiltering(currentTexture, true);
    }
    else if (value == GL_NEAREST || value == GL_NEAREST_MIPMAP_LINEAR)
    {
      g_vulkanBackend.setTextureLinearFiltering(currentTexture, false);
    }
    break;
  case GL_TEXTURE_WRAP_S:
  case GL_TEXTURE_WRAP_T:
    if (value == GL_REPEAT)
    {
      g_vulkanBackend.setTextureClampAddress(currentTexture, false);
    }
    else
    {
      g_vulkanBackend.setTextureClampAddress(currentTexture, true);
    }
    break;
  default:
    break;
  }
}

void C4JRender::TextureDynamicUpdateStart() {}
void C4JRender::TextureDynamicUpdateEnd() {}

HRESULT C4JRender::LoadTextureData(const char *szFilename, D3DXIMAGE_INFO *pSrcInfo, int **ppDataOut)
{
  if (szFilename == nullptr)
  {
    return E_FAIL;
  }

  FILE *file = std::fopen(szFilename, "rb");
  if (file == nullptr)
  {
#ifdef _DEBUG
    std::fprintf(stderr, "[mce_vulkan_boot] Failed to open texture: %s\n", szFilename);
#endif
    return E_FAIL;
  }

  std::fseek(file, 0, SEEK_END);
  const long fileSize = std::ftell(file);
  std::fseek(file, 0, SEEK_SET);
  if (fileSize <= 0)
  {
    std::fclose(file);
    return E_FAIL;
  }

  std::vector<unsigned char> buffer(static_cast<size_t>(fileSize));
  const size_t bytesRead = std::fread(buffer.data(), 1, buffer.size(), file);
  std::fclose(file);
  if (bytesRead != buffer.size())
  {
    return E_FAIL;
  }

  return decodeTextureBytes(
           buffer.data(),
           static_cast<int>(buffer.size()),
           pSrcInfo,
           ppDataOut,
           szFilename)
    ? ERROR_SUCCESS
    : E_FAIL;
}

HRESULT C4JRender::LoadTextureData(BYTE *pbData, DWORD dwBytes, D3DXIMAGE_INFO *pSrcInfo, int **ppDataOut)
{
  return decodeTextureBytes(
           pbData,
           static_cast<int>(dwBytes),
           pSrcInfo,
           ppDataOut,
           "<memory>")
    ? ERROR_SUCCESS
    : E_FAIL;
}

HRESULT C4JRender::SaveTextureData(const char *, D3DXIMAGE_INFO *, int *) { return E_FAIL; }
HRESULT C4JRender::SaveTextureDataToMemory(void *, int, int *, int, int, int *) { return E_FAIL; }
void C4JRender::TextureGetStats() {}
ID3D11ShaderResourceView *C4JRender::TextureGetTexture(int) { return nullptr; }

void C4JRender::StateSetColour(float r, float g, float b, float a)
{
  g_renderState.colour = {r, g, b, a};
}

void C4JRender::StateSetDepthMask(bool enable)
{
  g_renderState.depthWriteEnabled = enable;
}

void C4JRender::StateSetBlendEnable(bool enable)
{
  g_renderState.blendEnabled = enable;
}

void C4JRender::StateSetBlendFunc(int src, int dst)
{
  g_renderState.blendSrc = src;
  g_renderState.blendDst = dst;
}

void C4JRender::StateSetBlendFactor(unsigned int colour)
{
  g_renderState.blendFactorAlpha = static_cast<float>((colour >> 24) & 0xff) / 255.0f;
}

void C4JRender::StateSetAlphaFunc(int, float param)
{
  g_renderState.alphaReference = param;
}

void C4JRender::StateSetDepthFunc(int) {}

void C4JRender::StateSetFaceCull(bool enable)
{
  g_renderState.cullEnabled = enable;
}

void C4JRender::StateSetFaceCullCW(bool enable)
{
  g_renderState.cullClockwise = enable;
}

void C4JRender::StateSetLineWidth(float) {}
void C4JRender::StateSetWriteEnable(bool, bool, bool, bool) {}

void C4JRender::StateSetDepthTestEnable(bool enable)
{
  g_renderState.depthTestEnabled = enable;
}

void C4JRender::StateSetAlphaTestEnable(bool enable)
{
  g_renderState.alphaTestEnabled = enable;
}

void C4JRender::StateSetDepthSlopeAndBias(float, float) {}

void C4JRender::StateSetFogEnable(bool enable)
{
  g_renderState.fogEnabled = enable;
}

void C4JRender::StateSetFogMode(int) {}
void C4JRender::StateSetFogNearDistance(float) {}
void C4JRender::StateSetFogFarDistance(float) {}
void C4JRender::StateSetFogDensity(float) {}
void C4JRender::StateSetFogColour(float, float, float) {}
void C4JRender::StateSetLightingEnable(bool) {}
void C4JRender::StateSetVertexTextureUV(float, float) {}
void C4JRender::StateSetLightColour(int, float, float, float) {}
void C4JRender::StateSetLightAmbientColour(float, float, float) {}
void C4JRender::StateSetLightDirection(int, float, float, float) {}
void C4JRender::StateSetLightEnable(int, bool) {}
void C4JRender::StateSetViewport(eViewportType) {}
void C4JRender::StateSetEnableViewportClipPlanes(bool) {}
void C4JRender::StateSetTexGenCol(int, float, float, float, float, bool) {}
void C4JRender::StateSetStencil(int, uint8_t, uint8_t, uint8_t) {}
void C4JRender::StateSetForceLOD(int) {}
void C4JRender::BeginEvent(const wchar_t *) {}
void C4JRender::EndEvent() {}
void C4JRender::Suspend() { suspended_ = true; }
bool C4JRender::Suspended() { return suspended_; }
void C4JRender::Resume() { suspended_ = false; }
