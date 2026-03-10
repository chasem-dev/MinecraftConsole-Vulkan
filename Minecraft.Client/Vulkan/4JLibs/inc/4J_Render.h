#pragma once

#ifndef GLFW_INCLUDE_NONE
#define GLFW_INCLUDE_NONE
#endif
#include <GLFW/glfw3.h>

#include <array>
#include <cstdint>
#include <vector>

// When compiled as part of the legacy build, these types come from
// OrbisTypes.h (via AppleStubs.h → stdafx.h).  For standalone Vulkan
// bootstrap targets that don't include stdafx.h, define them here.
#ifndef _4J_RENDER_BASIC_TYPES
#define _4J_RENDER_BASIC_TYPES
typedef unsigned char BYTE;
typedef unsigned int DWORD;
typedef int HRESULT;

struct D3D11_RECT
{
  long left;
  long top;
  long right;
  long bottom;
};
#endif

struct D3DXIMAGE_INFO
{
  int Width;
  int Height;
};

struct ID3D11Device {};
struct IDXGISwapChain {};
struct ID3D11Buffer {};
struct ID3D11ShaderResourceView {};

class ImageFileBuffer
{
public:
  enum EImageType
  {
    e_typePNG,
    e_typeJPG
  };

  EImageType m_type;
  void *m_pBuffer;
  int m_bufferSize;

  int GetType() { return m_type; }
  void *GetBufferPointer() { return m_pBuffer; }
  int GetBufferSize() { return m_bufferSize; }
  void Release() {}
  bool Allocated() { return m_pBuffer != nullptr; }
};

typedef struct _XSOCIAL_PREVIEWIMAGE {
  BYTE *pBytes;
  DWORD Pitch;
  DWORD Width;
  DWORD Height;
} XSOCIAL_PREVIEWIMAGE, *PXSOCIAL_PREVIEWIMAGE;

class C4JRender
{
public:
  enum eVertexType
  {
    VERTEX_TYPE_PF3_TF2_CB4_NB4_XW1,
    VERTEX_TYPE_COMPRESSED,
    VERTEX_TYPE_PF3_TF2_CB4_NB4_XW1_LIT,
    VERTEX_TYPE_PF3_TF2_CB4_NB4_XW1_TEXGEN,
    VERTEX_TYPE_COUNT
  };

  enum ePixelShaderType
  {
    PIXEL_SHADER_TYPE_STANDARD,
    PIXEL_SHADER_TYPE_PROJECTION,
    PIXEL_SHADER_TYPE_FORCELOD,
    PIXEL_SHADER_COUNT
  };

  enum eViewportType
  {
    VIEWPORT_TYPE_FULLSCREEN,
    VIEWPORT_TYPE_SPLIT_TOP,
    VIEWPORT_TYPE_SPLIT_BOTTOM,
    VIEWPORT_TYPE_SPLIT_LEFT,
    VIEWPORT_TYPE_SPLIT_RIGHT,
    VIEWPORT_TYPE_QUADRANT_TOP_LEFT,
    VIEWPORT_TYPE_QUADRANT_TOP_RIGHT,
    VIEWPORT_TYPE_QUADRANT_BOTTOM_LEFT,
    VIEWPORT_TYPE_QUADRANT_BOTTOM_RIGHT,
  };

  enum ePrimitiveType
  {
    PRIMITIVE_TYPE_TRIANGLE_LIST,
    PRIMITIVE_TYPE_TRIANGLE_STRIP,
    PRIMITIVE_TYPE_TRIANGLE_FAN,
    PRIMITIVE_TYPE_QUAD_LIST,
    PRIMITIVE_TYPE_LINE_LIST,
    PRIMITIVE_TYPE_LINE_STRIP,
    PRIMITIVE_TYPE_COUNT
  };

  enum eTextureFormat
  {
    TEXTURE_FORMAT_RxGyBzAw,
    MAX_TEXTURE_FORMATS
  };

  void Initialise(ID3D11Device *pDevice, IDXGISwapChain *pSwapChain);
  void InitialiseForWindow(GLFWwindow *window);
  void Shutdown();
  void Tick();
  void UpdateGamma(unsigned short usGamma);
  void MatrixMode(int type);
  void MatrixSetIdentity();
  void MatrixTranslate(float x, float y, float z);
  void MatrixRotate(float angle, float x, float y, float z);
  void MatrixScale(float x, float y, float z);
  void MatrixPerspective(float fovy, float aspect, float zNear, float zFar);
  void MatrixOrthogonal(float left, float right, float bottom, float top, float zNear, float zFar);
  void MatrixPop();
  void MatrixPush();
  void MatrixMult(float *mat);
  const float *MatrixGet(int type);
  void Set_matrixDirty();
  void InitialiseContext();
  void StartFrame();
  void DoScreenGrabOnNextPresent();
  void Present();
  void Clear(int flags, D3D11_RECT *pRect = nullptr);
  void SetClearColour(const float colourRGBA[4]);
  bool IsWidescreen();
  bool IsHiDef();
  void CaptureThumbnail(ImageFileBuffer *pngOut);
  void CaptureScreen(ImageFileBuffer *jpgOut, XSOCIAL_PREVIEWIMAGE *previewOut);
  void BeginConditionalSurvey(int identifier);
  void EndConditionalSurvey();
  void BeginConditionalRendering(int identifier);
  void EndConditionalRendering();
  void DrawVertices(ePrimitiveType PrimitiveType, int count, void *dataIn, eVertexType vType, ePixelShaderType psType);
  void DrawVertexBuffer(ePrimitiveType PrimitiveType, int count, ID3D11Buffer *buffer, eVertexType vType, ePixelShaderType psType);
  void CBuffLockStaticCreations();
  int CBuffCreate(int count);
  void CBuffDelete(int first, int count);
  void CBuffStart(int index, bool full = false);
  void CBuffClear(int index);
  int CBuffSize(int index);
  void CBuffEnd();
  bool CBuffCall(int index, bool full = true);
  void CBuffTick();
  void CBuffDeferredModeStart();
  void CBuffDeferredModeEnd();
  int TextureCreate();
  void TextureFree(int idx);
  void TextureBind(int idx);
  void TextureBindVertex(int idx);
  void TextureSetTextureLevels(int levels);
  int TextureGetTextureLevels();
  void TextureData(int width, int height, void *data, int level, eTextureFormat format = TEXTURE_FORMAT_RxGyBzAw);
  void TextureDataUpdate(int xoffset, int yoffset, int width, int height, void *data, int level);
  void TextureSetParam(int param, int value);
  void TextureDynamicUpdateStart();
  void TextureDynamicUpdateEnd();
  HRESULT LoadTextureData(const char *szFilename, D3DXIMAGE_INFO *pSrcInfo, int **ppDataOut);
  HRESULT LoadTextureData(BYTE *pbData, DWORD dwBytes, D3DXIMAGE_INFO *pSrcInfo, int **ppDataOut);
  HRESULT SaveTextureData(const char *szFilename, D3DXIMAGE_INFO *pSrcInfo, int *ppDataOut);
  HRESULT SaveTextureDataToMemory(void *pOutput, int outputCapacity, int *outputLength, int width, int height, int *ppDataIn);
  void TextureGetStats();
  ID3D11ShaderResourceView *TextureGetTexture(int idx);
  void StateSetColour(float r, float g, float b, float a);
  void StateSetDepthMask(bool enable);
  void StateSetBlendEnable(bool enable);
  void StateSetBlendFunc(int src, int dst);
  void StateSetBlendFactor(unsigned int colour);
  void StateSetAlphaFunc(int func, float param);
  void StateSetDepthFunc(int func);
  void StateSetFaceCull(bool enable);
  void StateSetFaceCullCW(bool enable);
  void StateSetLineWidth(float width);
  void StateSetWriteEnable(bool red, bool green, bool blue, bool alpha);
  void StateSetDepthTestEnable(bool enable);
  void StateSetAlphaTestEnable(bool enable);
  void StateSetDepthSlopeAndBias(float slope, float bias);
  void StateSetFogEnable(bool enable);
  void StateSetFogMode(int mode);
  void StateSetFogNearDistance(float dist);
  void StateSetFogFarDistance(float dist);
  void StateSetFogDensity(float density);
  void StateSetFogColour(float red, float green, float blue);
  void StateSetLightingEnable(bool enable);
  void StateSetVertexTextureUV(float u, float v);
  void StateSetLightColour(int light, float red, float green, float blue);
  void StateSetLightAmbientColour(float red, float green, float blue);
  void StateSetLightDirection(int light, float x, float y, float z);
  void StateSetLightEnable(int light, bool enable);
  void StateSetViewport(eViewportType viewportType);
  void SetViewportRect(int x, int y, int w, int h);
  void StateSetEnableViewportClipPlanes(bool enable);
  void StateSetTexGenCol(int col, float x, float y, float z, float w, bool eyeSpace);
  void StateSetStencil(int Function, uint8_t stencil_ref, uint8_t stencil_func_mask, uint8_t stencil_write_mask);
  void StateSetForceLOD(int LOD);
  void BeginEvent(const wchar_t *eventName);
  void EndEvent();
  void Suspend();
  bool Suspended();
  void Resume();

  struct VulkanDebugStats
  {
    double drawFrameMs;
    double fenceWaitMs;
    unsigned int vertexCount;
    unsigned int batchCount;
    unsigned int textureCount;
    unsigned int swapchainImageCount;
    const char *presentModeName;
    char gpuName[256];
    unsigned int swapchainWidth;
    unsigned int swapchainHeight;
  };
  VulkanDebugStats GetVulkanDebugStats();

private:
  GLFWwindow *window_ = nullptr;
  float clearColour_[4] = {0.05f, 0.06f, 0.09f, 1.0f};
  float identityMatrix_[16] = {
    1.0f, 0.0f, 0.0f, 0.0f,
    0.0f, 1.0f, 0.0f, 0.0f,
    0.0f, 0.0f, 1.0f, 0.0f,
    0.0f, 0.0f, 0.0f, 1.0f
  };
  bool suspended_ = false;
  int currentMatrixMode_ = 0;
  std::vector<std::array<float, 16>> modelViewStack_;
  std::vector<std::array<float, 16>> projectionStack_;
  std::vector<std::array<float, 16>> textureStack_;
};

#ifdef GL_MODELVIEW_MATRIX
#undef GL_MODELVIEW_MATRIX
#endif
#ifdef GL_PROJECTION_MATRIX
#undef GL_PROJECTION_MATRIX
#endif
#ifdef GL_MODELVIEW
#undef GL_MODELVIEW
#endif
#ifdef GL_PROJECTION
#undef GL_PROJECTION
#endif
#ifdef GL_TEXTURE
#undef GL_TEXTURE
#endif
#ifdef GL_TEXTURE_2D
#undef GL_TEXTURE_2D
#endif
#ifdef GL_BLEND
#undef GL_BLEND
#endif
#ifdef GL_CULL_FACE
#undef GL_CULL_FACE
#endif
#ifdef GL_ALPHA_TEST
#undef GL_ALPHA_TEST
#endif
#ifdef GL_DEPTH_TEST
#undef GL_DEPTH_TEST
#endif
#ifdef GL_FOG
#undef GL_FOG
#endif
#ifdef GL_LIGHTING
#undef GL_LIGHTING
#endif
#ifdef GL_LIGHT0
#undef GL_LIGHT0
#endif
#ifdef GL_LIGHT1
#undef GL_LIGHT1
#endif
#ifdef GL_DEPTH_BUFFER_BIT
#undef GL_DEPTH_BUFFER_BIT
#endif
#ifdef GL_COLOR_BUFFER_BIT
#undef GL_COLOR_BUFFER_BIT
#endif
#ifdef GL_SRC_ALPHA
#undef GL_SRC_ALPHA
#endif
#ifdef GL_ONE_MINUS_SRC_ALPHA
#undef GL_ONE_MINUS_SRC_ALPHA
#endif
#ifdef GL_ONE
#undef GL_ONE
#endif
#ifdef GL_ZERO
#undef GL_ZERO
#endif
#ifdef GL_DST_ALPHA
#undef GL_DST_ALPHA
#endif
#ifdef GL_SRC_COLOR
#undef GL_SRC_COLOR
#endif
#ifdef GL_DST_COLOR
#undef GL_DST_COLOR
#endif
#ifdef GL_ONE_MINUS_DST_COLOR
#undef GL_ONE_MINUS_DST_COLOR
#endif
#ifdef GL_ONE_MINUS_SRC_COLOR
#undef GL_ONE_MINUS_SRC_COLOR
#endif
#ifdef GL_CONSTANT_ALPHA
#undef GL_CONSTANT_ALPHA
#endif
#ifdef GL_ONE_MINUS_CONSTANT_ALPHA
#undef GL_ONE_MINUS_CONSTANT_ALPHA
#endif
#ifdef GL_GREATER
#undef GL_GREATER
#endif
#ifdef GL_EQUAL
#undef GL_EQUAL
#endif
#ifdef GL_LEQUAL
#undef GL_LEQUAL
#endif
#ifdef GL_GEQUAL
#undef GL_GEQUAL
#endif
#ifdef GL_ALWAYS
#undef GL_ALWAYS
#endif
#ifdef GL_TEXTURE_MIN_FILTER
#undef GL_TEXTURE_MIN_FILTER
#endif
#ifdef GL_TEXTURE_MAG_FILTER
#undef GL_TEXTURE_MAG_FILTER
#endif
#ifdef GL_TEXTURE_WRAP_S
#undef GL_TEXTURE_WRAP_S
#endif
#ifdef GL_TEXTURE_WRAP_T
#undef GL_TEXTURE_WRAP_T
#endif
#ifdef GL_NEAREST
#undef GL_NEAREST
#endif
#ifdef GL_LINEAR
#undef GL_LINEAR
#endif
#ifdef GL_EXP
#undef GL_EXP
#endif
#ifdef GL_NEAREST_MIPMAP_LINEAR
#undef GL_NEAREST_MIPMAP_LINEAR
#endif
#ifdef GL_CLAMP
#undef GL_CLAMP
#endif
#ifdef GL_REPEAT
#undef GL_REPEAT
#endif
#ifdef GL_FOG_START
#undef GL_FOG_START
#endif
#ifdef GL_FOG_END
#undef GL_FOG_END
#endif
#ifdef GL_FOG_MODE
#undef GL_FOG_MODE
#endif
#ifdef GL_FOG_DENSITY
#undef GL_FOG_DENSITY
#endif
#ifdef GL_FOG_COLOR
#undef GL_FOG_COLOR
#endif
#ifdef GL_POSITION
#undef GL_POSITION
#endif
#ifdef GL_AMBIENT
#undef GL_AMBIENT
#endif
#ifdef GL_DIFFUSE
#undef GL_DIFFUSE
#endif
#ifdef GL_SPECULAR
#undef GL_SPECULAR
#endif
#ifdef GL_LIGHT_MODEL_AMBIENT
#undef GL_LIGHT_MODEL_AMBIENT
#endif
#ifdef GL_BACK
#undef GL_BACK
#endif
#ifdef GL_LINES
#undef GL_LINES
#endif
#ifdef GL_LINE_STRIP
#undef GL_LINE_STRIP
#endif
#ifdef GL_QUADS
#undef GL_QUADS
#endif
#ifdef GL_TRIANGLE_FAN
#undef GL_TRIANGLE_FAN
#endif
#ifdef GL_TRIANGLE_STRIP
#undef GL_TRIANGLE_STRIP
#endif

const int GL_MODELVIEW_MATRIX = 0;
const int GL_PROJECTION_MATRIX = 1;
const int GL_MODELVIEW = 0;
const int GL_PROJECTION = 1;
const int GL_TEXTURE = 2;
const int GL_TEXTURE_2D = 1;
const int GL_BLEND = 2;
const int GL_CULL_FACE = 3;
const int GL_ALPHA_TEST = 4;
const int GL_DEPTH_TEST = 5;
const int GL_FOG = 6;
const int GL_LIGHTING = 7;
const int GL_LIGHT0 = 8;
const int GL_LIGHT1 = 9;
const int CLEAR_DEPTH_FLAG = 1;
const int CLEAR_COLOUR_FLAG = 2;
const int GL_DEPTH_BUFFER_BIT = CLEAR_DEPTH_FLAG;
const int GL_COLOR_BUFFER_BIT = CLEAR_COLOUR_FLAG;
const int GL_SRC_ALPHA = 1;
const int GL_ONE_MINUS_SRC_ALPHA = 2;
const int GL_ONE = 3;
const int GL_ZERO = 4;
const int GL_DST_ALPHA = 5;
const int GL_SRC_COLOR = 6;
const int GL_DST_COLOR = 7;
const int GL_ONE_MINUS_DST_COLOR = 8;
const int GL_ONE_MINUS_SRC_COLOR = 9;
const int GL_CONSTANT_ALPHA = 10;
const int GL_ONE_MINUS_CONSTANT_ALPHA = 11;
const int GL_GREATER = 1;
const int GL_EQUAL = 2;
const int GL_LEQUAL = 3;
const int GL_GEQUAL = 4;
const int GL_ALWAYS = 5;
const int GL_TEXTURE_MIN_FILTER = 1;
const int GL_TEXTURE_MAG_FILTER = 2;
const int GL_TEXTURE_WRAP_S = 3;
const int GL_TEXTURE_WRAP_T = 4;
const int GL_NEAREST = 0;
const int GL_LINEAR = 1;
const int GL_EXP = 2;
const int GL_NEAREST_MIPMAP_LINEAR = 3;
const int GL_CLAMP = 0;
const int GL_REPEAT = 1;
const int GL_FOG_START = 1;
const int GL_FOG_END = 2;
const int GL_FOG_MODE = 3;
const int GL_FOG_DENSITY = 4;
const int GL_FOG_COLOR = 5;
const int GL_POSITION = 1;
const int GL_AMBIENT = 2;
const int GL_DIFFUSE = 3;
const int GL_SPECULAR = 4;
const int GL_LIGHT_MODEL_AMBIENT = 1;
const int GL_BACK = 1;
const int GL_LINES = C4JRender::PRIMITIVE_TYPE_LINE_LIST;
const int GL_LINE_STRIP = C4JRender::PRIMITIVE_TYPE_LINE_STRIP;
const int GL_QUADS = C4JRender::PRIMITIVE_TYPE_QUAD_LIST;
const int GL_TRIANGLE_FAN = C4JRender::PRIMITIVE_TYPE_TRIANGLE_FAN;
const int GL_TRIANGLE_STRIP = C4JRender::PRIMITIVE_TYPE_TRIANGLE_STRIP;

extern C4JRender RenderManager;
