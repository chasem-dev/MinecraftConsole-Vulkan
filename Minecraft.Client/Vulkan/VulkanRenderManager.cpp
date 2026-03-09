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
#include <mutex>
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

struct EngineVertexCompressed
{
  int16_t x;
  int16_t y;
  int16_t z;
  uint16_t color565;
  int16_t u;
  int16_t v;
  int16_t tex2u;
  int16_t tex2v;
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

struct ThreadRenderContext
{
  RenderStateTracker renderState {};
  int currentMatrixMode = GL_MODELVIEW;
  std::vector<Matrix> modelViewStack;
  std::vector<Matrix> projectionStack;
  std::vector<Matrix> textureStack;
  int currentTextureIndex = -1;
};

struct RecordedDrawCommand
{
  VulkanBootstrapApp::ShaderVariant shaderVariant = VulkanBootstrapApp::ShaderVariant::ColorOnly;
  VulkanBootstrapApp::RenderState renderState {};
  Matrix localModelView {};
  std::vector<VulkanBootstrapApp::Vertex> vertices;
};

struct RecordedCommandBuffer
{
  std::vector<RecordedDrawCommand> draws;
  Matrix baseModelView {};
  size_t bytes = 0;
};

thread_local ThreadRenderContext g_threadContext;
std::unordered_map<int, RecordedCommandBuffer> g_commandBuffers;
std::mutex g_commandBufferMutex;
int g_nextCommandBufferIndex = 1;
thread_local int g_activeCommandBufferIndex = -1;
thread_local bool g_replayingCommandBuffer = false;
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

ThreadRenderContext &getThreadContext()
{
  if (g_threadContext.modelViewStack.empty())
  {
    const Matrix identity = identityMatrix();
    g_threadContext.modelViewStack.assign(1, identity);
    g_threadContext.projectionStack.assign(1, identity);
    g_threadContext.textureStack.assign(1, identity);
    g_threadContext.currentMatrixMode = GL_MODELVIEW;
    g_threadContext.currentTextureIndex = -1;
    g_threadContext.renderState = RenderStateTracker {};
  }
  return g_threadContext;
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

Matrix invertMatrix(const Matrix &m)
{
  Matrix inv {};

  inv[0] = m[5]  * m[10] * m[15] -
           m[5]  * m[11] * m[14] -
           m[9]  * m[6]  * m[15] +
           m[9]  * m[7]  * m[14] +
           m[13] * m[6]  * m[11] -
           m[13] * m[7]  * m[10];

  inv[4] = -m[4]  * m[10] * m[15] +
            m[4]  * m[11] * m[14] +
            m[8]  * m[6]  * m[15] -
            m[8]  * m[7]  * m[14] -
            m[12] * m[6]  * m[11] +
            m[12] * m[7]  * m[10];

  inv[8] = m[4]  * m[9] * m[15] -
           m[4]  * m[11] * m[13] -
           m[8]  * m[5] * m[15] +
           m[8]  * m[7] * m[13] +
           m[12] * m[5] * m[11] -
           m[12] * m[7] * m[9];

  inv[12] = -m[4]  * m[9] * m[14] +
             m[4]  * m[10] * m[13] +
             m[8]  * m[5] * m[14] -
             m[8]  * m[6] * m[13] -
             m[12] * m[5] * m[10] +
             m[12] * m[6] * m[9];

  inv[1] = -m[1]  * m[10] * m[15] +
            m[1]  * m[11] * m[14] +
            m[9]  * m[2] * m[15] -
            m[9]  * m[3] * m[14] -
            m[13] * m[2] * m[11] +
            m[13] * m[3] * m[10];

  inv[5] = m[0]  * m[10] * m[15] -
           m[0]  * m[11] * m[14] -
           m[8]  * m[2] * m[15] +
           m[8]  * m[3] * m[14] +
           m[12] * m[2] * m[11] -
           m[12] * m[3] * m[10];

  inv[9] = -m[0]  * m[9] * m[15] +
            m[0]  * m[11] * m[13] +
            m[8]  * m[1] * m[15] -
            m[8]  * m[3] * m[13] -
            m[12] * m[1] * m[11] +
            m[12] * m[3] * m[9];

  inv[13] = m[0]  * m[9] * m[14] -
            m[0]  * m[10] * m[13] -
            m[8]  * m[1] * m[14] +
            m[8]  * m[2] * m[13] +
            m[12] * m[1] * m[10] -
            m[12] * m[2] * m[9];

  inv[2] = m[1]  * m[6] * m[15] -
           m[1]  * m[7] * m[14] -
           m[5]  * m[2] * m[15] +
           m[5]  * m[3] * m[14] +
           m[13] * m[2] * m[7] -
           m[13] * m[3] * m[6];

  inv[6] = -m[0]  * m[6] * m[15] +
            m[0]  * m[7] * m[14] +
            m[4]  * m[2] * m[15] -
            m[4]  * m[3] * m[14] -
            m[12] * m[2] * m[7] +
            m[12] * m[3] * m[6];

  inv[10] = m[0]  * m[5] * m[15] -
            m[0]  * m[7] * m[13] -
            m[4]  * m[1] * m[15] +
            m[4]  * m[3] * m[13] +
            m[12] * m[1] * m[7] -
            m[12] * m[3] * m[5];

  inv[14] = -m[0]  * m[5] * m[14] +
             m[0]  * m[6] * m[13] +
             m[4]  * m[1] * m[14] -
             m[4]  * m[2] * m[13] -
             m[12] * m[1] * m[6] +
             m[12] * m[2] * m[5];

  inv[3] = -m[1] * m[6] * m[11] +
            m[1] * m[7] * m[10] +
            m[5] * m[2] * m[11] -
            m[5] * m[3] * m[10] -
            m[9] * m[2] * m[7] +
            m[9] * m[3] * m[6];

  inv[7] = m[0] * m[6] * m[11] -
           m[0] * m[7] * m[10] -
           m[4] * m[2] * m[11] +
           m[4] * m[3] * m[10] +
           m[8] * m[2] * m[7] -
           m[8] * m[3] * m[6];

  inv[11] = -m[0] * m[5] * m[11] +
             m[0] * m[7] * m[9] +
             m[4] * m[1] * m[11] -
             m[4] * m[3] * m[9] -
             m[8] * m[1] * m[7] +
             m[8] * m[3] * m[5];

  inv[15] = m[0] * m[5] * m[10] -
            m[0] * m[6] * m[9] -
            m[4] * m[1] * m[10] +
            m[4] * m[2] * m[9] +
            m[8] * m[1] * m[6] -
            m[8] * m[2] * m[5];

  float det = m[0] * inv[0] + m[1] * inv[4] + m[2] * inv[8] + m[3] * inv[12];
  if (std::fabs(det) < 1.0e-8f)
  {
    return identityMatrix();
  }

  det = 1.0f / det;
  for (float &value : inv)
  {
    value *= det;
  }
  return inv;
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

float normalizePrimaryTexCoord(float u)
{
  // Legacy 4J code signals "disable mipmapping for this vertex" by pushing the
  // primary U coordinate into the [1, 2) range. The Vulkan path currently only
  // uploads the base level, so we should keep the original UV rather than
  // sampling from the wrong atlas page.
  return u > 1.0f ? (u - 1.0f) : u;
}

VulkanBootstrapApp::Vertex convertVertex(const EngineVertex &vertex)
{
  VulkanBootstrapApp::Vertex result {};
  result.position[0] = vertex.x;
  result.position[1] = vertex.y;
  result.position[2] = vertex.z;
  result.texCoord[0] = normalizePrimaryTexCoord(vertex.u);
  result.texCoord[1] = vertex.v;

  const float sourceColor[4] = {
    static_cast<float>((vertex.color >> 16) & 0xff) / 255.0f,
    static_cast<float>((vertex.color >> 8) & 0xff) / 255.0f,
    static_cast<float>(vertex.color & 0xff) / 255.0f,
    static_cast<float>((vertex.color >> 24) & 0xff) / 255.0f
  };

  ThreadRenderContext &context = getThreadContext();
  result.color[0] = sourceColor[0] * context.renderState.colour[0];
  result.color[1] = sourceColor[1] * context.renderState.colour[1];
  result.color[2] = sourceColor[2] * context.renderState.colour[2];
  result.color[3] = sourceColor[3] * context.renderState.colour[3];
  if (context.renderState.blendEnabled &&
      context.renderState.blendSrc == GL_CONSTANT_ALPHA &&
      context.renderState.blendDst == GL_ONE_MINUS_CONSTANT_ALPHA)
  {
    result.color[3] *= context.renderState.blendFactorAlpha;
  }

  return result;
}

VulkanBootstrapApp::Vertex convertCompressedVertex(const EngineVertexCompressed &vertex)
{
  VulkanBootstrapApp::Vertex result {};
  result.position[0] = static_cast<float>(vertex.x) / 1024.0f;
  result.position[1] = static_cast<float>(vertex.y) / 1024.0f;
  result.position[2] = static_cast<float>(vertex.z) / 1024.0f;
  // The Tesselator encodes UVs as (int)(u * 8192.0f) & 0xffff into int16_t slots.
  // For mipmap-disabled vertices, U is shifted by +1.0 before encoding (range [1,2)).
  // Decode using the unsigned 16-bit pattern to recover the full encoded value,
  // then strip the mipmap flag. Use fmodf to handle any wrap from 16-bit truncation.
  float rawU = static_cast<float>(static_cast<uint16_t>(vertex.u)) / 8192.0f;
  float rawV = static_cast<float>(static_cast<uint16_t>(vertex.v)) / 8192.0f;
  // Only U carries the mipmap-disable flag (shifted into [1,2) range by the
  // Tesselator).  Strip that offset to recover the real atlas coordinate.
  // V never has this flag, so leave it untouched.
  if (rawU >= 1.0f)
  {
    rawU = fmodf(rawU, 1.0f);
  }
  result.texCoord[0] = rawU;
  result.texCoord[1] = rawV;

  // Compact vertices bias RGB565 by -32768 before storing into the signed short slot.
  const uint16_t packed = static_cast<uint16_t>(static_cast<uint16_t>(vertex.color565) + 0x8000u);
  const float sourceColor[4] = {
    static_cast<float>((packed >> 11) & 0x1f) / 31.0f,
    static_cast<float>((packed >> 5) & 0x3f) / 63.0f,
    static_cast<float>(packed & 0x1f) / 31.0f,
    1.0f
  };

  ThreadRenderContext &context = getThreadContext();
  result.color[0] = sourceColor[0] * context.renderState.colour[0];
  result.color[1] = sourceColor[1] * context.renderState.colour[1];
  result.color[2] = sourceColor[2] * context.renderState.colour[2];
  result.color[3] = sourceColor[3] * context.renderState.colour[3];
  if (context.renderState.blendEnabled &&
      context.renderState.blendSrc == GL_CONSTANT_ALPHA &&
      context.renderState.blendDst == GL_ONE_MINUS_CONSTANT_ALPHA)
  {
    result.color[3] *= context.renderState.blendFactorAlpha;
  }

  return result;
}

VulkanBootstrapApp::BlendMode determineBlendMode()
{
  if (!getThreadContext().renderState.blendEnabled)
  {
    return VulkanBootstrapApp::BlendMode::Opaque;
  }

  if (getThreadContext().renderState.blendDst == GL_ONE)
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
  const bool fog = getThreadContext().renderState.fogEnabled;
  const bool alphaTest = getThreadContext().renderState.alphaTestEnabled;
  if (fog && alphaTest)
  {
    return VulkanBootstrapApp::ShaderVariant::TexturedFogAlphaTest;
  }
  if (fog)
  {
    return VulkanBootstrapApp::ShaderVariant::TexturedFog;
  }
  if (alphaTest)
  {
    return VulkanBootstrapApp::ShaderVariant::TexturedAlphaTest;
  }
  return VulkanBootstrapApp::ShaderVariant::Textured;
}

size_t getVertexStride(C4JRender::eVertexType vertexType)
{
  switch (vertexType)
  {
  case C4JRender::VERTEX_TYPE_COMPRESSED:
    return sizeof(EngineVertexCompressed);
  case C4JRender::VERTEX_TYPE_PF3_TF2_CB4_NB4_XW1:
  case C4JRender::VERTEX_TYPE_PF3_TF2_CB4_NB4_XW1_LIT:
  case C4JRender::VERTEX_TYPE_PF3_TF2_CB4_NB4_XW1_TEXGEN:
    return sizeof(EngineVertex);
  default:
    return 0;
  }
}

bool buildRecordedDrawCommand(
  C4JRender::ePrimitiveType primitiveType,
  int count,
  void *dataIn,
  C4JRender::eVertexType vertexType,
  C4JRender::ePixelShaderType pixelShaderType,
  RecordedDrawCommand *commandOut);

bool dispatchDrawVertices(
  C4JRender::ePrimitiveType primitiveType,
  int count,
  void *dataIn,
  C4JRender::eVertexType vertexType,
  C4JRender::ePixelShaderType pixelShaderType)
{
  RecordedDrawCommand command {};
  if (!buildRecordedDrawCommand(primitiveType, count, dataIn, vertexType, pixelShaderType, &command))
  {
    return false;
  }

  ThreadRenderContext &context = getThreadContext();
  const Matrix &projection = context.projectionStack.back();
  const Matrix &modelView = context.modelViewStack.back();
  const Matrix mvp = multiplyMatrix(projection, modelView);

  g_vulkanBackend.submitVertices(
    VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
    command.vertices.data(),
    command.vertices.size(),
    command.shaderVariant,
    mvp.data(),
    command.renderState);
  return !command.vertices.empty();
}

bool buildRecordedDrawCommand(
  C4JRender::ePrimitiveType primitiveType,
  int count,
  void *dataIn,
  C4JRender::eVertexType vertexType,
  C4JRender::ePixelShaderType pixelShaderType,
  RecordedDrawCommand *commandOut)
{
  if (dataIn == nullptr || count <= 0)
  {
    return false;
  }
  if (vertexType != C4JRender::VERTEX_TYPE_COMPRESSED &&
      vertexType != C4JRender::VERTEX_TYPE_PF3_TF2_CB4_NB4_XW1 &&
      vertexType != C4JRender::VERTEX_TYPE_PF3_TF2_CB4_NB4_XW1_LIT &&
      vertexType != C4JRender::VERTEX_TYPE_PF3_TF2_CB4_NB4_XW1_TEXGEN)
  {
    return false;
  }

  (void)pixelShaderType;

  const EngineVertex *vertices = static_cast<const EngineVertex *>(dataIn);
  const EngineVertexCompressed *compressedVertices = static_cast<const EngineVertexCompressed *>(dataIn);

  std::vector<VulkanBootstrapApp::Vertex> converted;
  converted.reserve(static_cast<size_t>(count) * 2);

  if (primitiveType == C4JRender::PRIMITIVE_TYPE_QUAD_LIST)
  {
    for (int index = 0; index + 3 < count; index += 4)
    {
      const VulkanBootstrapApp::Vertex v0 = vertexType == C4JRender::VERTEX_TYPE_COMPRESSED ? convertCompressedVertex(compressedVertices[index + 0]) : convertVertex(vertices[index + 0]);
      const VulkanBootstrapApp::Vertex v1 = vertexType == C4JRender::VERTEX_TYPE_COMPRESSED ? convertCompressedVertex(compressedVertices[index + 1]) : convertVertex(vertices[index + 1]);
      const VulkanBootstrapApp::Vertex v2 = vertexType == C4JRender::VERTEX_TYPE_COMPRESSED ? convertCompressedVertex(compressedVertices[index + 2]) : convertVertex(vertices[index + 2]);
      const VulkanBootstrapApp::Vertex v3 = vertexType == C4JRender::VERTEX_TYPE_COMPRESSED ? convertCompressedVertex(compressedVertices[index + 3]) : convertVertex(vertices[index + 3]);
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
      converted.push_back(vertexType == C4JRender::VERTEX_TYPE_COMPRESSED ? convertCompressedVertex(compressedVertices[index]) : convertVertex(vertices[index]));
    }
  }
  else if (primitiveType == C4JRender::PRIMITIVE_TYPE_TRIANGLE_STRIP)
  {
    for (int index = 0; index + 2 < count; ++index)
    {
      const VulkanBootstrapApp::Vertex a = vertexType == C4JRender::VERTEX_TYPE_COMPRESSED ? convertCompressedVertex(compressedVertices[index + 0]) : convertVertex(vertices[index + 0]);
      const VulkanBootstrapApp::Vertex b = vertexType == C4JRender::VERTEX_TYPE_COMPRESSED ? convertCompressedVertex(compressedVertices[index + 1]) : convertVertex(vertices[index + 1]);
      const VulkanBootstrapApp::Vertex c = vertexType == C4JRender::VERTEX_TYPE_COMPRESSED ? convertCompressedVertex(compressedVertices[index + 2]) : convertVertex(vertices[index + 2]);
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
    const VulkanBootstrapApp::Vertex origin = vertexType == C4JRender::VERTEX_TYPE_COMPRESSED ? convertCompressedVertex(compressedVertices[0]) : convertVertex(vertices[0]);
    for (int index = 1; index + 1 < count; ++index)
    {
      converted.push_back(origin);
      converted.push_back(vertexType == C4JRender::VERTEX_TYPE_COMPRESSED ? convertCompressedVertex(compressedVertices[index]) : convertVertex(vertices[index]));
      converted.push_back(vertexType == C4JRender::VERTEX_TYPE_COMPRESSED ? convertCompressedVertex(compressedVertices[index + 1]) : convertVertex(vertices[index + 1]));
    }
  }
  else
  {
    return false;
  }

  ThreadRenderContext &context = getThreadContext();
  const int currentTexture = context.currentTextureIndex;
  VulkanBootstrapApp::RenderState renderState {};
  renderState.blendMode = determineBlendMode();
  renderState.depthTestEnabled = context.renderState.depthTestEnabled;
  renderState.depthWriteEnabled = context.renderState.depthTestEnabled && context.renderState.depthWriteEnabled;
  renderState.cullEnabled = context.renderState.cullEnabled;
  renderState.cullClockwise = context.renderState.cullClockwise;
  renderState.textureIndex = currentTexture;

  // The legacy compact chunk path emits face winding that is not yet stable
  // under the Vulkan translation layer. Prefer rendering both sides over
  // dropping terrain faces while we mirror the old fixed-function behavior.
  if (vertexType == C4JRender::VERTEX_TYPE_COMPRESSED)
  {
    renderState.cullEnabled = false;
  }

  const VulkanBootstrapApp::ShaderVariant variant = determineShaderVariant(currentTexture);
  if (commandOut != nullptr)
  {
    commandOut->shaderVariant = variant;
    commandOut->renderState = renderState;
    commandOut->vertices = std::move(converted);
    return !commandOut->vertices.empty();
  }

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
  ThreadRenderContext &context = getThreadContext();
  context.modelViewStack.assign(1, identity);
  context.projectionStack.assign(1, identity);
  context.textureStack.assign(1, identity);
  context.currentMatrixMode = GL_MODELVIEW;
  context.currentTextureIndex = -1;
  std::copy(identity.begin(), identity.end(), identityMatrix_);
  context.renderState = RenderStateTracker {};
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
void C4JRender::MatrixMode(int type) { getThreadContext().currentMatrixMode = type; }

void C4JRender::MatrixSetIdentity()
{
  const Matrix identity = identityMatrix();
  ThreadRenderContext &context = getThreadContext();
  if (context.currentMatrixMode == GL_PROJECTION)
  {
    context.projectionStack.back() = identity;
  }
  else if (context.currentMatrixMode == GL_TEXTURE)
  {
    context.textureStack.back() = identity;
  }
  else
  {
    context.modelViewStack.back() = identity;
  }
}

void C4JRender::MatrixTranslate(float x, float y, float z)
{
  const Matrix matrix = translationMatrix(x, y, z);
  ThreadRenderContext &context = getThreadContext();
  if (context.currentMatrixMode == GL_PROJECTION)
  {
    context.projectionStack.back() = multiplyMatrix(context.projectionStack.back(), matrix);
  }
  else if (context.currentMatrixMode == GL_TEXTURE)
  {
    context.textureStack.back() = multiplyMatrix(context.textureStack.back(), matrix);
  }
  else
  {
    context.modelViewStack.back() = multiplyMatrix(context.modelViewStack.back(), matrix);
  }
}

void C4JRender::MatrixRotate(float angle, float x, float y, float z)
{
  const Matrix matrix = rotationMatrix(angle, x, y, z);
  ThreadRenderContext &context = getThreadContext();
  if (context.currentMatrixMode == GL_PROJECTION)
  {
    context.projectionStack.back() = multiplyMatrix(context.projectionStack.back(), matrix);
  }
  else if (context.currentMatrixMode == GL_TEXTURE)
  {
    context.textureStack.back() = multiplyMatrix(context.textureStack.back(), matrix);
  }
  else
  {
    context.modelViewStack.back() = multiplyMatrix(context.modelViewStack.back(), matrix);
  }
}

void C4JRender::MatrixScale(float x, float y, float z)
{
  const Matrix matrix = scaleMatrix(x, y, z);
  ThreadRenderContext &context = getThreadContext();
  if (context.currentMatrixMode == GL_PROJECTION)
  {
    context.projectionStack.back() = multiplyMatrix(context.projectionStack.back(), matrix);
  }
  else if (context.currentMatrixMode == GL_TEXTURE)
  {
    context.textureStack.back() = multiplyMatrix(context.textureStack.back(), matrix);
  }
  else
  {
    context.modelViewStack.back() = multiplyMatrix(context.modelViewStack.back(), matrix);
  }
}

void C4JRender::MatrixPerspective(float fovy, float aspect, float zNear, float zFar)
{
  const Matrix matrix = perspectiveMatrix(fovy, aspect, zNear, zFar);
  ThreadRenderContext &context = getThreadContext();
  if (context.currentMatrixMode == GL_PROJECTION)
  {
    context.projectionStack.back() = multiplyMatrix(context.projectionStack.back(), matrix);
  }
  else
  {
    context.modelViewStack.back() = multiplyMatrix(context.modelViewStack.back(), matrix);
  }
}

void C4JRender::MatrixOrthogonal(float left, float right, float bottom, float top, float zNear, float zFar)
{
  const Matrix matrix = orthographicMatrix(left, right, bottom, top, zNear, zFar);
  ThreadRenderContext &context = getThreadContext();
  if (context.currentMatrixMode == GL_PROJECTION)
  {
    context.projectionStack.back() = multiplyMatrix(context.projectionStack.back(), matrix);
  }
  else
  {
    context.modelViewStack.back() = multiplyMatrix(context.modelViewStack.back(), matrix);
  }
}

void C4JRender::MatrixPop()
{
  ThreadRenderContext &context = getThreadContext();
  auto *stack = &context.modelViewStack;
  if (context.currentMatrixMode == GL_PROJECTION)
  {
    stack = &context.projectionStack;
  }
  else if (context.currentMatrixMode == GL_TEXTURE)
  {
    stack = &context.textureStack;
  }
  if (stack->size() > 1)
  {
    stack->pop_back();
  }
}

void C4JRender::MatrixPush()
{
  ThreadRenderContext &context = getThreadContext();
  auto *stack = &context.modelViewStack;
  if (context.currentMatrixMode == GL_PROJECTION)
  {
    stack = &context.projectionStack;
  }
  else if (context.currentMatrixMode == GL_TEXTURE)
  {
    stack = &context.textureStack;
  }
  stack->push_back(stack->back());
}

void C4JRender::MatrixMult(float *mat)
{
  Matrix matrix {};
  std::memcpy(matrix.data(), mat, sizeof(float) * 16);
  ThreadRenderContext &context = getThreadContext();
  if (context.currentMatrixMode == GL_PROJECTION)
  {
    context.projectionStack.back() = multiplyMatrix(context.projectionStack.back(), matrix);
  }
  else if (context.currentMatrixMode == GL_TEXTURE)
  {
    context.textureStack.back() = multiplyMatrix(context.textureStack.back(), matrix);
  }
  else
  {
    context.modelViewStack.back() = multiplyMatrix(context.modelViewStack.back(), matrix);
  }
}

const float *C4JRender::MatrixGet(int type)
{
  ThreadRenderContext &context = getThreadContext();
  if (type == GL_PROJECTION_MATRIX || type == GL_PROJECTION)
  {
    return context.projectionStack.back().data();
  }
  if (type == GL_TEXTURE)
  {
    return context.textureStack.back().data();
  }
  return context.modelViewStack.back().data();
}

void C4JRender::Set_matrixDirty() {}
void C4JRender::InitialiseContext() {}
void C4JRender::StartFrame() { g_vulkanBackend.beginFrame(); }
void C4JRender::DoScreenGrabOnNextPresent() {}
void C4JRender::Present() { g_vulkanBackend.tickFrame(); }
void C4JRender::Clear(int flags, D3D11_RECT *)
{
  if (flags == 0)
  {
    return;
  }
  g_vulkanBackend.requestClear(static_cast<uint32_t>(flags));
}

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
    RecordedDrawCommand command {};
    if (!buildRecordedDrawCommand(primitiveType, count, dataIn, vType, psType, &command))
    {
      return;
    }

    ThreadRenderContext &context = getThreadContext();
    const Matrix &currentModelView = context.modelViewStack.back();
    std::lock_guard<std::mutex> lock(g_commandBufferMutex);
    RecordedCommandBuffer &buffer = g_commandBuffers[g_activeCommandBufferIndex];
    command.localModelView = multiplyMatrix(invertMatrix(buffer.baseModelView), currentModelView);
    buffer.draws.push_back(std::move(command));
    buffer.bytes += buffer.draws.back().vertices.size() * sizeof(VulkanBootstrapApp::Vertex);
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

  std::lock_guard<std::mutex> lock(g_commandBufferMutex);
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
  std::lock_guard<std::mutex> lock(g_commandBufferMutex);
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
  ThreadRenderContext &context = getThreadContext();
  std::lock_guard<std::mutex> lock(g_commandBufferMutex);
  RecordedCommandBuffer &buffer = g_commandBuffers[index];
  buffer.draws.clear();
  buffer.bytes = 0;
  buffer.baseModelView = context.modelViewStack.back();
  g_activeCommandBufferIndex = index;
}

void C4JRender::CBuffClear(int index)
{
  std::lock_guard<std::mutex> lock(g_commandBufferMutex);
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
  std::lock_guard<std::mutex> lock(g_commandBufferMutex);
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
  ThreadRenderContext &context = getThreadContext();
  const Matrix &projection = context.projectionStack.back();
  const Matrix &modelView = context.modelViewStack.back();

  std::lock_guard<std::mutex> lock(g_commandBufferMutex);
  const auto it = g_commandBuffers.find(index);
  if (it == g_commandBuffers.end() || it->second.draws.empty())
  {
    return false;
  }

  g_replayingCommandBuffer = true;
  for (const RecordedDrawCommand &command : it->second.draws)
  {
    const Matrix replayModelView = multiplyMatrix(modelView, command.localModelView);
    const Matrix mvp = multiplyMatrix(projection, replayModelView);
    VulkanBootstrapApp::RenderState replayState = command.renderState;
    replayState.textureIndex = context.currentTextureIndex;
    VulkanBootstrapApp::ShaderVariant replayVariant =
      replayState.textureIndex < 0
      ? VulkanBootstrapApp::ShaderVariant::ColorOnly
      : determineShaderVariant(replayState.textureIndex);
    g_vulkanBackend.submitVertices(
      VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
      command.vertices.data(),
      command.vertices.size(),
      replayVariant,
      mvp.data(),
      replayState);
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
  getThreadContext().currentTextureIndex = idx;
}

void C4JRender::TextureBindVertex(int) {}

void C4JRender::TextureSetTextureLevels(int levels)
{
  getThreadContext().renderState.textureLevels = std::max(levels, 1);
}

int C4JRender::TextureGetTextureLevels()
{
  return getThreadContext().renderState.textureLevels;
}

void C4JRender::TextureData(int width, int height, void *data, int level, eTextureFormat)
{
  if (level != 0 || data == nullptr)
  {
    return;
  }

  const int currentTexture = getThreadContext().currentTextureIndex;
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

  const int currentTexture = getThreadContext().currentTextureIndex;
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
  const int currentTexture = getThreadContext().currentTextureIndex;
  if (currentTexture < 0)
  {
    return;
  }

  switch (param)
  {
  case GL_TEXTURE_MIN_FILTER:
  case GL_TEXTURE_MAG_FILTER:
    if (value == GL_LINEAR || value == GL_NEAREST || value == GL_NEAREST_MIPMAP_LINEAR)
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
  getThreadContext().renderState.colour = {r, g, b, a};
}

void C4JRender::StateSetDepthMask(bool enable)
{
  getThreadContext().renderState.depthWriteEnabled = enable;
}

void C4JRender::StateSetBlendEnable(bool enable)
{
  getThreadContext().renderState.blendEnabled = enable;
}

void C4JRender::StateSetBlendFunc(int src, int dst)
{
  ThreadRenderContext &context = getThreadContext();
  context.renderState.blendSrc = src;
  context.renderState.blendDst = dst;
}

void C4JRender::StateSetBlendFactor(unsigned int colour)
{
  getThreadContext().renderState.blendFactorAlpha = static_cast<float>((colour >> 24) & 0xff) / 255.0f;
}

void C4JRender::StateSetAlphaFunc(int, float param)
{
  getThreadContext().renderState.alphaReference = param;
}

void C4JRender::StateSetDepthFunc(int) {}

void C4JRender::StateSetFaceCull(bool enable)
{
  getThreadContext().renderState.cullEnabled = enable;
}

void C4JRender::StateSetFaceCullCW(bool enable)
{
  getThreadContext().renderState.cullClockwise = enable;
}

void C4JRender::StateSetLineWidth(float) {}
void C4JRender::StateSetWriteEnable(bool, bool, bool, bool) {}

void C4JRender::StateSetDepthTestEnable(bool enable)
{
  getThreadContext().renderState.depthTestEnabled = enable;
}

void C4JRender::StateSetAlphaTestEnable(bool enable)
{
  getThreadContext().renderState.alphaTestEnabled = enable;
}

void C4JRender::StateSetDepthSlopeAndBias(float, float) {}

void C4JRender::StateSetFogEnable(bool enable)
{
  getThreadContext().renderState.fogEnabled = enable;
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
void C4JRender::StateSetViewport(eViewportType viewportType)
{
  int framebufferWidth = 0;
  int framebufferHeight = 0;
  if (window_ == nullptr)
  {
    return;
  }

  glfwGetFramebufferSize(window_, &framebufferWidth, &framebufferHeight);
  if (framebufferWidth <= 0 || framebufferHeight <= 0)
  {
    return;
  }

  int x = 0;
  int y = 0;
  uint32_t width = static_cast<uint32_t>(framebufferWidth);
  uint32_t height = static_cast<uint32_t>(framebufferHeight);

  switch (viewportType)
  {
  case VIEWPORT_TYPE_FULLSCREEN:
    break;
  case VIEWPORT_TYPE_SPLIT_TOP:
    x = framebufferWidth / 4;
    width = static_cast<uint32_t>(framebufferWidth / 2);
    height = static_cast<uint32_t>(framebufferHeight / 2);
    break;
  case VIEWPORT_TYPE_SPLIT_BOTTOM:
    x = framebufferWidth / 4;
    y = framebufferHeight / 2;
    width = static_cast<uint32_t>(framebufferWidth / 2);
    height = static_cast<uint32_t>(framebufferHeight / 2);
    break;
  case VIEWPORT_TYPE_SPLIT_LEFT:
    y = framebufferHeight / 4;
    width = static_cast<uint32_t>(framebufferWidth / 2);
    height = static_cast<uint32_t>(framebufferHeight / 2);
    break;
  case VIEWPORT_TYPE_SPLIT_RIGHT:
    x = framebufferWidth / 2;
    y = framebufferHeight / 4;
    width = static_cast<uint32_t>(framebufferWidth / 2);
    height = static_cast<uint32_t>(framebufferHeight / 2);
    break;
  case VIEWPORT_TYPE_QUADRANT_TOP_LEFT:
    width = static_cast<uint32_t>(framebufferWidth / 2);
    height = static_cast<uint32_t>(framebufferHeight / 2);
    break;
  case VIEWPORT_TYPE_QUADRANT_TOP_RIGHT:
    x = framebufferWidth / 2;
    width = static_cast<uint32_t>(framebufferWidth / 2);
    height = static_cast<uint32_t>(framebufferHeight / 2);
    break;
  case VIEWPORT_TYPE_QUADRANT_BOTTOM_LEFT:
    y = framebufferHeight / 2;
    width = static_cast<uint32_t>(framebufferWidth / 2);
    height = static_cast<uint32_t>(framebufferHeight / 2);
    break;
  case VIEWPORT_TYPE_QUADRANT_BOTTOM_RIGHT:
    x = framebufferWidth / 2;
    y = framebufferHeight / 2;
    width = static_cast<uint32_t>(framebufferWidth / 2);
    height = static_cast<uint32_t>(framebufferHeight / 2);
    break;
  }

  g_vulkanBackend.setViewportRect(x, y, width, height);
}
void C4JRender::StateSetEnableViewportClipPlanes(bool) {}
void C4JRender::StateSetTexGenCol(int, float, float, float, float, bool) {}
void C4JRender::StateSetStencil(int, uint8_t, uint8_t, uint8_t) {}
void C4JRender::StateSetForceLOD(int) {}
void C4JRender::BeginEvent(const wchar_t *) {}
void C4JRender::EndEvent() {}
void C4JRender::Suspend() { suspended_ = true; }
bool C4JRender::Suspended() { return suspended_; }
void C4JRender::Resume() { suspended_ = false; }
