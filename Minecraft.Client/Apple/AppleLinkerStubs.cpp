// =============================================================================
// AppleLinkerStubs.cpp
//
// Stub implementations for all undefined symbols encountered during the macOS
// port link step.  Every function here is a no-op / default-value stub so the
// binary links.  Real implementations can replace these one-by-one.
// =============================================================================

#include "../stdafx.h"

#include "../Tesselator.h"
#include "../Minecraft.h"
#include "../Textures.h"
#include "../Font.h"
#include "../Gui.h"
#include "../ItemRenderer.h"
#include "../Lighting.h"
#include "../ProgressRenderer.h"
#include "../Common/UI/UI.h"
#include "../Common/UI/UIScene_SettingsAudioMenu.h"
#include "../Vulkan/Vulkan_UIController.h"

#include "../MultiplayerLocalPlayer.h"
#include "../../Minecraft.World/InventoryMenu.h"
#include "../../Minecraft.World/Slot.h"

#include <chrono>
#include <cctype>
#include <cstdarg>
#include <cstdlib>
#include <cmath>
#include <unordered_map>
#include <vector>

// ---------------------------------------------------------------------------
// Mouse / button hover tracking for Apple UI scenes
// ---------------------------------------------------------------------------
extern double g_cursorX, g_cursorY;
extern int g_iScreenWidth, g_iScreenHeight;

// Hover state: set during IggyPlayerDraw, read by UIScene handleInput
static int  g_appleHoveredControlId = -1;
static int  g_appleHoveredScene     = -1;  // EUIScene as int
static int  g_mainMenuSelectedIdx   = 0;   // keyboard-tracked selection (0-5)
static int  g_loadOrJoinSelectedIdx = 0;   // keyboard-tracked selection for LoadOrJoin
static int  g_createWorldSelectedIdx = 0;  // keyboard-tracked selection for CreateWorld rows
static bool g_createWorldSurvival  = true; // mirrors m_bGameModeSurvival for draw code
static int  g_pauseMenuSelectedIdx  = 0;   // keyboard-tracked selection for PauseMenu (0-2)

// Check if GLFW cursor is over a game-space rectangle.
// (gx,gy,gw,gh) in game-space coords, scale = GUI scale, swfW/swfH = SWF pixel dims.
static bool isMouseOverGameRect(float gx, float gy, float gw, float gh,
                                float scale, float swfW, float swfH)
{
  float mx = (float)(g_cursorX * (double)swfW / (double)g_iScreenWidth);
  float my = (float)(g_cursorY * (double)swfH / (double)g_iScreenHeight);
  float bx = gx * scale, by = gy * scale;
  float bw = gw * scale, bh = gh * scale;
  return (mx >= bx && mx < bx + bw && my >= by && my < by + bh);
}

// Exported for UIScene input code
int  AppleMouse_GetHoveredControlId() { return g_appleHoveredControlId; }
int  AppleMouse_GetHoveredScene()     { return g_appleHoveredScene; }
bool AppleMouse_IsOverButton()        { return g_appleHoveredControlId >= 0; }
int  AppleMouse_GetMainMenuSelected() { return g_mainMenuSelectedIdx; }
void AppleMouse_SetMainMenuSelected(int idx) { g_mainMenuSelectedIdx = idx; }
int  AppleMouse_GetLoadOrJoinSelected() { return g_loadOrJoinSelectedIdx; }
void AppleMouse_SetLoadOrJoinSelected(int idx) { g_loadOrJoinSelectedIdx = idx; }
int  AppleMouse_GetCreateWorldSelected() { return g_createWorldSelectedIdx; }
void AppleMouse_SetCreateWorldSelected(int idx) { g_createWorldSelectedIdx = idx; }
bool AppleMouse_GetCreateWorldSurvival() { return g_createWorldSurvival; }
void AppleMouse_SetCreateWorldSurvival(bool val) { g_createWorldSurvival = val; }
int  AppleMouse_GetPauseMenuSelected() { return g_pauseMenuSelectedIdx; }
void AppleMouse_SetPauseMenuSelected(int idx) { g_pauseMenuSelectedIdx = idx; }

// ---------------------------------------------------------------------------
// Iggy (RAD UI library) -- C linkage stubs
// ---------------------------------------------------------------------------
// iggy.h is already included transitively through stdafx.h.  Its declarations
// are wrapped in RADDEFSTART / RADDEFEND (= extern "C") and on macOS
// RADEXPFUNC / RADEXPLINK expand to nothing, so simple C-linkage definitions
// with matching signatures are sufficient.
// ---------------------------------------------------------------------------

namespace
{
struct FakeIggyLibrary
{
  std::wstring name;
};

std::unordered_map<IggyLibrary, FakeIggyLibrary *> g_fakeLibraries;
IggyLibrary g_nextFakeLibrary = 1;

// Stored custom draw callback so IggyPlayerDraw can invoke it
Iggy_CustomDrawCallback *g_customDrawCallback = nullptr;
void *g_customDrawCallbackData = nullptr;

struct FakeIggyPlayer
{
  IggyProperties properties = {};
  IggyValuePath rootPath = {};
  void *userdata = nullptr;
  std::unordered_map<IggyName, std::string> fastNames;
  IggyName nextFastName = 1;
  bool tickPending = true;
  std::chrono::steady_clock::time_point nextTickTime = std::chrono::steady_clock::now();
};

FakeIggyPlayer *toFakePlayer(Iggy *player)
{
  return reinterpret_cast<FakeIggyPlayer *>(player);
}

std::string normalizeAscii(std::string text)
{
  for (char &ch : text)
  {
    ch = (char)std::tolower((unsigned char)ch);
  }
  return text;
}

std::string lookupFastName(FakeIggyPlayer *player, IggyName name)
{
  if (player == nullptr)
  {
    return std::string();
  }

  AUTO_VAR(it, player->fastNames.find(name));
  if (it == player->fastNames.end())
  {
    return std::string();
  }

  return it->second;
}

void setNumericResult(IggyDataValue *result, double value)
{
  if (result == nullptr)
  {
    return;
  }

  std::memset(result, 0, sizeof(*result));
  result->type = IGGY_DATATYPE_number;
  result->number = value;
}

void setBooleanResult(IggyDataValue *result, rrbool value)
{
  if (result == nullptr)
  {
    return;
  }

  std::memset(result, 0, sizeof(*result));
  result->type = IGGY_DATATYPE_boolean;
  result->boolval = value;
}

void setEmptyStringResult(IggyDataValue *result)
{
  static IggyUTF16 emptyString[1] = {0};

  if (result == nullptr)
  {
    return;
  }

  std::memset(result, 0, sizeof(*result));
  result->type = IGGY_DATATYPE_string_UTF16;
  result->string16.string = emptyString;
  result->string16.length = 0;
}
}

extern "C" {

void IggyInit(IggyAllocator *) {}

Iggy *IggyPlayerCreateFromMemory(void const *, U32, IggyPlayerConfig *)
{
  FakeIggyPlayer *player = new FakeIggyPlayer();
  player->properties.movie_width_in_pixels = 1920;
  player->properties.movie_height_in_pixels = 1080;
  player->properties.movie_frame_rate_current_in_fps = 60.0f;
  player->properties.movie_frame_rate_from_file_in_fps = 60.0f;
  player->properties.swf_major_version_number = 9;
  player->properties.seconds_per_drawn_frame = 1.0 / 60.0;
  player->rootPath.f = reinterpret_cast<Iggy *>(player);
  return reinterpret_cast<Iggy *>(player);
}

void IggyPlayerDestroy(Iggy *player)
{
  if (player != nullptr) delete toFakePlayer(player);
}

IggyLibrary IggyLibraryCreateFromMemoryUTF16(IggyUTF16 const *name, void const *, U32, IggyPlayerConfig *)
{
  FakeIggyLibrary *library = new FakeIggyLibrary();
  if (name != nullptr)
  {
    library->name = std::wstring((const wchar_t *)name);
  }
  const IggyLibrary handle = g_nextFakeLibrary++;
  g_fakeLibraries[handle] = library;
  return handle;
}

void IggyLibraryDestroy(IggyLibrary library)
{
  AUTO_VAR(it, g_fakeLibraries.find(library));
  if (it != g_fakeLibraries.end())
  {
    delete it->second;
    g_fakeLibraries.erase(it);
  }
}

void IggySetWarningCallback(Iggy_WarningFunction *, void *) {}

void IggySetTraceCallbackUTF8(Iggy_TraceFunctionUTF8 *, void *) {}

IggyProperties *IggyPlayerProperties(Iggy *player)
{
  FakeIggyPlayer *fake = toFakePlayer(player);
  return fake != nullptr ? &fake->properties : nullptr;
}

void *IggyPlayerGetUserdata(Iggy *player)
{
  FakeIggyPlayer *fake = toFakePlayer(player);
  return fake != nullptr ? fake->userdata : nullptr;
}

void IggyPlayerSetUserdata(Iggy *player, void *userdata)
{
  FakeIggyPlayer *fake = toFakePlayer(player);
  if (fake != nullptr)
  {
    fake->userdata = userdata;
  }
}

void IggyPlayerInitializeAndTickRS(Iggy *player)
{
  FakeIggyPlayer *fake = toFakePlayer(player);
  if (fake != nullptr)
  {
    fake->properties.frames_passed = 1;
    fake->properties.time_passed_in_seconds = fake->properties.seconds_per_drawn_frame;
    fake->properties.seconds_since_last_tick = fake->properties.seconds_per_drawn_frame;
    fake->tickPending = true;
    const auto frameDuration = std::chrono::duration<double>(fake->properties.seconds_per_drawn_frame);
    fake->nextTickTime = std::chrono::steady_clock::now() + std::chrono::duration_cast<std::chrono::steady_clock::duration>(frameDuration);
  }
}

rrbool IggyPlayerReadyToTick(Iggy *player)
{
  FakeIggyPlayer *fake = toFakePlayer(player);
  if (fake == nullptr)
  {
    return 0;
  }

  if (fake->tickPending)
  {
    return 1;
  }

  return std::chrono::steady_clock::now() >= fake->nextTickTime ? 1 : 0;
}

void IggyPlayerTickRS(Iggy *player)
{
  FakeIggyPlayer *fake = toFakePlayer(player);
  if (fake != nullptr)
  {
    ++fake->properties.frames_passed;
    fake->properties.seconds_since_last_tick = fake->properties.seconds_per_drawn_frame;
    fake->properties.time_passed_in_seconds += fake->properties.seconds_per_drawn_frame;
    const auto frameDuration = std::chrono::duration<double>(fake->properties.seconds_per_drawn_frame);
    fake->tickPending = false;
    fake->nextTickTime = std::chrono::steady_clock::now() + std::chrono::duration_cast<std::chrono::steady_clock::duration>(frameDuration);
  }
}

void IggyPlayerSetDisplaySize(Iggy *player, S32 width, S32 height)
{
  FakeIggyPlayer *fake = toFakePlayer(player);
  if (fake != nullptr && width > 0 && height > 0)
  {
    fake->properties.movie_width_in_pixels = width;
    fake->properties.movie_height_in_pixels = height;
  }
}

// Get the UIScene that owns this Iggy player (stored via IggyPlayerSetUserdata)
static UIScene *getSceneForPlayer(Iggy *player)
{
  void *userdata = IggyPlayerGetUserdata(player);
  return (userdata != nullptr) ? static_cast<UIScene *>(userdata) : nullptr;
}

// Matches GuiComponent::blit() — draws a textured rect from a 256x256 texture atlas.
// (x,y) = screen position, (sx,sy) = source texel, (w,h) = size in texels, s = GUI scale.
static void guiBlit(Tesselator *t, float x, float y, int sx, int sy, int w, int h, float s)
{
  const float us = 1.0f / 256.0f;
  const float vs = 1.0f / 256.0f;
  const float fx = x * s, fy = y * s;
  const float fw = (float)w * s, fh = (float)h * s;
  t->begin();
  t->vertexUV(fx,      fy + fh, 0.0f, (float)(sx)     * us, (float)(sy + h) * vs);
  t->vertexUV(fx + fw, fy + fh, 0.0f, (float)(sx + w)  * us, (float)(sy + h) * vs);
  t->vertexUV(fx + fw, fy,      0.0f, (float)(sx + w)  * us, (float)(sy)     * vs);
  t->vertexUV(fx,      fy,      0.0f, (float)(sx)     * us, (float)(sy)     * vs);
  t->end();
}

// Draws a Minecraft button using the real gui/gui.png texture (Button::render style).
// yImage: 0=disabled, 1=normal, 2=hovered.  Coordinates are in game-space (pre-scale).
static void drawRealButton(Tesselator *t, Textures *textures, Font *font,
                           float x, float y, int bw, int bh, const wstring &label,
                           float s, int yImage)
{
  glEnable(GL_TEXTURE_2D);
  glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
  glBindTexture(GL_TEXTURE_2D, textures->loadTexture(TN_GUI_GUI));
  // Left half then right half — exactly like Button::render
  guiBlit(t, x, y, 0, 46 + yImage * 20, bw / 2, bh, s);
  guiBlit(t, x + (float)(bw / 2), y, 200 - bw / 2, 46 + yImage * 20, bw / 2, bh, s);
  // Centered label
  glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
  const int textColor = (yImage == 2) ? 0xffffa0 : 0xe0e0e0;
  const float lx = x * s + ((float)bw * s - (float)font->width(label) * s) * 0.5f;
  const float ly = y * s + ((float)bh * s - 8.0f * s) * 0.5f;
  glPushMatrix();
  glTranslatef(lx, ly, 0.0f);
  glScalef(s, s, 1.0f);
  font->drawShadow(label, 0, 0, textColor);
  glPopMatrix();
}

// Matches Screen::renderDirtBackground() — tiles gui/background.png with gray overlay.
static void drawDirtBackground(Tesselator *t, Textures *textures, float w, float h)
{
  glDisable(GL_LIGHTING);
  glDisable(GL_FOG);
  glEnable(GL_TEXTURE_2D);
  glBindTexture(GL_TEXTURE_2D, textures->loadTexture(TN_GUI_BACKGROUND));
  glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
  const float ts = 32.0f;
  t->begin();
  t->color(0x40, 0x40, 0x40, 0xFF);
  t->vertexUV(0.0f, h,    0.0f, 0.0f,    h / ts);
  t->vertexUV(w,    h,    0.0f, w / ts,   h / ts);
  t->vertexUV(w,    0.0f, 0.0f, w / ts,   0.0f);
  t->vertexUV(0.0f, 0.0f, 0.0f, 0.0f,    0.0f);
  t->end();
}

// ---- PNG texture loading helper (ARGB → RGBA, upload to GL) ----
static int loadPngToTexture(const char *path, int *outW, int *outH)
{
  D3DXIMAGE_INFO info;
  ZeroMemory(&info, sizeof(info));
  int *pixelData = nullptr;
  HRESULT hr = RenderManager.LoadTextureData(path, &info, &pixelData);
  if (hr != ERROR_SUCCESS || pixelData == nullptr || info.Width == 0 || info.Height == 0)
  {
    std::fprintf(stderr, "[MCE] Failed to load texture: %s\n", path);
    return -1;
  }
  int texId = glGenTextures();
  glBindTexture(GL_TEXTURE_2D, texId);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
  const int pixelCount = (int)(info.Width * info.Height);
  unsigned char *rgba = new unsigned char[pixelCount * 4];
  for (int i = 0; i < pixelCount; i++)
  {
    int px = pixelData[i];
    rgba[i * 4 + 0] = (unsigned char)((px >> 16) & 0xFF);
    rgba[i * 4 + 1] = (unsigned char)((px >>  8) & 0xFF);
    rgba[i * 4 + 2] = (unsigned char)((px      ) & 0xFF);
    rgba[i * 4 + 3] = (unsigned char)((px >> 24) & 0xFF);
  }
  RenderManager.TextureData((int)info.Width, (int)info.Height, rgba, 0, C4JRender::TEXTURE_FORMAT_RxGyBzAw);
  delete[] rgba;
  delete[] pixelData;
  *outW = (int)info.Width;
  *outH = (int)info.Height;
  std::fprintf(stderr, "[MCE] Loaded texture %s: %dx%d (id=%d)\n", path, *outW, *outH, texId);
  return texId;
}

// ---- Save icon textures (chest + arrow from Media/Graphics) ----
static int  g_saveChestTexId = -1, g_saveChestW = 0, g_saveChestH = 0;
static int  g_saveArrowTexId = -1, g_saveArrowW = 0, g_saveArrowH = 0;
static bool g_saveTexLoaded  = false;

static void loadSaveTextures()
{
  if (g_saveTexLoaded) return;
  g_saveTexLoaded = true;
  g_saveChestTexId = loadPngToTexture("Common/Media/Graphics/SaveChest.png", &g_saveChestW, &g_saveChestH);
  g_saveArrowTexId = loadPngToTexture("Common/Media/Graphics/SaveArrow.png", &g_saveArrowW, &g_saveArrowH);
}

// Helper: draw a filled + bordered rect in game-space coords (scaled by s).
static void drawDialogBox(Tesselator *t, float x, float y, float bw, float bh, float s,
                          float fillR, float fillG, float fillB, float fillA,
                          float bordR, float bordG, float bordB)
{
  glDisable(GL_TEXTURE_2D);
  // Fill
  glColor4f(fillR, fillG, fillB, fillA);
  t->begin();
  t->vertex(x * s, (y + bh) * s, 0.0f);
  t->vertex((x + bw) * s, (y + bh) * s, 0.0f);
  t->vertex((x + bw) * s, y * s, 0.0f);
  t->vertex(x * s, y * s, 0.0f);
  t->end();
  // Border (1 game-px on each side)
  const float b = 1.0f;
  glColor4f(bordR, bordG, bordB, 1.0f);
  // top
  t->begin();
  t->vertex((x - b) * s, y * s, 0.0f);       t->vertex((x + bw + b) * s, y * s, 0.0f);
  t->vertex((x + bw + b) * s, (y - b) * s, 0.0f); t->vertex((x - b) * s, (y - b) * s, 0.0f);
  t->end();
  // bottom
  t->begin();
  t->vertex((x - b) * s, (y + bh + b) * s, 0.0f);       t->vertex((x + bw + b) * s, (y + bh + b) * s, 0.0f);
  t->vertex((x + bw + b) * s, (y + bh) * s, 0.0f); t->vertex((x - b) * s, (y + bh) * s, 0.0f);
  t->end();
  // left
  t->begin();
  t->vertex((x - b) * s, (y + bh) * s, 0.0f); t->vertex(x * s, (y + bh) * s, 0.0f);
  t->vertex(x * s, y * s, 0.0f);               t->vertex((x - b) * s, y * s, 0.0f);
  t->end();
  // right
  t->begin();
  t->vertex((x + bw) * s, (y + bh) * s, 0.0f);       t->vertex((x + bw + b) * s, (y + bh) * s, 0.0f);
  t->vertex((x + bw + b) * s, y * s, 0.0f); t->vertex((x + bw) * s, y * s, 0.0f);
  t->end();
}

// Helper: draw a textured quad (full UV 0-1) at game-space coords.
static void drawTexQuad(Tesselator *t, int texId, float x, float y, float qw, float qh, float s)
{
  glEnable(GL_TEXTURE_2D);
  glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
  glBindTexture(GL_TEXTURE_2D, texId);
  t->begin();
  t->vertexUV(x * s,        (y + qh) * s, 0.0f, 0.0f, 1.0f);
  t->vertexUV((x + qw) * s, (y + qh) * s, 0.0f, 1.0f, 1.0f);
  t->vertexUV((x + qw) * s, y * s,        0.0f, 1.0f, 0.0f);
  t->vertexUV(x * s,        y * s,        0.0f, 0.0f, 0.0f);
  t->end();
}

enum AppleNineSliceIndex
{
  eAppleSlice_TL = 0,
  eAppleSlice_TM,
  eAppleSlice_TR,
  eAppleSlice_ML,
  eAppleSlice_MM,
  eAppleSlice_MR,
  eAppleSlice_BL,
  eAppleSlice_BM,
  eAppleSlice_BR,
  eAppleSlice_Count
};

static bool g_createWorldChromeLoaded = false;
static int g_scenePanelTex[eAppleSlice_Count];
static int g_scenePanelW[eAppleSlice_Count];
static int g_scenePanelH[eAppleSlice_Count];
static int g_sliderTrackTex = -1;
static int g_sliderTrackW = 0;
static int g_sliderTrackH = 0;
static int g_sliderButtonTex = -1;
static int g_sliderButtonW = 0;
static int g_sliderButtonH = 0;
static int g_mainMenuButtonNormTex = -1;
static int g_mainMenuButtonNormW = 0;
static int g_mainMenuButtonNormH = 0;
static int g_mainMenuButtonOverTex = -1;
static int g_mainMenuButtonOverW = 0;
static int g_mainMenuButtonOverH = 0;
static int g_tickboxNormTex = -1;
static int g_tickboxNormW = 0;
static int g_tickboxNormH = 0;
static int g_tickboxOverTex = -1;
static int g_tickboxOverW = 0;
static int g_tickboxOverH = 0;
static int g_tickTex = -1;
static int g_tickW = 0;
static int g_tickH = 0;

static void loadCreateWorldChromeTextures()
{
  if (g_createWorldChromeLoaded) return;
  g_createWorldChromeLoaded = true;

  const char *scenePanelPaths[eAppleSlice_Count] = {
    "Common/Media/Graphics/PanelsAndTabs/Panel_Top_L.png",
    "Common/Media/Graphics/PanelsAndTabs/Panel_Top_M.png",
    "Common/Media/Graphics/PanelsAndTabs/Panel_Top_R.png",
    "Common/Media/Graphics/PanelsAndTabs/Panel_Mid_L.png",
    "Common/Media/Graphics/PanelsAndTabs/Panel_Mid_M.png",
    "Common/Media/Graphics/PanelsAndTabs/Panel_Mid_R.png",
    "Common/Media/Graphics/PanelsAndTabs/Panel_Bot_L.png",
    "Common/Media/Graphics/PanelsAndTabs/Panel_Bot_M.png",
    "Common/Media/Graphics/PanelsAndTabs/Panel_Bot_R.png",
  };
  for (int i = 0; i < eAppleSlice_Count; ++i)
  {
    g_scenePanelTex[i] = loadPngToTexture(scenePanelPaths[i], &g_scenePanelW[i], &g_scenePanelH[i]);
  }

  g_sliderTrackTex = loadPngToTexture(
    "Common/Media/Graphics/Slider_Track.png",
    &g_sliderTrackW,
    &g_sliderTrackH);
  g_sliderButtonTex = loadPngToTexture(
    "Common/Media/Graphics/Slider_Button.png",
    &g_sliderButtonW,
    &g_sliderButtonH);
  g_mainMenuButtonNormTex = loadPngToTexture(
    "Common/Media/Graphics/MainMenuButton_Norm.png",
    &g_mainMenuButtonNormW,
    &g_mainMenuButtonNormH);
  g_mainMenuButtonOverTex = loadPngToTexture(
    "Common/Media/Graphics/MainMenuButton_Over.png",
    &g_mainMenuButtonOverW,
    &g_mainMenuButtonOverH);
  g_tickboxNormTex = loadPngToTexture(
    "Common/Media/Graphics/Tickbox_Norm.png",
    &g_tickboxNormW,
    &g_tickboxNormH);
  g_tickboxOverTex = loadPngToTexture(
    "Common/Media/Graphics/Tickbox_Over.png",
    &g_tickboxOverW,
    &g_tickboxOverH);
  g_tickTex = loadPngToTexture(
    "Common/Media/Graphics/Tick.png",
    &g_tickW,
    &g_tickH);
}

static void drawNineSlicePanel(Tesselator *t,
                               const int texIds[eAppleSlice_Count],
                               const int widths[eAppleSlice_Count],
                               const int heights[eAppleSlice_Count],
                               float x, float y, float w, float h, float s)
{
  if (texIds[eAppleSlice_TL] < 0 || texIds[eAppleSlice_MM] < 0) return;

  const float leftW = (float)widths[eAppleSlice_TL];
  const float rightW = (float)widths[eAppleSlice_TR];
  const float topH = (float)heights[eAppleSlice_TL];
  const float bottomH = (float)heights[eAppleSlice_BL];
  const float centerW = (w - leftW - rightW) > 1.0f ? (w - leftW - rightW) : 1.0f;
  const float centerH = (h - topH - bottomH) > 1.0f ? (h - topH - bottomH) : 1.0f;

  drawTexQuad(t, texIds[eAppleSlice_TL], x, y, leftW, topH, s);
  drawTexQuad(t, texIds[eAppleSlice_TM], x + leftW, y, centerW, topH, s);
  drawTexQuad(t, texIds[eAppleSlice_TR], x + leftW + centerW, y, rightW, topH, s);

  drawTexQuad(t, texIds[eAppleSlice_ML], x, y + topH, leftW, centerH, s);
  drawTexQuad(t, texIds[eAppleSlice_MM], x + leftW, y + topH, centerW, centerH, s);
  drawTexQuad(t, texIds[eAppleSlice_MR], x + leftW + centerW, y + topH, rightW, centerH, s);

  drawTexQuad(t, texIds[eAppleSlice_BL], x, y + topH + centerH, leftW, bottomH, s);
  drawTexQuad(t, texIds[eAppleSlice_BM], x + leftW, y + topH + centerH, centerW, bottomH, s);
  drawTexQuad(t, texIds[eAppleSlice_BR], x + leftW + centerW, y + topH + centerH, rightW, bottomH, s);
}

static void drawConsoleMenuButton(Tesselator *t, Font *font, float x, float y, float w, float h,
                                  const wstring &label, float s, bool selected)
{
  const int texId = selected ? g_mainMenuButtonOverTex : g_mainMenuButtonNormTex;
  if (texId >= 0)
  {
    drawTexQuad(t, texId, x, y, w, h, s);
  }
  else
  {
    drawDialogBox(t, x, y, w, h, s, 0.45f, 0.45f, 0.45f, 1.0f, 0.15f, 0.15f, 0.15f);
  }

  glEnable(GL_TEXTURE_2D);
  glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
  glPushMatrix();
  const float textScale = 1.12f;
  glScalef(s * textScale, s * textScale, 1.0f);
  const int textColor = selected ? 0xFFFFFF : 0xE0E0E0;
  const float textX = (x + w * 0.5f) / textScale - (float)font->width(label) * 0.5f;
  const float textY = (y + h * 0.5f) / textScale - 4.0f;
  font->drawShadow(label, (int)textX, (int)textY, textColor);
  glPopMatrix();
}

static int loadCELogoTexture();  // forward decl — defined below panorama code
static void drawPanoramaBackground(Tesselator *t, Textures *textures, float w, float h);

static void drawSaveMessageScene(Tesselator *t, Textures *textures, Font *font, float w, float h)
{
  const float s = floorf(h / 270.0f);
  const float gw = w / s, gh = h / s;

  if (textures == nullptr || font == nullptr) return;

  drawPanoramaBackground(t, textures, w, h);
  loadCreateWorldChromeTextures();
  loadSaveTextures();

  // Recreate the original 640x480 XUI canvas at a uniform scale so the
  // save panel occupies the same screen proportion as the console scene.
  const float layoutScale = (gw / 640.0f) < (gh / 480.0f) ? (gw / 640.0f) : (gh / 480.0f);
  const float layoutX = (gw - 640.0f * layoutScale) * 0.5f;
  const float layoutY = (gh - 480.0f * layoutScale) * 0.5f;
  const float panelW = 400.0f * layoutScale;
  const float panelH = 280.0f * layoutScale;
  const float panelX = layoutX + 119.0f * layoutScale;
  const float panelY = layoutY + 130.0f * layoutScale;

  if (g_scenePanelTex[eAppleSlice_TL] >= 0 && g_scenePanelTex[eAppleSlice_MM] >= 0)
  {
    drawNineSlicePanel(t, g_scenePanelTex, g_scenePanelW, g_scenePanelH, panelX, panelY, panelW, panelH, s);
  }
  else
  {
    drawDialogBox(t, panelX, panelY, panelW, panelH, s, 0.78f, 0.78f, 0.78f, 1.0f, 0.22f, 0.22f, 0.22f);
  }

  const float iconW = 48.0f * layoutScale;
  const float iconH = 73.0f * layoutScale;
  const float iconX = panelX + 176.0f * layoutScale;
  const float iconY = panelY + 18.0f * layoutScale;
  const float iconCX = iconX + iconW * 0.5f;

  auto now = std::chrono::steady_clock::now();
  static auto startTime = now;
  double elapsed = (double)std::chrono::duration_cast<std::chrono::milliseconds>(now - startTime).count();
  float arrowBob = (float)std::sin(elapsed / 240.0) * (3.0f * layoutScale);

  if (g_saveArrowTexId >= 0)
  {
    const float arrowSz = 16.0f * layoutScale;
    drawTexQuad(t, g_saveArrowTexId, iconCX - arrowSz * 0.5f, iconY + arrowBob, arrowSz, arrowSz, s);
  }
  if (g_saveChestTexId >= 0)
  {
    const float chestSz = 24.0f * layoutScale;
    drawTexQuad(t, g_saveChestTexId, iconCX - chestSz * 0.5f, iconY + 24.0f * layoutScale, chestSz, chestSz, s);
  }

  const float textX = panelX + 25.0f * layoutScale;
  const float textY = panelY + 100.0f * layoutScale;
  const int textW = (int)(352.0f * layoutScale);
  {
    const wstring body =
      L"This game has a level autosave feature. "
      L"When you see the icon above displayed, "
      L"the game is saving your data. "
      L"\nPlease do not turn off your console while this icon is on-screen.";
    glEnable(GL_TEXTURE_2D);
    glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
    glPushMatrix();
    glTranslatef(textX * s, textY * s, 0.0f);
    glScalef(s, s, 1.0f);
    font->drawWordWrap(body, 0, 0, textW, 0x404040, 9999);
    glPopMatrix();
  }

  const float btnX = panelX + 50.0f * layoutScale;
  const float btnY = panelY + 218.0f * layoutScale;
  const float btnW = 300.0f * layoutScale;
  const float btnH = 36.0f * layoutScale;

  bool hovered = isMouseOverGameRect(btnX, btnY, btnW, btnH, s, w, h);
  if (hovered)
  {
    g_appleHoveredControlId = 0;  // eControl_Confirm
    g_appleHoveredScene = (int)eUIScene_SaveMessage;
  }
  drawConsoleMenuButton(t, font, btnX, btnY, btnW, btnH, L"OK", s, hovered);

  glDisable(GL_TEXTURE_2D);
  glDisable(GL_BLEND);
}

// ---------------------------------------------------------------------------
// Panorama background — renders the scrolling panorama PNGs like Xbox 360.
// Two copies of the panorama strip scroll left continuously.
// ---------------------------------------------------------------------------
static int  g_panoramaTexId = -1;   // cached GL texture id
static int  g_panoramaTexW  = 0;
static int  g_panoramaTexH  = 0;
static bool g_panoramaLoadAttempted = false;
static std::chrono::steady_clock::time_point g_panoramaStartTime;
static bool g_panoramaTimerStarted = false;

static int loadPanoramaTexture()
{
  if (g_panoramaLoadAttempted) return g_panoramaTexId;
  g_panoramaLoadAttempted = true;
  g_panoramaTexId = loadPngToTexture("Common/Media/Graphics/Panorama_Background_S.png",
                                     &g_panoramaTexW, &g_panoramaTexH);
  return g_panoramaTexId;
}

static void drawPanoramaBackground(Tesselator *t, Textures *textures, float w, float h)
{
  int texId = loadPanoramaTexture();
  if (texId < 0)
  {
    // Fallback to dirt if panorama PNG failed to load
    if (textures != nullptr) drawDirtBackground(t, textures, w, h);
    return;
  }

  // Start the scroll timer on first draw
  if (!g_panoramaTimerStarted)
  {
    g_panoramaStartTime = std::chrono::steady_clock::now();
    g_panoramaTimerStarted = true;
  }

  // Calculate scroll offset — full cycle every 120 seconds (slow gentle scroll like console)
  const double cycleDurationMs = 120000.0;
  auto now = std::chrono::steady_clock::now();
  double elapsedMs = (double)std::chrono::duration_cast<std::chrono::milliseconds>(
                       now - g_panoramaStartTime).count();
  double scrollFraction = std::fmod(elapsedMs, cycleDurationMs) / cycleDurationMs;

  // The panorama strip is 820x144, stretched to fill the screen height.
  glDisable(GL_LIGHTING);
  glDisable(GL_FOG);
  glEnable(GL_TEXTURE_2D);
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  glBindTexture(GL_TEXTURE_2D, texId);
  glColor4f(1.0f, 1.0f, 1.0f, 1.0f);

  // Xbox 360 approach: two copies of the panorama strip scroll left.
  // The strip is 820x144, scaled to fill screen height. At 5:1 aspect ratio,
  // one strip is ~5.69x screen height wide. Two copies placed side-by-side
  // scroll continuously.
  // One strip fills (texW/texH * h) screen pixels wide when stretched to screen height
  float stripW = ((float)g_panoramaTexW / (float)g_panoramaTexH) * h;

  // Scroll offset: moves from 0 to stripW over one cycle, then wraps
  float scrollOffset = (float)(scrollFraction * (double)stripW);

  // Draw two copies of the strip to ensure seamless coverage
  for (int copy = -1; copy <= 1; copy++)
  {
    float x0 = (float)copy * stripW - scrollOffset;
    float x1 = x0 + stripW;

    // Skip if entirely off-screen
    if (x1 < 0.0f || x0 > w) continue;

    t->begin();
    t->vertexUV(x0, h,    0.0f, 0.0f, 1.0f);
    t->vertexUV(x1, h,    0.0f, 1.0f, 1.0f);
    t->vertexUV(x1, 0.0f, 0.0f, 1.0f, 0.0f);
    t->vertexUV(x0, 0.0f, 0.0f, 0.0f, 0.0f);
    t->end();
  }

  // Slight darkening overlay to match console look (panorama is slightly dimmed behind UI)
  glDisable(GL_TEXTURE_2D);
  glColor4f(0.0f, 0.0f, 0.0f, 0.25f);
  t->begin();
  t->vertex(0.0f, h,    0.0f);
  t->vertex(w,    h,    0.0f);
  t->vertex(w,    0.0f, 0.0f);
  t->vertex(0.0f, 0.0f, 0.0f);
  t->end();
}

// ---------------------------------------------------------------------------
// CE logo — loaded once from title/mc-logo-ce.png (666x375 RGBA)
// ---------------------------------------------------------------------------
static int  g_ceLogoTexId = -1;
static int  g_ceLogoTexW  = 0;
static int  g_ceLogoTexH  = 0;
static bool g_ceLogoLoadAttempted = false;

static int loadCELogoTexture()
{
  if (g_ceLogoLoadAttempted) return g_ceLogoTexId;
  g_ceLogoLoadAttempted = true;
  g_ceLogoTexId = loadPngToTexture("Common/res/1_2_2/title/mc-logo-ce.png",
                                   &g_ceLogoTexW, &g_ceLogoTexH);
  return g_ceLogoTexId;
}

// ---------------------------------------------------------------------------
// Splash text — loaded once from splashes.txt, displayed on the main menu
// with -17° rotation and pulsing scale (matching customDrawSplash).
// ---------------------------------------------------------------------------
static wstring g_splashText;
static bool    g_splashLoaded = false;

static void ensureSplashLoaded()
{
  if (g_splashLoaded) return;
  g_splashLoaded = true;

  wstring filename = L"splashes.txt";
  if (!app.hasArchiveFile(filename)) return;

  byteArray splashesArray = app.getArchiveFile(filename);
  // Parse lines
  std::vector<wstring> splashes;
  wstring line;
  for (unsigned int i = 0; i < splashesArray.length; i++)
  {
    char ch = (char)((unsigned char *)splashesArray.data)[i];
    if (ch == '\n' || ch == '\r')
    {
      if (!line.empty()) splashes.push_back(line);
      line.clear();
    }
    else
    {
      line += (wchar_t)ch;
    }
  }
  if (!line.empty()) splashes.push_back(line);

  if (splashes.size() > 5)
  {
    // Skip first 5 (special date entries), pick a random one
    int idx = 5 + (int)(std::rand() % (splashes.size() - 5));
    g_splashText = splashes[idx];
  }
  else if (!splashes.empty())
  {
    g_splashText = splashes[0];
  }
}

static bool g_wasShowingLoadingScreen = false;

static void drawMainMenuScene(Tesselator *t, Textures *textures, Font *font, float w, float h)
{
  // Don't draw main menu while the loading screen is/was active
  if (g_wasShowingLoadingScreen || app.GetGameStarted()) return;

  const float s = floorf(h / 270.0f);
  const float gw = w / s, gh = h / s;

  if (font == nullptr) return;

  // ---- CE logo (mc-logo-ce.png, 666x375, single image) ----
  const float logoAspect = 666.0f / 375.0f;
  const float logoGameH = 192.0f;  // 20% larger (160 * 1.2)
  const float logoGameW = logoGameH * logoAspect;
  const float logoX = (gw - logoGameW) * 0.5f;
  const float logoY = -10.0f;

  int ceLogoId = loadCELogoTexture();
  if (ceLogoId >= 0)
  {
    glEnable(GL_TEXTURE_2D);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
    glBindTexture(GL_TEXTURE_2D, ceLogoId);
    float lx = logoX * s, ly = logoY * s;
    float lw = logoGameW * s, lh = logoGameH * s;
    t->begin();
    t->vertexUV(lx,      ly + lh, 0.0f, 0.0f, 1.0f);
    t->vertexUV(lx + lw, ly + lh, 0.0f, 1.0f, 1.0f);
    t->vertexUV(lx + lw, ly,      0.0f, 1.0f, 0.0f);
    t->vertexUV(lx,      ly,      0.0f, 0.0f, 0.0f);
    t->end();
  }
  else if (textures != nullptr)
  {
    int logoTexId = textures->loadTexture(TN_TITLE_MCLOGO);
    if (logoTexId > 0)
    {
      glEnable(GL_TEXTURE_2D);
      glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
      glBindTexture(GL_TEXTURE_2D, logoTexId);
      guiBlit(t, logoX, logoY, 0, 0, 155, 44, s);
      guiBlit(t, logoX + 155.0f, logoY, 0, 45, 155, 44, s);
    }
  }

  if (textures != nullptr)
  {
    // ---- Splash text (yellow, rotated -17°, pulsing) ----
    ensureSplashLoaded();
    if (!g_splashText.empty())
    {
      glEnable(GL_TEXTURE_2D);
      glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
      glPushMatrix();
      float anchorX = (logoX + logoGameW) * s;
      float anchorY = (logoY + logoGameH * 0.6f) * s;
      glTranslatef(anchorX, anchorY, 0.0f);
      glRotatef(-17.0f, 0.0f, 0.0f, 1.0f);
      float sss = 1.8f - std::abs(std::sin(
        (float)(System::currentTimeMillis() % 1000) / 1000.0f * (float)PI * 2.0f) * 0.1f);
      sss *= s;
      sss = sss * 100.0f / (float)(font->width(g_splashText) + 8 * 4);
      glScalef(sss, sss, 1.0f);
      font->drawShadow(g_splashText, 0 - font->width(g_splashText) / 2, -8, 0xFFFF00);
      glPopMatrix();
    }
  }

  // ---- Menu buttons with mouse hover + keyboard selection ----
  if (textures != nullptr)
  {
    const int btnW = 200, btnH = 20;
    const float btnX = (gw - (float)btnW) * 0.5f;
    const float startY = gh * 0.42f;
    const float spacing = 24.0f;

    // Button labels and their control IDs (matching UIScene_MainMenu::EControls)
    static const int NUM_BUTTONS = 6;
    static const wchar_t *menuLabels[] = {
      L"Play Game",        // eControl_PlayGame = 0
      L"Leaderboards",     // eControl_Leaderboards = 1
      L"Achievements",     // eControl_Achievements = 2
      L"Help & Options",   // eControl_HelpAndOptions = 3
      L"Minecraft Store",  // eControl_UnlockOrDLC = 4
      L"Exit Game",        // eControl_Exit = 5
    };
    static const int controlIds[] = { 0, 1, 2, 3, 4, 5 };

    // Check mouse hover over each button
    int mouseHoverIdx = -1;
    for (int i = 0; i < NUM_BUTTONS; ++i)
    {
      const float by = startY + (float)i * spacing;
      if (isMouseOverGameRect(btnX, by, (float)btnW, (float)btnH, s, w, h))
      {
        mouseHoverIdx = i;
        break;
      }
    }

    // Mouse hover overrides keyboard selection for visual highlight
    if (mouseHoverIdx >= 0)
    {
      g_mainMenuSelectedIdx = mouseHoverIdx;
      if (controlIds[mouseHoverIdx] >= 0)
      {
        g_appleHoveredControlId = controlIds[mouseHoverIdx];
        g_appleHoveredScene = (int)eUIScene_MainMenu;
      }
    }

    // Draw buttons
    for (int i = 0; i < NUM_BUTTONS; ++i)
    {
      const float by = startY + (float)i * spacing;
      int yImg = 1;  // normal
      if (i == g_mainMenuSelectedIdx) yImg = 2;  // hovered/selected
      drawRealButton(t, textures, font, btnX, by, btnW, btnH,
                     wstring(menuLabels[i]), s, yImg);
    }
  }

  // Version string at bottom-left
  glEnable(GL_TEXTURE_2D);
  glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
  glPushMatrix();
  glScalef(s, s, 1.0f);
  font->drawShadow(L"Minecraft Community Edition", 2, (int)(gh - 10.0f), 0x808080);
  glPopMatrix();
  glDisable(GL_TEXTURE_2D);
}

static void drawMenuPanelTitle(Font *font, const wstring &title, float centerX, float y, float s)
{
  if (font == nullptr)
  {
    return;
  }

  glEnable(GL_TEXTURE_2D);
  glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
  glPushMatrix();
  glTranslatef(centerX * s, y * s, 0.0f);
  glScalef(s, s, 1.0f);
  font->drawShadow(title, -font->width(title) / 2, 0, 0xFFFFFF);
  glPopMatrix();
}

static void drawButtonMenuChrome(Tesselator *t, Font *font, float gw, float gh, float s,
                                 const wstring &title, int rowCount,
                                 float &panelX, float &panelY, float &panelW, float &panelH)
{
  panelW = 300.0f;
  panelH = 44.0f + (float)rowCount * 24.0f + 14.0f;
  panelX = (gw - panelW) * 0.5f;
  panelY = (gh - panelH) * 0.5f + 16.0f;

  glDisable(GL_TEXTURE_2D);
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  drawDialogBox(t, panelX, panelY, panelW, panelH, s, 0.10f, 0.10f, 0.10f, 0.82f, 0.40f, 0.40f, 0.40f);
  drawMenuPanelTitle(font, title, panelX + panelW * 0.5f, panelY + 12.0f, s);
}

static void drawHelpAndOptionsScene(Tesselator *t, Textures *textures, Font *font, float w, float h, UIScene_HelpAndOptionsMenu *scene)
{
  if (textures == nullptr || font == nullptr || scene == nullptr)
  {
    return;
  }

  const float s = floorf(h / 270.0f);
  const float gw = w / s;
  const float gh = h / s;
  const int buttonCount = scene->appleGetVisibleButtonCount();

  float panelX = 0.0f, panelY = 0.0f, panelW = 0.0f, panelH = 0.0f;
  drawButtonMenuChrome(t, font, gw, gh, s, app.GetString(IDS_HELP_AND_OPTIONS), buttonCount, panelX, panelY, panelW, panelH);

  const float btnX = panelX + 18.0f;
  const float btnY = panelY + 28.0f;
  const int btnW = (int)(panelW - 36.0f);
  const int btnH = 20;

  int hoverIndex = -1;
  for (int i = 0; i < buttonCount; ++i)
  {
    if (isMouseOverGameRect(btnX, btnY + (float)i * 24.0f, (float)btnW, (float)btnH, s, w, h))
    {
      hoverIndex = i;
      break;
    }
  }

  if (hoverIndex >= 0)
  {
    scene->appleSetSelectedIndex(hoverIndex);
    g_appleHoveredControlId = scene->appleGetControlIdForVisibleButton(hoverIndex);
    g_appleHoveredScene = (int)eUIScene_HelpAndOptionsMenu;
  }

  for (int i = 0; i < buttonCount; ++i)
  {
    int yImage = i == scene->appleGetSelectedIndex() ? 2 : 1;
    drawRealButton(t, textures, font, btnX, btnY + (float)i * 24.0f, btnW, btnH,
                   wstring(scene->appleGetLabelForVisibleButton(i)), s, yImage);
  }
}

static void drawSettingsMenuScene(Tesselator *t, Textures *textures, Font *font, float w, float h, UIScene_SettingsMenu *scene)
{
  if (textures == nullptr || font == nullptr || scene == nullptr)
  {
    return;
  }

  const float s = floorf(h / 270.0f);
  const float gw = w / s;
  const float gh = h / s;
  const int buttonCount = scene->appleGetVisibleButtonCount();

  float panelX = 0.0f, panelY = 0.0f, panelW = 0.0f, panelH = 0.0f;
  drawButtonMenuChrome(t, font, gw, gh, s, app.GetString(IDS_SETTINGS), buttonCount, panelX, panelY, panelW, panelH);

  const float btnX = panelX + 18.0f;
  const float btnY = panelY + 28.0f;
  const int btnW = (int)(panelW - 36.0f);
  const int btnH = 20;

  int hoverIndex = -1;
  for (int i = 0; i < buttonCount; ++i)
  {
    if (isMouseOverGameRect(btnX, btnY + (float)i * 24.0f, (float)btnW, (float)btnH, s, w, h))
    {
      hoverIndex = i;
      break;
    }
  }

  if (hoverIndex >= 0)
  {
    scene->appleSetSelectedIndex(hoverIndex);
    g_appleHoveredControlId = scene->appleGetControlIdForVisibleButton(hoverIndex);
    g_appleHoveredScene = (int)eUIScene_SettingsMenu;
  }

  for (int i = 0; i < buttonCount; ++i)
  {
    int yImage = i == scene->appleGetSelectedIndex() ? 2 : 1;
    drawRealButton(t, textures, font, btnX, btnY + (float)i * 24.0f, btnW, btnH,
                   wstring(scene->appleGetLabelForVisibleButton(i)), s, yImage);
  }
}

static void drawSettingsOptionsScene(Tesselator *t, Textures *textures, Font *font, float w, float h, UIScene_SettingsOptionsMenu *scene)
{
  if (textures == nullptr || font == nullptr || scene == nullptr)
  {
    return;
  }

  const float s = floorf(h / 270.0f);
  const float gw = w / s;
  const float gh = h / s;
  const int rowCount = scene->appleGetVisibleRowCount();

  float panelX = 0.0f;
  float panelY = 0.0f;
  float panelW = 360.0f;
  float panelH = 44.0f + (float)rowCount * 24.0f + 14.0f;
  panelX = (gw - panelW) * 0.5f;
  panelY = (gh - panelH) * 0.5f + 12.0f;

  glDisable(GL_TEXTURE_2D);
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  drawDialogBox(t, panelX, panelY, panelW, panelH, s, 0.10f, 0.10f, 0.10f, 0.82f, 0.40f, 0.40f, 0.40f);
  drawMenuPanelTitle(font, app.GetString(IDS_OPTIONS), panelX + panelW * 0.5f, panelY + 12.0f, s);

  const float rowX = panelX + 18.0f;
  const float rowY = panelY + 28.0f;
  const int rowW = (int)(panelW - 36.0f);
  const int rowH = 20;

  int hoverIndex = -1;
  for (int i = 0; i < rowCount; ++i)
  {
    if (isMouseOverGameRect(rowX, rowY + (float)i * 24.0f, (float)rowW, (float)rowH, s, w, h))
    {
      hoverIndex = i;
      break;
    }
  }

  if (hoverIndex >= 0)
  {
    scene->appleSetSelectedIndex(hoverIndex);
    g_appleHoveredControlId = scene->appleGetControlIdForVisibleRow(hoverIndex);
    g_appleHoveredScene = (int)eUIScene_SettingsOptionsMenu;
  }

  for (int i = 0; i < rowCount; ++i)
  {
    int controlId = scene->appleGetControlIdForVisibleRow(i);
    wstring label = scene->appleGetLabelForVisibleRow(i);
    if (scene->appleIsCheckboxControl(controlId))
    {
      label += scene->appleGetCheckboxValue(controlId) ? L": On" : L": Off";
    }

    int yImage = i == scene->appleGetSelectedIndex() ? 2 : 1;
    drawRealButton(t, textures, font, rowX, rowY + (float)i * 24.0f, rowW, rowH, label, s, yImage);
  }
}

// ---- SettingsGraphicsMenu scene (Settings -> Graphics) ----
static void drawSettingsGraphicsScene(Tesselator *t, Textures *textures, Font *font, float w, float h, UIScene_SettingsGraphicsMenu *scene)
{
  if (textures == nullptr || font == nullptr || scene == nullptr)
  {
    return;
  }

  const float s = floorf(h / 270.0f);
  const float gw = w / s;
  const float gh = h / s;
  const int rowCount = scene->appleGetVisibleRowCount();

  float panelX = 0.0f;
  float panelY = 0.0f;
  float panelW = 360.0f;
  float panelH = 44.0f + (float)rowCount * 24.0f + 14.0f;
  panelX = (gw - panelW) * 0.5f;
  panelY = (gh - panelH) * 0.5f + 12.0f;

  glDisable(GL_TEXTURE_2D);
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  drawDialogBox(t, panelX, panelY, panelW, panelH, s, 0.10f, 0.10f, 0.10f, 0.82f, 0.40f, 0.40f, 0.40f);
  drawMenuPanelTitle(font, app.GetString(IDS_GRAPHICS), panelX + panelW * 0.5f, panelY + 12.0f, s);

  const float rowX = panelX + 18.0f;
  const float rowY = panelY + 28.0f;
  const int rowW = (int)(panelW - 36.0f);
  const int rowH = 20;

  int hoverIndex = -1;
  for (int i = 0; i < rowCount; ++i)
  {
    if (isMouseOverGameRect(rowX, rowY + (float)i * 24.0f, (float)rowW, (float)rowH, s, w, h))
    {
      hoverIndex = i;
      break;
    }
  }

  if (hoverIndex >= 0)
  {
    scene->appleSetSelectedIndex(hoverIndex);
    g_appleHoveredControlId = scene->appleGetControlIdForVisibleRow(hoverIndex);
    g_appleHoveredScene = (int)eUIScene_SettingsGraphicsMenu;
  }

  for (int i = 0; i < rowCount; ++i)
  {
    wstring label = scene->appleGetLabelForVisibleRow(i);
    int yImage = i == scene->appleGetSelectedIndex() ? 2 : 1;
    drawRealButton(t, textures, font, rowX, rowY + (float)i * 24.0f, rowW, rowH, label, s, yImage);
  }
}

// ---- SettingsAudioMenu scene ----
static void drawSettingsAudioScene(Tesselator *t, Textures *textures, Font *font, float w, float h, UIScene_SettingsAudioMenu *scene)
{
  if (textures == nullptr || font == nullptr || scene == nullptr) return;

  const float s = floorf(h / 270.0f);
  const float gw = w / s;
  const float gh = h / s;
  const int rowCount = scene->appleGetVisibleRowCount();

  float panelW = 360.0f;
  float panelH = 44.0f + (float)rowCount * 24.0f + 14.0f;
  float panelX = (gw - panelW) * 0.5f;
  float panelY = (gh - panelH) * 0.5f + 12.0f;

  glDisable(GL_TEXTURE_2D);
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  drawDialogBox(t, panelX, panelY, panelW, panelH, s, 0.10f, 0.10f, 0.10f, 0.82f, 0.40f, 0.40f, 0.40f);
  drawMenuPanelTitle(font, L"Audio", panelX + panelW * 0.5f, panelY + 12.0f, s);

  const float rowX = panelX + 18.0f;
  const float rowY = panelY + 28.0f;
  const int rowW = (int)(panelW - 36.0f);
  const int rowH = 20;

  int hoverIndex = -1;
  for (int i = 0; i < rowCount; ++i)
  {
    if (isMouseOverGameRect(rowX, rowY + (float)i * 24.0f, (float)rowW, (float)rowH, s, w, h))
    {
      hoverIndex = i;
      break;
    }
  }

  if (hoverIndex >= 0)
  {
    scene->appleSetSelectedIndex(hoverIndex);
    g_appleHoveredControlId = scene->appleGetControlIdForVisibleRow(hoverIndex);
    g_appleHoveredScene = (int)eUIScene_SettingsAudioMenu;
  }

  for (int i = 0; i < rowCount; ++i)
  {
    wstring label = scene->appleGetLabelForVisibleRow(i);
    int yImage = i == scene->appleGetSelectedIndex() ? 2 : 1;
    drawRealButton(t, textures, font, rowX, rowY + (float)i * 24.0f, rowW, rowH, label, s, yImage);
  }
}

// ---- LoadOrJoinMenu scene (Play Game -> world list) ----
static void drawLoadOrJoinScene(Tesselator *t, Textures *textures, Font *font, float w, float h)
{
  const float s = floorf(h / 270.0f);
  const float gw = w / s, gh = h / s;
  if (textures == nullptr || font == nullptr) return;

  drawDirtBackground(t, textures, w, h);

  glEnable(GL_TEXTURE_2D);
  glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
  glPushMatrix();
  glScalef(s, s, 1.0f);
  const wstring title = L"Play Game";
  font->drawShadow(title, (int)((gw - (float)font->width(title)) * 0.5f), 20, 0xFFFFFF);
  glPopMatrix();

  const int btnW = 200, btnH = 20;
  const float btnX = (gw - (float)btnW) * 0.5f;
  const float btnY = gh * 0.4f;

  if (isMouseOverGameRect(btnX, btnY, (float)btnW, (float)btnH, s, w, h))
  {
    g_loadOrJoinSelectedIdx = 0;
    g_appleHoveredControlId = 0;
    g_appleHoveredScene = (int)eUIScene_LoadOrJoinMenu;
  }

  drawRealButton(t, textures, font, btnX, btnY, btnW, btnH, L"Create New World", s, 2);

  glEnable(GL_TEXTURE_2D);
  glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
  glPushMatrix();
  glScalef(s, s, 1.0f);
  font->drawShadow(L"Esc: Back", 2, (int)(gh - 10.0f), 0x808080);
  glPopMatrix();
  glDisable(GL_TEXTURE_2D);
}

// ---- CreateWorldMenu scene ----
static void drawCreateWorldScene(Tesselator *t, Textures *textures, Font *font, float w, float h, UIScene_CreateWorldMenu *scene)
{
  const float s = floorf(h / 270.0f);
  const float gw = w / s, gh = h / s;
  if (textures == nullptr || font == nullptr) return;
  drawPanoramaBackground(t, textures, w, h);
  loadCreateWorldChromeTextures();

  const wstring worldName = (scene != nullptr && !scene->appleGetWorldName().empty())
    ? scene->appleGetWorldName()
    : L"New World";
  const wstring seedValue = (scene != nullptr && !scene->appleGetSeed().empty())
    ? scene->appleGetSeed()
    : L"";
  const bool isSurvival = scene != nullptr ? scene->appleIsGameModeSurvival() : g_createWorldSurvival;
  const wstring difficultyText = scene != nullptr ? scene->appleGetDifficultyText() : L"Normal";
  const bool isOnlineGame = scene != nullptr && scene->appleIsOnlineGame();
  const bool isInviteOnly = scene != nullptr && scene->appleIsInviteOnly();
  const bool allowFoF = scene != nullptr && scene->appleAllowsFriendsOfFriends();
  g_createWorldSurvival = isSurvival;

  // Match the PS3 480 Create World scene proportions/spacing.
  const float basePanelW = 340.0f;
  const float basePanelH = 410.0f;
  const float maxScaleY = (gh - 42.0f) / basePanelH;
  const float maxScaleX = (gw - 70.0f) / basePanelW;
  const float layoutScale = maxScaleX < maxScaleY ? maxScaleX : maxScaleY;
  const float panelW = basePanelW * layoutScale;
  const float panelH = basePanelH * layoutScale;
  const float panelX = (gw - panelW) * 0.5f;
  const float panelY = (gh - panelH) * 0.5f;
  const float contentX = panelX + 18.0f * layoutScale;
  const float contentW = 304.0f * layoutScale;
  const float fieldX = panelX + 21.0f * layoutScale;
  const float fieldW = 298.0f * layoutScale;
  const float fieldH = 24.0f * layoutScale;
  const float buttonH = 36.0f * layoutScale;
  const float worldLabelY = panelY + 93.0f * layoutScale;
  const float worldFieldY = panelY + 111.0f * layoutScale;
  const float seedLabelY = panelY + 147.0f * layoutScale;
  const float seedFieldY = panelY + 167.0f * layoutScale;
  const float randomSeedY = panelY + 203.0f * layoutScale;
  const float modeY = panelY + 226.0f * layoutScale;
  const float sliderY = panelY + 269.0f * layoutScale;
  const float moreOptionsY = panelY + 312.0f * layoutScale;
  const float createWorldY = panelY + 357.0f * layoutScale;

  extern bool AppleInput_IsKeyboardActive();
  extern const wchar_t *AppleInput_GetKeyboardTitle();
  extern const wchar_t *AppleInput_GetKeyboardText();
  const bool keyboardActive = AppleInput_IsKeyboardActive();

  auto drawField = [&](float x, float y, float fw, const wstring &text, bool selected, bool editing)
  {
    const bool highlighted = selected || editing;

    if (highlighted)
    {
      drawDialogBox(
        t,
        x - 2.0f * layoutScale,
        y - 2.0f * layoutScale,
        fw + 4.0f * layoutScale,
        fieldH + 4.0f * layoutScale,
        s,
        0.0f,
        0.0f,
        0.0f,
        0.0f,
        0.92f,
        0.80f,
        0.06f);
    }

    // Keep the field body simple to avoid layered-box artifacts.
    drawDialogBox(
      t,
      x,
      y,
      fw,
      fieldH,
      s,
      0.40f,
      0.40f,
      0.40f,
      0.92f,
      0.20f,
      0.20f,
      0.20f);

    glEnable(GL_TEXTURE_2D);
    glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
    glPushMatrix();
    glScalef(s, s, 1.0f);
    wstring displayText = text.empty() ? L" " : text;
    if (editing)
    {
      displayText = wstring(AppleInput_GetKeyboardText()) + L"_";
    }
    const float textY = y + (fieldH - 8.0f * layoutScale) * 0.5f;
    font->drawShadow(displayText, (int)(x + 6.0f * layoutScale), (int)textY, 0xEBEBEB);
    glPopMatrix();
  };

  const float interactiveX[6] = {
    fieldX, fieldX, contentX, contentX, contentX, contentX
  };
  const float interactiveY[6] = {
    worldFieldY, seedFieldY, modeY, sliderY, moreOptionsY, createWorldY
  };
  const float interactiveW[6] = {
    fieldW, fieldW, contentW, contentW, contentW, contentW
  };
  const float interactiveH[6] = {
    fieldH, fieldH, buttonH, 38.0f * layoutScale, buttonH, buttonH
  };

  int mouseHoverIdx = -1;
  for (int i = 0; i < 6; ++i)
  {
    if (isMouseOverGameRect(interactiveX[i], interactiveY[i], interactiveW[i], interactiveH[i], s, w, h))
    {
      mouseHoverIdx = i;
      break;
    }
  }
  if (mouseHoverIdx >= 0)
  {
    g_createWorldSelectedIdx = mouseHoverIdx;
    g_appleHoveredControlId = mouseHoverIdx;
    g_appleHoveredScene = (int)eUIScene_CreateWorldMenu;
  }
  else if (g_appleHoveredScene == (int)eUIScene_CreateWorldMenu)
  {
    g_appleHoveredControlId = -1;
    g_appleHoveredScene = -1;
  }

  drawNineSlicePanel(t, g_scenePanelTex, g_scenePanelW, g_scenePanelH, panelX, panelY, panelW, panelH, s);

  glEnable(GL_TEXTURE_2D);
  glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
  glPushMatrix();
  glScalef(s, s, 1.0f);
  const wstring title = L"Create New World";
  font->drawShadow(title, (int)((gw - (float)font->width(title)) * 0.5f), (int)(panelY - 18.0f), 0xFFFFFF);

  const float checkboxX = panelX + 16.0f * layoutScale;
  const float checkboxTextX = checkboxX + 20.0f * layoutScale;
  const float checkboxStartY = panelY + 16.0f * layoutScale;
  const float checkboxSpacing = 26.0f * layoutScale;
  const float checkboxSize = 12.0f * layoutScale;
  const wchar_t *checkboxLabels[3] = {
    L"Online game",
    L"Invite only",
    L"Allow friends of friends"
  };
  const bool checkboxValues[3] = { isOnlineGame, isInviteOnly, allowFoF };
  for (int i = 0; i < 3; ++i)
  {
    font->draw(checkboxLabels[i], (int)checkboxTextX, (int)(checkboxStartY + checkboxSpacing * (float)i), 0x4A4A4A);
  }

  font->draw(L"World Name", (int)contentX, (int)worldLabelY, 0x3C3C3C);
  font->draw(L"Seed for the World Generator", (int)contentX, (int)seedLabelY, 0x3C3C3C);
  font->draw(L"Leave blank for a random seed", (int)contentX, (int)randomSeedY, 0x585858);

  glPopMatrix();

  for (int i = 0; i < 3; ++i)
  {
    const float cy = checkboxStartY + checkboxSpacing * (float)i + 1.0f;
    const int tickboxTex = g_tickboxNormTex >= 0 ? g_tickboxNormTex : -1;
    if (tickboxTex >= 0)
    {
      drawTexQuad(t, tickboxTex, checkboxX, cy, checkboxSize, checkboxSize, s);
    }
    else
    {
      drawDialogBox(t, checkboxX, cy, checkboxSize, checkboxSize, s, 0.82f, 0.82f, 0.82f, 1.0f, 0.28f, 0.28f, 0.28f);
    }
    if (checkboxValues[i])
    {
      if (g_tickTex >= 0)
      {
        drawTexQuad(t, g_tickTex, checkboxX - 3.0f * layoutScale, cy - 3.0f * layoutScale, 20.0f * layoutScale, 18.0f * layoutScale, s);
      }
    }
  }

  drawField(fieldX, worldFieldY, fieldW, worldName, g_createWorldSelectedIdx == 0,
            keyboardActive && wcscmp(AppleInput_GetKeyboardTitle(), L"Create New World") == 0);
  drawField(fieldX, seedFieldY, fieldW, seedValue, g_createWorldSelectedIdx == 1,
            keyboardActive && wcscmp(AppleInput_GetKeyboardTitle(), L"Seed for the World Generator") == 0);

  const wstring buttonLabels[3] = {
    isSurvival ? L"Game Mode: Survival" : L"Game Mode: Creative",
    L"More Options",
    L"Create New World"
  };
  drawConsoleMenuButton(t, font, contentX, modeY, contentW, buttonH, buttonLabels[0], s, g_createWorldSelectedIdx == 2);

  const wstring difficultyLabel = L"Difficulty: " + difficultyText;
  const float sliderTrackW = contentW;
  const float sliderTrackH = (g_sliderTrackH > 0 ? (float)g_sliderTrackH : 16.0f) * layoutScale;
  const float sliderTrackX = contentX;
  const float sliderTrackY = sliderY + (38.0f * layoutScale - sliderTrackH) * 0.5f;
  const float sliderButtonW = (g_sliderButtonW > 0 ? (float)g_sliderButtonW : 12.0f) * layoutScale;
  const float sliderButtonH = (g_sliderButtonH > 0 ? (float)g_sliderButtonH : 32.0f) * layoutScale;
  if (g_sliderTrackTex >= 0)
  {
    drawTexQuad(t, g_sliderTrackTex, sliderTrackX, sliderTrackY, sliderTrackW, sliderTrackH, s);
  }
  else
  {
    drawDialogBox(t, sliderTrackX, sliderTrackY, sliderTrackW, sliderTrackH, s, 0.18f, 0.18f, 0.18f, 1.0f, 0.05f, 0.05f, 0.05f);
  }

  const int difficulty = scene != nullptr ? scene->appleGetDifficulty() : 1;
  const float knobTravel = (sliderTrackW - sliderButtonW) > 1.0f ? (sliderTrackW - sliderButtonW) : 1.0f;
  const float knobX = sliderTrackX + knobTravel * ((float)difficulty / 3.0f);
  if (g_createWorldSelectedIdx == 3)
  {
    drawDialogBox(t, sliderTrackX - 3.0f, sliderTrackY - 3.0f, sliderTrackW + 6.0f, sliderTrackH + 6.0f, s, 0.0f, 0.0f, 0.0f, 0.0f, 0.92f, 0.80f, 0.06f);
  }
  if (g_sliderButtonTex >= 0)
  {
    drawTexQuad(t, g_sliderButtonTex, knobX, sliderY + (38.0f * layoutScale - sliderButtonH) * 0.5f, sliderButtonW, sliderButtonH, s);
  }
  else
  {
    drawRealButton(t, textures, font, knobX - 4.0f, sliderTrackY - 4.0f, (int)(20.0f * layoutScale), (int)(20.0f * layoutScale), L"", s, g_createWorldSelectedIdx == 3 ? 2 : 1);
  }

  drawConsoleMenuButton(t, font, contentX, moreOptionsY, contentW, buttonH, buttonLabels[1], s, g_createWorldSelectedIdx == 4);
  drawConsoleMenuButton(t, font, contentX, createWorldY, contentW, buttonH, buttonLabels[2], s, g_createWorldSelectedIdx == 5);

  glEnable(GL_TEXTURE_2D);
  glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
  glPushMatrix();
  glScalef(s, s, 1.0f);
  font->drawShadow(
    difficultyLabel,
    (int)(sliderTrackX + sliderTrackW * 0.5f - (float)font->width(difficultyLabel) * 0.5f),
    (int)(sliderY + 38.0f * layoutScale * 0.5f - 4.0f),
    0xFFFFFF);
  if (keyboardActive)
  {
    const wstring hint = L"Enter: Confirm   Esc: Cancel";
    font->drawShadow(hint, (int)((gw - (float)font->width(hint)) * 0.5f), (int)(panelY + panelH + 5.0f), 0x808080);
  }
  font->drawShadow(L"Esc: Back", 2, (int)(gh - 10.0f), 0x808080);
  glPopMatrix();
  glDisable(GL_TEXTURE_2D);
}

// ---- FullscreenProgress scene (loading screen) ----
// Matches the Console Edition loading screen: panorama bg, CE logo, title,
// progress bar with percentage, status text, and rotating game tips.

static wstring g_loadingTipText;
static std::chrono::steady_clock::time_point g_lastTipChangeTime;
static bool g_loadingTipInitialized = false;
// g_wasShowingLoadingScreen defined above drawMainMenuScene

// Strip {*...*} controller placeholders from tip strings for plain-text rendering
static wstring stripControllerPlaceholders(const wstring &text)
{
  wstring result;
  result.reserve(text.size());
  size_t i = 0;
  while (i < text.size())
  {
    if (i + 1 < text.size() && text[i] == L'{' && text[i + 1] == L'*')
    {
      // Find closing *}
      size_t end = text.find(L"*}", i + 2);
      if (end != wstring::npos)
      {
        // Extract tag name for keyboard substitution
        wstring tag = text.substr(i + 2, end - (i + 2));
        if (tag == L"CONTROLLER_VK_A") result += L"LMB";
        else if (tag == L"CONTROLLER_VK_B") result += L"Q";
        // Skip other tags silently
        i = end + 2;
        continue;
      }
    }
    result += text[i];
    i++;
  }
  return result;
}

static void drawFullscreenProgressScene(Tesselator *t, Textures *textures, Font *font, float w, float h)
{
  const float s = floorf(h / 270.0f);
  const float gw = w / s, gh = h / s;
  if (textures == nullptr || font == nullptr) return;

  // ---- Panorama background (same as title screen) ----
  drawPanoramaBackground(t, textures, w, h);

  // ---- CE Logo (smaller than main menu — ~60% size, centered near top) ----
  int ceLogoId = loadCELogoTexture();
  if (ceLogoId >= 0)
  {
    const float logoAspect = 666.0f / 375.0f;
    const float logoGameH = 115.0f;
    const float logoGameW = logoGameH * logoAspect;
    const float logoX = (gw - logoGameW) * 0.5f;
    const float logoY = 2.0f;

    glEnable(GL_TEXTURE_2D);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
    glBindTexture(GL_TEXTURE_2D, ceLogoId);
    float lx = logoX * s, ly = logoY * s;
    float lw = logoGameW * s, lh = logoGameH * s;
    t->begin();
    t->vertexUV(lx,      ly + lh, 0.0f, 0.0f, 1.0f);
    t->vertexUV(lx + lw, ly + lh, 0.0f, 1.0f, 1.0f);
    t->vertexUV(lx + lw, ly,      0.0f, 1.0f, 0.0f);
    t->vertexUV(lx,      ly,      0.0f, 0.0f, 0.0f);
    t->end();
  }

  Minecraft *mc = Minecraft::GetInstance();
  if (mc == nullptr || mc->progressRenderer == nullptr) return;

  // ---- Read progress state ----
  int percent = mc->progressRenderer->getCurrentPercent();
  if (percent < 0) percent = 0;
  if (percent > 100) percent = 100;

  int titleId = mc->progressRenderer->getCurrentTitle();
  wstring titleText = L"Loading...";
  if (titleId >= 0)
  {
    LPCWSTR str = app.GetString(titleId);
    if (str != nullptr && str[0] != L'\0') titleText = str;
  }

  wstring statusText;
  ProgressRenderer::eProgressStringType progressType = mc->progressRenderer->getType();
  if (progressType == ProgressRenderer::eProgressStringType_ID)
  {
    int statusId = mc->progressRenderer->getCurrentStatus();
    if (statusId >= 0)
    {
      LPCWSTR str = app.GetString(statusId);
      if (str != nullptr && str[0] != L'\0') statusText = str;
    }
  }
  else
  {
    statusText = mc->progressRenderer->getProgressString();
  }

  // ---- Title text (centered, above progress bar) ----
  glEnable(GL_TEXTURE_2D);
  glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
  glPushMatrix();
  glScalef(s, s, 1.0f);
  font->drawShadow(titleText,
    (int)((gw - (float)font->width(titleText)) * 0.5f),
    (int)(gh * 0.50f), 0xFFFFFF);
  glPopMatrix();

  // ---- Progress bar (centered, console-style) ----
  const float barW = 200.0f, barH = 5.0f;
  const float barX = (gw - barW) * 0.5f;
  const float barY = gh * 0.57f;

  // Bar border (dark outline)
  glDisable(GL_TEXTURE_2D);
  glColor4f(0.08f, 0.08f, 0.08f, 1.0f);
  t->begin();
  t->vertex((barX - 1.0f) * s, (barY + barH + 1.0f) * s, 0.0f);
  t->vertex((barX + barW + 1.0f) * s, (barY + barH + 1.0f) * s, 0.0f);
  t->vertex((barX + barW + 1.0f) * s, (barY - 1.0f) * s, 0.0f);
  t->vertex((barX - 1.0f) * s, (barY - 1.0f) * s, 0.0f);
  t->end();

  // Bar background (dark gray)
  glColor4f(0.15f, 0.15f, 0.15f, 1.0f);
  t->begin();
  t->vertex(barX * s, (barY + barH) * s, 0.0f);
  t->vertex((barX + barW) * s, (barY + barH) * s, 0.0f);
  t->vertex((barX + barW) * s, barY * s, 0.0f);
  t->vertex(barX * s, barY * s, 0.0f);
  t->end();

  // Bar fill (green, matching console)
  float fillW = barW * ((float)percent / 100.0f);
  if (fillW > 0.0f)
  {
    glColor4f(0.31f, 0.78f, 0.31f, 1.0f);
    t->begin();
    t->vertex(barX * s, (barY + barH) * s, 0.0f);
    t->vertex((barX + fillW) * s, (barY + barH) * s, 0.0f);
    t->vertex((barX + fillW) * s, barY * s, 0.0f);
    t->vertex(barX * s, barY * s, 0.0f);
    t->end();
  }

  // ---- Status text + percentage (below progress bar) ----
  glEnable(GL_TEXTURE_2D);
  glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
  glPushMatrix();
  glScalef(s, s, 1.0f);

  wstring belowBarText;
  if (!statusText.empty())
  {
    wchar_t pctBuf[16];
    swprintf(pctBuf, 16, L" %d%%", percent);
    belowBarText = statusText + pctBuf;
  }
  else
  {
    wchar_t pctBuf[16];
    swprintf(pctBuf, 16, L"%d%%", percent);
    belowBarText = pctBuf;
  }
  font->drawShadow(belowBarText,
    (int)((gw - (float)font->width(belowBarText)) * 0.5f),
    (int)(barY + barH + 4.0f), 0xAAAAAA);

  glPopMatrix();

  // ---- Rotating tips (every 7 seconds, like console) ----
  auto now = std::chrono::steady_clock::now();
  if (!g_loadingTipInitialized)
  {
    UINT tipId = app.GetNextTip();
    LPCWSTR tipStr = app.GetString(tipId);
    if (tipStr != nullptr && tipStr[0] != L'\0')
      g_loadingTipText = stripControllerPlaceholders(tipStr);
    else
      g_loadingTipText = L"";
    g_lastTipChangeTime = now;
    g_loadingTipInitialized = true;
  }
  else
  {
    double elapsedMs = (double)std::chrono::duration_cast<std::chrono::milliseconds>(
                         now - g_lastTipChangeTime).count();
    if (elapsedMs >= 7000.0)
    {
      UINT tipId = app.GetNextTip();
      LPCWSTR tipStr = app.GetString(tipId);
      if (tipStr != nullptr && tipStr[0] != L'\0')
        g_loadingTipText = stripControllerPlaceholders(tipStr);
      else
        g_loadingTipText = L"";
      g_lastTipChangeTime = now;
    }
  }

  if (!g_loadingTipText.empty())
  {
    glEnable(GL_TEXTURE_2D);
    glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
    glPushMatrix();
    glScalef(s, s, 1.0f);
    // Word-wrap tip text across the bottom of the screen
    int tipAreaW = (int)(gw * 0.8f);
    int tipX = (int)(gw * 0.1f);
    int tipY = (int)(gh * 0.82f);
    font->drawWordWrap(g_loadingTipText, tipX, tipY, tipAreaW, 0xFFFF55, 9999);
    glPopMatrix();
  }

  glDisable(GL_TEXTURE_2D);
}

// ---------------------------------------------------------------------------
// Pause Menu scene drawing (Apple replacement for Iggy-based PauseMenu)
// ---------------------------------------------------------------------------
static void drawPauseMenuScene(Tesselator *t, Textures *textures, Font *font, float w, float h)
{
  const float s = floorf(h / 270.0f);
  const float gw = w / s, gh = h / s;

  if (textures == nullptr || font == nullptr) return;

  // Semi-transparent dark overlay
  glDisable(GL_TEXTURE_2D);
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  t->begin();
  t->color(0x00, 0x00, 0x00, 0xB0);
  t->vertexUV(0.0f, h,    0.0f, 0.0f, 0.0f);
  t->vertexUV(w,    h,    0.0f, 0.0f, 0.0f);
  t->vertexUV(w,    0.0f, 0.0f, 0.0f, 0.0f);
  t->vertexUV(0.0f, 0.0f, 0.0f, 0.0f, 0.0f);
  t->end();

  // Title: "Game Menu"
  glEnable(GL_TEXTURE_2D);
  glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
  {
    const wstring title = L"Game Menu";
    const float titleX = (gw - (float)font->width(title)) * 0.5f;
    const float titleY = gh * 0.22f;
    glPushMatrix();
    glScalef(s, s, 1.0f);
    font->drawShadow(title, (int)titleX, (int)titleY, 0xFFFFFF);
    glPopMatrix();
  }

  // Buttons: Resume, Help & Options, Save and Quit to Title
  static const int NUM_BUTTONS = 3;
  static const wchar_t *labels[] = {
    L"Back to Game",
    L"Help & Options",
    L"Save and Quit to Title",
  };
  // Map to PauseMenu button IDs: 0=Resume, 1=H&O, 5=Exit
  static const int controlIds[] = { 0, 1, 5 };

  const int btnW = 200, btnH = 20;
  const float btnX = (gw - (float)btnW) * 0.5f;
  const float startY = gh * 0.35f;
  const float spacing = 24.0f;

  // Check mouse hover
  int mouseHoverIdx = -1;
  for (int i = 0; i < NUM_BUTTONS; ++i)
  {
    const float by = startY + (float)i * spacing;
    if (isMouseOverGameRect(btnX, by, (float)btnW, (float)btnH, s, w, h))
    {
      mouseHoverIdx = i;
      break;
    }
  }

  if (mouseHoverIdx >= 0)
  {
    g_pauseMenuSelectedIdx = mouseHoverIdx;
    g_appleHoveredControlId = controlIds[mouseHoverIdx];
    g_appleHoveredScene = (int)eUIScene_PauseMenu;
  }

  // Draw buttons
  for (int i = 0; i < NUM_BUTTONS; ++i)
  {
    const float by = startY + (float)i * spacing;
    int yImg = 1;  // normal
    if (i == g_pauseMenuSelectedIdx) yImg = 2;  // hovered/selected
    drawRealButton(t, textures, font, btnX, by, btnW, btnH,
                   wstring(labels[i]), s, yImg);
  }
}

// ---------------------------------------------------------------------------
// Inventory Menu scene drawing (Apple replacement for Iggy-based InventoryMenu)
// ---------------------------------------------------------------------------
static void drawInventoryScene(Tesselator *t, Textures *textures, Font *font, float w, float h, UIScene *scene)
{
  if (textures == nullptr || font == nullptr || scene == nullptr) return;

  Minecraft *mc = Minecraft::GetInstance();
  if (mc == nullptr || mc->player == nullptr) return;

  const float s = floorf(h / 270.0f);
  const float gw = w / s, gh = h / s;

  // Semi-transparent dark overlay
  glDisable(GL_TEXTURE_2D);
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  t->begin();
  t->color(0x00, 0x00, 0x00, 0xB0);
  t->vertexUV(0.0f, h,    0.0f, 0.0f, 0.0f);
  t->vertexUV(w,    h,    0.0f, 0.0f, 0.0f);
  t->vertexUV(w,    0.0f, 0.0f, 0.0f, 0.0f);
  t->vertexUV(0.0f, 0.0f, 0.0f, 0.0f, 0.0f);
  t->end();

  // gui/inventory.png is a 256×256 spritesheet; the inventory occupies (0,0)-(176,166)
  const float imgW = 176.0f, imgH = 166.0f;
  const float texSize = 256.0f;
  const float u1 = imgW / texSize;  // 0.6875
  const float v1 = imgH / texSize;  // 0.6484375

  // Center in GUI-scaled coordinate space
  const float cx = (gw - imgW) * 0.5f;
  const float cy = (gh - imgH) * 0.5f;

  glEnable(GL_TEXTURE_2D);
  glColor4f(1.0f, 1.0f, 1.0f, 1.0f);

  glPushMatrix();
  glScalef(s, s, 1.0f);

  // Draw inventory background from the spritesheet
  textures->bindTexture(L"/gui/inventory.png");

  t->begin();
  t->vertexUV(cx,        cy + imgH, 0.0f, 0.0f, v1);
  t->vertexUV(cx + imgW, cy + imgH, 0.0f, u1,   v1);
  t->vertexUV(cx + imgW, cy,        0.0f, u1,   0.0f);
  t->vertexUV(cx,        cy,        0.0f, 0.0f, 0.0f);
  t->end();

  // Text labels (dark gray, matching vanilla)
  glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
  font->draw(L"Crafting", (int)(cx + 88), (int)(cy + 16), 0x404040);
  font->draw(L"Inventory", (int)(cx + 8), (int)(cy + 73), 0x404040);

  // Draw items in inventory slots
  AbstractContainerMenu *menu = mc->player->inventoryMenu;

  if (menu != nullptr && Gui::itemRenderer != nullptr)
  {
    glEnable(GL_RESCALE_NORMAL);
    Lighting::turnOnGui();

    unsigned int slotCount = menu->getSize();
    for (unsigned int i = 0; i < slotCount; ++i)
    {
      Slot *slot = menu->getSlot(i);
      if (slot == nullptr || !slot->hasItem()) continue;

      shared_ptr<ItemInstance> item = slot->getItem();
      if (item == nullptr) continue;

      // slot->x/y are pixel offsets within the 176×166 image; offset by container origin
      int ix = (int)(cx + (float)slot->x);
      int iy = (int)(cy + (float)slot->y);

      Gui::itemRenderer->renderAndDecorateItem(font, textures, item, ix, iy);
      Gui::itemRenderer->renderGuiItemDecorations(font, textures, item, ix, iy);
    }

    Lighting::turnOff();
    glDisable(GL_RESCALE_NORMAL);
  }

  glPopMatrix();

  glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
}

void IggyPlayerDraw(Iggy *player)
{
  FakeIggyPlayer *fake = toFakePlayer(player);
  if (fake == nullptr)
  {
    return;
  }

  const float w = (float)fake->properties.movie_width_in_pixels;
  const float h = (float)fake->properties.movie_height_in_pixels;

  // Get game objects for texture/font rendering
  Minecraft *mc = Minecraft::GetInstance();
  Textures *textures = (mc != nullptr) ? mc->textures : nullptr;
  Font *font = (mc != nullptr) ? mc->font : nullptr;

  // Set up 2D orthographic projection matching the SWF movie dimensions
  glMatrixMode(GL_PROJECTION);
  glPushMatrix();
  glLoadIdentity();
  glOrtho(0.0f, w, h, 0.0f, 100.0f, 300.0f);
  glMatrixMode(GL_MODELVIEW);
  glPushMatrix();
  glLoadIdentity();
  glTranslatef(0.0f, 0.0f, -200.0f);

  glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
  glDisable(GL_DEPTH_TEST);
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

  Tesselator *t = Tesselator::getInstance();
  if (t != nullptr)
  {
    // Each Iggy player is owned by a UIScene — identify which one this is
    UIScene *scene = getSceneForPlayer(player);
    const EUIScene sceneType = (scene != nullptr) ? scene->getSceneType() : eUIScene_COUNT;

    // Clear hover state before scene draws — the active scene will set it
    if (sceneType == eUIScene_SaveMessage ||
        sceneType == eUIScene_MainMenu ||
        sceneType == eUIScene_HelpAndOptionsMenu ||
        sceneType == eUIScene_SettingsMenu ||
        sceneType == eUIScene_SettingsOptionsMenu ||
        sceneType == eUIScene_SettingsGraphicsMenu ||
        sceneType == eUIScene_SettingsAudioMenu ||
        sceneType == eUIScene_LoadOrJoinMenu ||
        sceneType == eUIScene_CreateWorldMenu ||
        sceneType == eUIScene_PauseMenu)
    {
      g_appleHoveredControlId = -1;
      g_appleHoveredScene = -1;
    }

    // Reset loading screen tip state when transitioning away
    if (sceneType != eUIScene_FullscreenProgress && g_wasShowingLoadingScreen)
    {
      g_loadingTipInitialized = false;
      g_wasShowingLoadingScreen = false;
    }

    switch (sceneType)
    {
    case eUIScene_SaveMessage:
      drawSaveMessageScene(t, textures, font, w, h);
      break;

    case eUIScene_MainMenu:
      drawMainMenuScene(t, textures, font, w, h);
      break;

    case eUIScene_HelpAndOptionsMenu:
      drawHelpAndOptionsScene(t, textures, font, w, h, static_cast<UIScene_HelpAndOptionsMenu *>(scene));
      break;

    case eUIScene_SettingsMenu:
      drawSettingsMenuScene(t, textures, font, w, h, static_cast<UIScene_SettingsMenu *>(scene));
      break;

    case eUIScene_SettingsOptionsMenu:
      drawSettingsOptionsScene(t, textures, font, w, h, static_cast<UIScene_SettingsOptionsMenu *>(scene));
      break;

    case eUIScene_SettingsGraphicsMenu:
      drawSettingsGraphicsScene(t, textures, font, w, h, static_cast<UIScene_SettingsGraphicsMenu *>(scene));
      break;

    case eUIScene_SettingsAudioMenu:
      drawSettingsAudioScene(t, textures, font, w, h, static_cast<UIScene_SettingsAudioMenu *>(scene));
      break;

    case eUIScene_LoadOrJoinMenu:
      drawLoadOrJoinScene(t, textures, font, w, h);
      break;

    case eUIScene_CreateWorldMenu:
      drawCreateWorldScene(t, textures, font, w, h, static_cast<UIScene_CreateWorldMenu *>(scene));
      break;

    case eUIScene_FullscreenProgress:
      drawFullscreenProgressScene(t, textures, font, w, h);
      g_wasShowingLoadingScreen = true;
      break;

    case eUIScene_PauseMenu:
      drawPauseMenuScene(t, textures, font, w, h);
      break;

    case eUIScene_InventoryMenu:
      drawInventoryScene(t, textures, font, w, h, scene);
      break;

    case eUIComponent_Panorama:
      // Draw the dirt background (same as the real game's panorama fallback).
      // This component sits behind MainMenu and SaveMessage in the layer stack.
      drawPanoramaBackground(t, textures, w, h);
      break;

    case eUIComponent_Logo:
    case eUIScene_HUD:
      // These scenes' SWF content is cosmetic; their visual content is
      // drawn by the parent scene or the panorama component.
      break;

    default:
      break;
    }
  }

  // Restore render state
  glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
  glEnable(GL_DEPTH_TEST);
  glDisable(GL_BLEND);
  glEnable(GL_TEXTURE_2D);

  glMatrixMode(GL_MODELVIEW);
  glPopMatrix();
  glMatrixMode(GL_PROJECTION);
  glPopMatrix();

  // Emit custom-draw regions for C++ overlays
  if (g_customDrawCallback != nullptr)
  {
    static const wchar_t *regionNames[] = {
      L"Splash",
      L"player",
      L"EnchantmentBook",
      L"pointerIcon",
    };

    for (const wchar_t *name : regionNames)
    {
      IggyCustomDrawCallbackRegion region = {};
      region.name = (IggyUTF16 *)name;
      region.x0 = 0;
      region.y0 = 0;
      region.x1 = w;
      region.y1 = h;
      g_customDrawCallback(g_customDrawCallbackData, player, &region);
    }
  }
}

void IggyPlayerDrawTile(Iggy *, S32, S32, S32, S32, S32) {}

void IggyPlayerDrawTilesStart(Iggy *) {}

void IggyPlayerDrawTilesEnd(Iggy *) {}

IggyValuePath *IggyPlayerRootPath(Iggy *player)
{
  FakeIggyPlayer *fake = toFakePlayer(player);
  return fake != nullptr ? &fake->rootPath : nullptr;
}

rrbool IggyPlayerDispatchEventRS(Iggy *, IggyEvent *, IggyEventResult *) { return 0; }

IggyResult IggyPlayerCallMethodRS(Iggy *player, IggyDataValue *result, IggyValuePath *, IggyName methodName, S32, IggyDataValue *)
{
  FakeIggyPlayer *fake = toFakePlayer(player);
  const std::string normalizedName = normalizeAscii(lookupFastName(fake, methodName));

  if (normalizedName.find("label") != std::string::npos || normalizedName.find("text") != std::string::npos)
  {
    setEmptyStringResult(result);
  }
  else if (normalizedName.find("touch") != std::string::npos || normalizedName.find("visible") != std::string::npos)
  {
    setBooleanResult(result, 1);
  }
  else if (normalizedName.find("height") != std::string::npos)
  {
    setNumericResult(result, 64.0);
  }
  else if (normalizedName.find("width") != std::string::npos)
  {
    setNumericResult(result, 256.0);
  }
  else
  {
    setNumericResult(result, 0.0);
  }

  return IGGY_RESULT_SUCCESS;
}

IggyName IggyPlayerCreateFastName(Iggy *player, IggyUTF16 const *name, S32 len)
{
  FakeIggyPlayer *fake = toFakePlayer(player);
  if (fake == nullptr || name == nullptr)
  {
    return 0;
  }

  const S32 safeLen = len >= 0 ? len : (S32)wcslen((const wchar_t *)name);
  std::wstring wideName((const wchar_t *)name, safeLen);
  std::string narrowName(wideName.begin(), wideName.end());
  const IggyName fastName = fake->nextFastName++;
  fake->fastNames[fastName] = narrowName;
  return fastName;
}

void IggyMakeEventKey(IggyEvent *, IggyKeyevent, IggyKeycode, IggyKeyloc) {}

void IggyMakeEventMouseMove(IggyEvent *, S32, S32) {}

void IggySetAS3ExternalFunctionCallbackUTF16(Iggy_AS3ExternalFunctionUTF16 *, void *) {}

void IggySetCustomDrawCallback(Iggy_CustomDrawCallback *callback, void *userData)
{
  g_customDrawCallback = callback;
  g_customDrawCallbackData = userData;
}

// GDraw integration stubs — called by gdraw_apple.c and Vulkan_UIController.cpp
static GDrawFunctions *g_gdrawFunctions = nullptr;

void IggySetGDraw(GDrawFunctions *gdraw)
{
  g_gdrawFunctions = gdraw;
}

void *IggyGDrawMallocAnnotated(SINTa size, const char *, int)
{
  return std::malloc((size_t)size);
}

void IggyGDrawFree(void *ptr)
{
  std::free(ptr);
}

void IggyGDrawSendWarning(Iggy *, char const *message, ...)
{
  if (message != nullptr)
  {
    va_list args;
    va_start(args, message);
    std::vfprintf(stderr, message, args);
    va_end(args);
    std::fputc('\n', stderr);
  }
}

void IggyDiscardVertexBufferCallback(void *, void *) {}

void IggySetTextureSubstitutionCallbacks(Iggy_TextureSubstitutionCreateCallback *, Iggy_TextureSubstitutionDestroyCallback *, void *) {}

void IggySetFontCachingCalculationBuffer(S32, void *, S32) {}

void IggyFontInstallTruetypeUTF8(const void *, S32, const char *, S32, U32) {}

void IggyFontInstallTruetypeFallbackCodepointUTF8(const char *, S32, U32, S32) {}

void IggyFontInstallBitmapUTF8(const IggyBitmapFontProvider *, const char *, S32, U32) {}

void IggyFontRemoveUTF8(const char *, S32, U32) {}

void IggyFontSetIndirectUTF8(const char *, S32, U32, const char *, S32, U32) {}

IggyResult IggyValueGetF64RS(IggyValuePath *path, IggyName subName, char const *subNameUtf8, F64 *result)
{
  if (result == nullptr)
  {
    return IGGY_RESULT_SUCCESS;
  }

  FakeIggyPlayer *fake = path != nullptr ? toFakePlayer(path->f) : nullptr;
  std::string normalizedName = subNameUtf8 != nullptr ? normalizeAscii(subNameUtf8) : normalizeAscii(lookupFastName(fake, subName));

  if (normalizedName == "width")
  {
    *result = 256.0;
  }
  else if (normalizedName == "height")
  {
    *result = 64.0;
  }
  else
  {
    *result = 0.0;
  }

  return IGGY_RESULT_SUCCESS;
}

IggyResult IggyValueGetBooleanRS(IggyValuePath *, IggyName, char const *, rrbool *result)
{
  if (result != nullptr)
  {
    *result = 1;
  }
  return IGGY_RESULT_SUCCESS;
}

rrbool IggyValueSetBooleanRS(IggyValuePath *, IggyName, char const *, rrbool) { return 1; }

rrbool IggyValuePathMakeNameRef(IggyValuePath *result, IggyValuePath *parent, char const *textUtf8)
{
  if (result == nullptr)
  {
    return 0;
  }

  std::memset(result, 0, sizeof(*result));
  if (parent != nullptr)
  {
    result->f = parent->f;
    result->parent = parent;
  }

  if (textUtf8 != nullptr && parent != nullptr)
  {
    FakeIggyPlayer *fake = toFakePlayer(parent->f);
    if (fake != nullptr)
    {
      const IggyName fastName = fake->nextFastName++;
      fake->fastNames[fastName] = textUtf8;
      result->name = fastName;
    }
  }

  return 1;
}

rrbool IggyDebugGetMemoryUseInfo(Iggy *, IggyLibrary, char const *, S32, S32, IggyMemoryUseInfo *) { return 0; }

} // extern "C"

// ---------------------------------------------------------------------------
// C_4JProfile -- singleton + method stubs
// ---------------------------------------------------------------------------
#include "../Windows64/4JLibs/inc/4J_Profile.h"

// ProfileManager singleton is defined in Extrax64Stubs.cpp

namespace
{
unsigned char g_profileData[XUSER_MAX_COUNT][2048] = {};
C_4JProfile::PROFILESETTINGS g_profileSettings[XUSER_MAX_COUNT] = {};
bool g_profileIsFullVersion = true;
int g_primaryPad = 0;

static std::string getProfileFilePath()
{
    const char *home = std::getenv("HOME");
    if (!home) home = "/tmp";
    std::string dir = std::string(home) + "/Library/Application Support/minecraft";
    mkdir(dir.c_str(), 0755);
    return dir + "/profile.dat";
}

static bool loadProfileFromDisk()
{
    std::string path = getProfileFilePath();
    FILE *f = fopen(path.c_str(), "rb");
    if (!f) return false;
    fread(g_profileData, sizeof(g_profileData), 1, f);
    fclose(f);
    app.DebugPrintf("[MCE] Loaded profile data from %s\n", path.c_str());
    return true;
}

static void saveProfileToDisk()
{
    std::string path = getProfileFilePath();
    FILE *f = fopen(path.c_str(), "wb");
    if (!f) return;
    fwrite(g_profileData, sizeof(g_profileData), 1, f);
    fclose(f);
}
}

void C_4JProfile::Initialise(DWORD, DWORD, unsigned short, UINT, UINT, DWORD *, int iGameDefinedDataSizeX4, unsigned int *puiGameDefinedDataChangedBitmask)
{
  if (puiGameDefinedDataChangedBitmask != nullptr)
  {
    *puiGameDefinedDataChangedBitmask = 0;
  }

  // Try to load saved profile data from disk first
  bool loaded = loadProfileFromDisk();
  bool migratedLegacyAudioDefaults = false;

  const size_t profileBytes = std::min<size_t>((size_t)(iGameDefinedDataSizeX4 / 4), sizeof(g_profileData[0]));
  for (int i = 0; i < XUSER_MAX_COUNT; ++i)
  {
    std::memset(&g_profileSettings[i], 0, sizeof(g_profileSettings[i]));
    GAME_SETTINGS *gameSettings = reinterpret_cast<GAME_SETTINGS *>(g_profileData[i]);
    if (loaded)
    {
      // Early Apple profiles accidentally left both audio sliders at zero.
      // Treat an all-muted pair as legacy bad defaults and restore the intended 100%.
      if (profileBytes >= sizeof(GAME_SETTINGS) &&
          gameSettings->ucMusicVolume == 0 &&
          gameSettings->ucSoundFXVolume == 0)
      {
        gameSettings->ucMusicVolume = 100;
        gameSettings->ucSoundFXVolume = 100;
        migratedLegacyAudioDefaults = true;
      }
      continue;
    }

    std::memset(g_profileData[i], 0, sizeof(g_profileData[i]));

    if (profileBytes < sizeof(GAME_SETTINGS))
    {
      continue;
    }

    gameSettings->ucMusicVolume = 100;
    gameSettings->ucSoundFXVolume = 100;
    gameSettings->ucMenuSensitivity = 100;
    gameSettings->ucInterfaceOpacity = 80;
    gameSettings->usBitmaskValues |= 0x0200;
    gameSettings->usBitmaskValues |= 0x0400;
    gameSettings->usBitmaskValues |= 0x1000;
    gameSettings->usBitmaskValues |= 0x8000;
    gameSettings->uiBitmaskValues = 0;
    gameSettings->uiBitmaskValues |= GAMESETTING_CLOUDS;
    gameSettings->uiBitmaskValues |= GAMESETTING_ONLINE;
    gameSettings->uiBitmaskValues |= GAMESETTING_FRIENDSOFFRIENDS;
    gameSettings->uiBitmaskValues |= GAMESETTING_DISPLAYUPDATEMSG;
    gameSettings->uiBitmaskValues &= ~GAMESETTING_BEDROCKFOG;
    gameSettings->uiBitmaskValues |= GAMESETTING_DISPLAYHUD;
    gameSettings->uiBitmaskValues |= GAMESETTING_DISPLAYHAND;
    gameSettings->uiBitmaskValues |= GAMESETTING_CUSTOMSKINANIM;
    gameSettings->uiBitmaskValues |= GAMESETTING_DEATHMESSAGES;
    gameSettings->uiBitmaskValues |= (GAMESETTING_UISIZE & 0x00000800);
    gameSettings->uiBitmaskValues |= (GAMESETTING_UISIZE_SPLITSCREEN & 0x00004000);
    gameSettings->uiBitmaskValues |= GAMESETTING_ANIMATEDCHARACTER;
    for (int skinIndex = 0; skinIndex < MAX_FAVORITE_SKINS; ++skinIndex)
    {
      gameSettings->uiFavoriteSkinA[skinIndex] = 0xFFFFFFFF;
    }
    gameSettings->ucCurrentFavoriteSkinPos = 0;
    gameSettings->uiMashUpPackWorldsDisplay = 0xFFFFFFFF;
    gameSettings->uiBitmaskValues &= ~GAMESETTING_PS3EULAREAD;
    gameSettings->ucLanguage = MINECRAFT_LANGUAGE_DEFAULT;
    gameSettings->uiBitmaskValues &= ~GAMESETTING_PSVITANETWORKMODEADHOC;
    gameSettings->ucTutorialCompletion[0] = 0xFF;
    gameSettings->ucTutorialCompletion[1] = 0xFF;
    gameSettings->ucTutorialCompletion[2] = 0x0F;
    gameSettings->ucTutorialCompletion[28] |= 1 << 0;
  }

  if (migratedLegacyAudioDefaults)
  {
    saveProfileToDisk();
    app.DebugPrintf("[MCE] Migrated legacy Apple profile audio defaults to 100%%");
  }
}

void C_4JProfile::Tick() {}
bool C_4JProfile::IsSignedIn(int) { return true; }
bool C_4JProfile::IsSignedInLive(int) { return false; }
bool C_4JProfile::IsGuest(int) { return false; }
UINT C_4JProfile::RequestSignInUI(bool, bool, bool, bool, bool, int(*)(LPVOID, const bool, const int), LPVOID, int) { return 0; }
UINT C_4JProfile::RequestConvertOfflineToGuestUI(int(*)(LPVOID, const bool, const int), LPVOID, int) { return 0; }
void C_4JProfile::GetXUID(int, PlayerUID *pXuid, bool) { if (pXuid) *pXuid = 0; }
BOOL C_4JProfile::AreXUIDSEqual(PlayerUID a, PlayerUID b) { return a == b; }
bool C_4JProfile::AllowedToPlayMultiplayer(int) { return true; }
void C_4JProfile::AllowedPlayerCreatedContent(int, bool, BOOL *allAllowed, BOOL *friendsAllowed) { if (allAllowed) *allAllowed = TRUE; if (friendsAllowed) *friendsAllowed = TRUE; }
void C_4JProfile::CancelProfileAvatarRequest() {}
void C_4JProfile::DisplayFullVersionPurchase(bool, int, int) {}
void C_4JProfile::SetDebugFullOverride(bool value) { g_profileIsFullVersion = value; }
void C_4JProfile::ForceQueuedProfileWrites(int) {}
eAwardType C_4JProfile::GetAwardType(int) { return eAwardType_Achievement; }
C_4JProfile::PROFILESETTINGS *C_4JProfile::GetDashboardProfileSettings(int pad) {
    if (pad < 0 || pad >= XUSER_MAX_COUNT) pad = 0;
    return &g_profileSettings[pad];
}
wstring C_4JProfile::GetDisplayName(int) { extern wchar_t g_Win64UsernameW[17]; return g_Win64UsernameW; }
void *C_4JProfile::GetGameDefinedProfileData(int pad) {
    if (pad < 0 || pad >= XUSER_MAX_COUNT) pad = 0;
    return g_profileData[pad];
}
char *C_4JProfile::GetGamertag(int) { extern char g_Win64Username[17]; return g_Win64Username; }
HRESULT C_4JProfile::GetLiveConnectionStatus() { return S_OK; }
int C_4JProfile::GetLockedProfile() { return 0; }
int C_4JProfile::GetPrimaryPad() { return g_primaryPad; }
bool C_4JProfile::GetProfileAvatar(int, int(*)(LPVOID, PBYTE, DWORD), LPVOID) { return false; }
bool C_4JProfile::IsFullVersion() { return g_profileIsFullVersion; }
bool C_4JProfile::IsSystemUIDisplayed() { return false; }
bool C_4JProfile::QuerySigninStatus() { return true; }
void C_4JProfile::ResetProfileProcessState() {}
void C_4JProfile::SetCurrentGameActivity(int, int, bool) {}
void C_4JProfile::SetLockedProfile(int) {}
void C_4JProfile::SetPrimaryPad(int pad) { g_primaryPad = pad; }
void C_4JProfile::SetRichPresenceContextValue(int, int, int) {}
void C_4JProfile::ShowProfileCard(int, PlayerUID) {}
void C_4JProfile::StartTrialGame() {}
void C_4JProfile::WriteToProfile(int, bool, bool) { saveProfileToDisk(); }
void C_4JProfile::Award(int, int, bool) {}
bool C_4JProfile::CanBeAwarded(int, int) { return false; }

// ---------------------------------------------------------------------------
// C4JStorage -- singleton + method stubs
// ---------------------------------------------------------------------------
#include "../Windows64/4JLibs/inc/4J_Storage.h"

// StorageManager singleton is defined in Extrax64Stubs.cpp

C4JStorage::C4JStorage() : m_pStringTable(nullptr) {}
void C4JStorage::Tick() {}
PVOID C4JStorage::AllocateSaveData(unsigned int uiBytes) { return calloc(1, uiBytes); }
void C4JStorage::ClearDLCOffers() {}
C4JStorage::ESaveGameState C4JStorage::DeleteSaveData(PSAVE_INFO, int(*)(LPVOID, const bool), LPVOID) { return ESaveGame_Idle; }
C4JStorage::ESaveGameState C4JStorage::DoesSaveExist(bool *pbExists) { if (pbExists) *pbExists = false; return ESaveGame_Idle; }
bool C4JStorage::EnoughSpaceForAMinSaveGame() { return true; }
C4JStorage::EDLCStatus C4JStorage::GetDLCOffers(int, int(*)(LPVOID, int, DWORD, int), LPVOID, DWORD) { return EDLC_NoOffers; }
C4JStorage::EDLCStatus C4JStorage::GetInstalledDLC(int iPad, int(*callback)(LPVOID, int, int), LPVOID param)
{
    if (callback != nullptr)
    {
        callback(param, 0, iPad);
    }
    return EDLC_NoInstalledDLC;
}
void C4JStorage::GetMountedDLCFileList(const char *, std::vector<std::string> &) {}
std::string C4JStorage::GetMountedPath(std::string) { return ""; }
XMARKETPLACE_CONTENTOFFER_INFO &C4JStorage::GetOffer(DWORD) { static XMARKETPLACE_CONTENTOFFER_INFO dummy = {}; return dummy; }
void C4JStorage::GetSaveData(void *, unsigned int *puiBytes) { if (puiBytes) *puiBytes = 0; }
bool C4JStorage::GetSaveDisabled() { return false; }
C4JStorage::ESaveGameState C4JStorage::GetSavesInfo(int, int(*)(LPVOID, SAVE_DETAILS *, const bool), LPVOID, char *) { return ESaveGame_Idle; }
unsigned int C4JStorage::GetSaveSize() { return 0; }
bool C4JStorage::GetSaveUniqueFilename(char *pszName) { if (pszName) pszName[0] = '\0'; return false; }
bool C4JStorage::GetSaveUniqueNumber(int *piVal) { if (piVal) *piVal = 0; return false; }
DWORD C4JStorage::InstallOffer(int, __uint64 *, int(*)(LPVOID, int, int), LPVOID, bool) { return 0; }
C4JStorage::ESaveGameState C4JStorage::LoadSaveData(PSAVE_INFO, int(*)(LPVOID, const bool, const bool), LPVOID) { return ESaveGame_Idle; }
C4JStorage::ESaveGameState C4JStorage::LoadSaveDataThumbnail(PSAVE_INFO, int(*)(LPVOID, PBYTE, DWORD), LPVOID) { return ESaveGame_Idle; }
DWORD C4JStorage::MountInstalledDLC(int, DWORD, int(*)(LPVOID, int, DWORD, DWORD), LPVOID, LPCSTR) { return 0; }
void C4JStorage::ResetSaveData() {}
PSAVE_DETAILS C4JStorage::ReturnSavesInfo() { return nullptr; }
C4JStorage::ESaveGameState C4JStorage::SaveSaveData(int(*)(LPVOID, const bool), LPVOID) { return ESaveGame_Idle; }
void C4JStorage::SetSaveDeviceSelected(unsigned int, bool) {}
void C4JStorage::SetSaveDisabled(bool) {}
void C4JStorage::SetSaveImages(PBYTE, DWORD, PBYTE, DWORD, PBYTE, DWORD) {}
void C4JStorage::SetSaveTitle(LPCWSTR) {}
DWORD C4JStorage::UnmountInstalledDLC(LPCSTR) { return 0; }

// ---------------------------------------------------------------------------
// WinsockNetLayer -- static member definitions + method stubs
// ---------------------------------------------------------------------------
// The header is guarded by _WINDOWS64 || __APPLE__, so it is available.
#include "../Windows64/Network/WinsockNetLayer.h"

bool WinsockNetLayer::s_active = false;
bool WinsockNetLayer::s_connected = false;
BYTE WinsockNetLayer::s_localSmallId = 0;
bool WinsockNetLayer::s_upnpMapped = false;
char WinsockNetLayer::s_externalIP[64] = {};

// Private statics that are also needed for the translation unit
bool WinsockNetLayer::s_isHost = false;
bool WinsockNetLayer::s_initialized = false;
BYTE WinsockNetLayer::s_hostSmallId = 0;
BYTE WinsockNetLayer::s_nextSmallId = 1;
int WinsockNetLayer::s_hostGamePort = WIN64_NET_DEFAULT_PORT;
SOCKET WinsockNetLayer::s_listenSocket = INVALID_SOCKET;
SOCKET WinsockNetLayer::s_hostConnectionSocket = INVALID_SOCKET;
HANDLE WinsockNetLayer::s_acceptThread = nullptr;
HANDLE WinsockNetLayer::s_clientRecvThread = nullptr;
CRITICAL_SECTION WinsockNetLayer::s_sendLock = {};
CRITICAL_SECTION WinsockNetLayer::s_connectionsLock = {};
std::vector<Win64RemoteConnection> WinsockNetLayer::s_connections;
SOCKET WinsockNetLayer::s_advertiseSock = INVALID_SOCKET;
HANDLE WinsockNetLayer::s_advertiseThread = nullptr;
volatile bool WinsockNetLayer::s_advertising = false;
Win64LANBroadcast WinsockNetLayer::s_advertiseData = {};
CRITICAL_SECTION WinsockNetLayer::s_advertiseLock = {};
SOCKET WinsockNetLayer::s_discoverySock = INVALID_SOCKET;
HANDLE WinsockNetLayer::s_discoveryThread = nullptr;
volatile bool WinsockNetLayer::s_discovering = false;
CRITICAL_SECTION WinsockNetLayer::s_discoveryLock = {};
std::vector<Win64LANSession> WinsockNetLayer::s_discoveredSessions;
CRITICAL_SECTION WinsockNetLayer::s_disconnectLock = {};
std::vector<BYTE> WinsockNetLayer::s_disconnectedSmallIds;
CRITICAL_SECTION WinsockNetLayer::s_freeSmallIdLock = {};
std::vector<BYTE> WinsockNetLayer::s_freeSmallIds;

bool WinsockNetLayer::Initialize() { return false; }
void WinsockNetLayer::Shutdown() {}
bool WinsockNetLayer::HostGame(int) { return false; }
bool WinsockNetLayer::JoinGame(const char *, int) { return false; }
bool WinsockNetLayer::SendToSmallId(BYTE, const void *, int) { return false; }
void WinsockNetLayer::CloseConnectionBySmallId(BYTE) {}
bool WinsockNetLayer::PopDisconnectedSmallId(BYTE *) { return false; }
void WinsockNetLayer::PushFreeSmallId(BYTE) {}
bool WinsockNetLayer::StartAdvertising(int, const wchar_t *, unsigned int, unsigned int, unsigned char, unsigned short) { return false; }
void WinsockNetLayer::StopAdvertising() {}
void WinsockNetLayer::UpdateAdvertiseJoinable(bool) {}
void WinsockNetLayer::StopDiscovery() {}
std::vector<Win64LANSession> WinsockNetLayer::GetDiscoveredSessions() { return {}; }
void WinsockNetLayer::CleanupUPnP(int) {}

// ---------------------------------------------------------------------------
// Item::*_Id constant definitions
// ---------------------------------------------------------------------------
// These are declared as "static const int" in the Item class (Item.h).
// In C++, static const integral members with in-class initialisers normally
// do not need out-of-line definitions -- unless their address is taken
// (ODR-used).  The linker reports them as undefined, so we provide definitions
// here.  The values must match the in-class initialisers in Item.h.
// ---------------------------------------------------------------------------
#include "../../Minecraft.World/Item.h"

const int Item::apple_Id;
const int Item::arrow_Id;
const int Item::beef_cooked_Id;
const int Item::beef_raw_Id;
const int Item::book_Id;
const int Item::boots_chain_Id;
const int Item::boots_cloth_Id;
const int Item::boots_diamond_Id;
const int Item::boots_iron_Id;
const int Item::bread_Id;
const int Item::chestplate_chain_Id;
const int Item::chestplate_cloth_Id;
const int Item::chestplate_diamond_Id;
const int Item::chestplate_iron_Id;
const int Item::chicken_cooked_Id;
const int Item::chicken_raw_Id;
const int Item::clock_Id;
const int Item::coal_Id;
const int Item::compass_Id;
const int Item::cookie_Id;
const int Item::diamond_Id;
const int Item::enderPearl_Id;
const int Item::expBottle_Id;
const int Item::eyeOfEnder_Id;
const int Item::fish_cooked_Id;
const int Item::flintAndSteel_Id;
const int Item::goldIngot_Id;
const int Item::hatchet_diamond_Id;
const int Item::hatchet_iron_Id;
const int Item::helmet_chain_Id;
const int Item::helmet_cloth_Id;
const int Item::helmet_diamond_Id;
const int Item::helmet_iron_Id;
const int Item::hoe_diamond_Id;
const int Item::hoe_iron_Id;
const int Item::ironIngot_Id;
const int Item::leggings_chain_Id;
const int Item::leggings_cloth_Id;
const int Item::leggings_diamond_Id;
const int Item::leggings_iron_Id;
const int Item::melon_Id;
const int Item::paper_Id;
const int Item::pickAxe_diamond_Id;
const int Item::pickAxe_iron_Id;
const int Item::porkChop_cooked_Id;
const int Item::porkChop_raw_Id;
const int Item::redStone_Id;
const int Item::rotten_flesh_Id;
const int Item::saddle_Id;
const int Item::seeds_melon_Id;
const int Item::seeds_pumpkin_Id;
const int Item::seeds_wheat_Id;
const int Item::shears_Id;
const int Item::shovel_diamond_Id;
const int Item::shovel_iron_Id;
const int Item::sword_diamond_Id;
const int Item::sword_iron_Id;
const int Item::wheat_Id;

// ---------------------------------------------------------------------------
// Tile::*_Id constant definitions
// ---------------------------------------------------------------------------
#include "../../Minecraft.World/Tile.h"

const int Tile::bookshelf_Id;
const int Tile::cloth_Id;
const int Tile::glass_Id;
const int Tile::lightGem_Id;

// ---------------------------------------------------------------------------
// ShutdownManager stubs
// ---------------------------------------------------------------------------
// The class is defined in platform-specific headers with inline empty-bodied
// methods.  The compiler may not emit standalone out-of-line copies, causing
// linker errors.  We include the header and force emission by assigning the
// member function addresses to volatile pointers.
// ---------------------------------------------------------------------------
#include "../Orbis/OrbisExtras/ShutdownManager.h"

// Force the compiler to emit out-of-line bodies for the inline static methods.
// We use __attribute__((used)) on function pointers with external linkage to
// guarantee they survive optimisation and dead-stripping.
__attribute__((used)) void (*_sm_HasStarted1)(ShutdownManager::EThreadId) =
    static_cast<void(*)(ShutdownManager::EThreadId)>(&ShutdownManager::HasStarted);
__attribute__((used)) void (*_sm_HasStarted2)(ShutdownManager::EThreadId, C4JThread::EventArray *) =
    static_cast<void(*)(ShutdownManager::EThreadId, C4JThread::EventArray *)>(&ShutdownManager::HasStarted);
__attribute__((used)) bool (*_sm_ShouldRun)(ShutdownManager::EThreadId) = &ShutdownManager::ShouldRun;
__attribute__((used)) void (*_sm_HasFinished)(ShutdownManager::EThreadId) = &ShutdownManager::HasFinished;

// ---------------------------------------------------------------------------
// NetworkPlayerXbox stubs
// ---------------------------------------------------------------------------
#include "../Xbox/Network/NetworkPlayerXbox.h"

NetworkPlayerXbox::NetworkPlayerXbox(IQNetPlayer *qnetPlayer)
    : m_qnetPlayer(qnetPlayer), m_pSocket(nullptr) {}

unsigned char NetworkPlayerXbox::GetSmallId() { return m_qnetPlayer != nullptr ? m_qnetPlayer->GetSmallId() : 0; }
void NetworkPlayerXbox::SendData(INetworkPlayer *player, const void *data, int dataSize, bool lowPriority)
{
  if (m_qnetPlayer == nullptr || player == nullptr || data == nullptr || dataSize <= 0)
  {
    return;
  }

  NetworkPlayerXbox *otherPlayer = static_cast<NetworkPlayerXbox *>(player);
  IQNetPlayer *otherQnetPlayer = otherPlayer != nullptr ? otherPlayer->GetQNetPlayer() : nullptr;
  if (otherQnetPlayer != nullptr)
  {
    m_qnetPlayer->SendData(otherQnetPlayer, data, (DWORD)dataSize, lowPriority ? QNET_SENDDATA_LOW_PRIORITY : 0);
  }
}
bool NetworkPlayerXbox::IsSameSystem(INetworkPlayer *player)
{
  if (m_qnetPlayer == nullptr || player == nullptr)
  {
    return false;
  }

  NetworkPlayerXbox *otherPlayer = static_cast<NetworkPlayerXbox *>(player);
  IQNetPlayer *otherQnetPlayer = otherPlayer != nullptr ? otherPlayer->GetQNetPlayer() : nullptr;
  return otherQnetPlayer != nullptr ? m_qnetPlayer->IsSameSystem(otherQnetPlayer) : false;
}
int NetworkPlayerXbox::GetSendQueueSizeBytes(INetworkPlayer *, bool) { return 0; }
int NetworkPlayerXbox::GetSendQueueSizeMessages(INetworkPlayer *, bool) { return 0; }
int NetworkPlayerXbox::GetCurrentRtt() { return m_qnetPlayer != nullptr ? (int)m_qnetPlayer->GetCurrentRtt() : 0; }
bool NetworkPlayerXbox::IsHost() { return m_qnetPlayer != nullptr ? m_qnetPlayer->IsHost() : false; }
bool NetworkPlayerXbox::IsGuest() { return false; }
bool NetworkPlayerXbox::IsLocal() { return m_qnetPlayer != nullptr ? m_qnetPlayer->IsLocal() : false; }
int NetworkPlayerXbox::GetSessionIndex() { return m_qnetPlayer != nullptr ? m_qnetPlayer->GetSessionIndex() : 0; }
bool NetworkPlayerXbox::IsTalking() { return false; }
bool NetworkPlayerXbox::IsMutedByLocalUser(int) { return false; }
bool NetworkPlayerXbox::HasVoice() { return m_qnetPlayer != nullptr ? m_qnetPlayer->HasVoice() : false; }
bool NetworkPlayerXbox::HasCamera() { return m_qnetPlayer != nullptr ? m_qnetPlayer->HasCamera() : false; }
int NetworkPlayerXbox::GetUserIndex() { return m_qnetPlayer != nullptr ? m_qnetPlayer->GetUserIndex() : 0; }
void NetworkPlayerXbox::SetSocket(Socket *pSocket) { m_pSocket = pSocket; }
Socket *NetworkPlayerXbox::GetSocket() { return m_pSocket; }
const wchar_t *NetworkPlayerXbox::GetOnlineName() { return m_qnetPlayer != nullptr ? m_qnetPlayer->GetGamertag() : L"Player"; }
std::wstring NetworkPlayerXbox::GetDisplayName() { return GetOnlineName(); }
PlayerUID NetworkPlayerXbox::GetUID() { return m_qnetPlayer != nullptr ? m_qnetPlayer->GetXuid() : 0; }
IQNetPlayer *NetworkPlayerXbox::GetQNetPlayer() { return m_qnetPlayer; }

// ---------------------------------------------------------------------------
// RuntimeException
// ---------------------------------------------------------------------------
#include "../../Minecraft.World/Exceptions.h"

RuntimeException::RuntimeException(const wstring &) {}

// ---------------------------------------------------------------------------
// NbtSlotFile::MAGIC_NUMBER definition
// ---------------------------------------------------------------------------
#include "../../Minecraft.World/TilePos.h"
#include "../../Minecraft.World/NbtSlotFile.h"

const int NbtSlotFile::MAGIC_NUMBER;

// ---------------------------------------------------------------------------
// LeaderboardManager::m_instance definition
// ---------------------------------------------------------------------------
#include "../Common/Leaderboards/LeaderboardManager.h"

LeaderboardManager *LeaderboardManager::m_instance = nullptr;

// ---------------------------------------------------------------------------
// CMinecraftApp::GetTPConfigVal stub
// ---------------------------------------------------------------------------
// The header is large and platform-heavy; we use the forward declaration
// that's already included transitively through stdafx.h.
// ---------------------------------------------------------------------------
#ifndef __PS3__
int CMinecraftApp::GetTPConfigVal(WCHAR *) { return 0; }
#endif

// ---------------------------------------------------------------------------
// MemSect stub
// ---------------------------------------------------------------------------
void MemSect(int) {}
