// macOS entry point for Minecraft Community Edition
// Replaces the demo rotating-cube bootstrap with actual game initialization.
// Follows the same flow as Windows64/Windows64_Minecraft.cpp.

#include "stdafx.h"

#include "../Vulkan/VulkanBootstrapApp.h"
#include "../Vulkan/4JLibs/inc/4J_Render.h"
#include "../Windows64/GameConfig/Minecraft.spa.h"
#include "../Common/App_Defines.h"
#include "../Tesselator.h"
#include "../Options.h"
#include "../User.h"
#include "../../Minecraft.World/TilePos.h"
#include "../../Minecraft.World/AABB.h"
#include "../../Minecraft.World/Vec3.h"
#include "../../Minecraft.World/IntCache.h"
#include "../../Minecraft.World/compression.h"
#include "../../Minecraft.World/OldChunkStorage.h"
#include "../../Minecraft.World/Level.h"
#include "../../Minecraft.World/net.minecraft.world.level.tile.h"
#include "../TitleScreen.h"
#include "../ScreenSizeCalculator.h"
#include "../StringTable.h"
#include <filesystem>
#include <signal.h>
#include <execinfo.h>
#include <cstdlib>
#include <cstring>
#include <cwchar>

// Bridge: Language::getElement() calls this to look up strings from app's StringTable
// Access via friend or by exposing a helper on CMinecraftApp
wstring LanguageGetStringFromApp(const wstring &id)
{
  return app.lookupString(id);
}

// GLFW mouse state for Screen input - forward declarations
extern int g_iScreenWidth;
extern int g_iScreenHeight;
static Minecraft *g_pMinecraft = nullptr;

double g_cursorX = 0.0;
double g_cursorY = 0.0;

// ---- Mouse grab & delta tracking for in-game look ----
static bool   g_appleMouseGrabbed = false;
static double g_lastCursorX = 0.0;
static double g_lastCursorY = 0.0;
static float  g_appleMouseDeltaX = 0.0f;
static float  g_appleMouseDeltaY = 0.0f;
static GLFWwindow *g_appleWindow = nullptr;

// ---- Apple keyboard state for UI input ----
// GLFW key callback stores edge-triggered state here; UIController reads it.
static bool g_appleKeyDown[512]     = {};
static bool g_appleKeyPressed[512]  = {};  // went down this frame
static bool g_appleKeyReleased[512] = {};  // went up this frame

extern "C" void AppleInput_HandleChar(unsigned int codepoint);
extern "C" void AppleInput_HandleKey(int key, int action);

static void glfwKeyCallback(GLFWwindow *, int key, int /*scancode*/, int action, int /*mods*/)
{
  if (key < 0 || key >= 512) return;
  if (action == GLFW_PRESS)   { g_appleKeyDown[key] = true;  g_appleKeyPressed[key] = true; }
  if (action == GLFW_RELEASE) { g_appleKeyDown[key] = false; g_appleKeyReleased[key] = true; }
  AppleInput_HandleKey(key, action);
}

static void glfwCharCallback(GLFWwindow *, unsigned int codepoint)
{
  AppleInput_HandleChar(codepoint);
}

// Called at start of each frame (before glfwPollEvents) to clear edge flags
void AppleKeyboard_ClearEdges()
{
  std::memset(g_appleKeyPressed,  0, sizeof(g_appleKeyPressed));
  std::memset(g_appleKeyReleased, 0, sizeof(g_appleKeyReleased));
}

bool AppleKeyboard_IsDown(int glfwKey)     { return (glfwKey >= 0 && glfwKey < 512) ? g_appleKeyDown[glfwKey] : false; }
bool AppleKeyboard_IsPressed(int glfwKey)  { return (glfwKey >= 0 && glfwKey < 512) ? g_appleKeyPressed[glfwKey] : false; }
bool AppleKeyboard_IsReleased(int glfwKey) { return (glfwKey >= 0 && glfwKey < 512) ? g_appleKeyReleased[glfwKey] : false; }

static void glfwMouseButtonCallback(GLFWwindow *, int button, int action, int);

// Store mouse button state in the same keyboard array (slot 500 = left, 501 = right)
static void glfwMouseButtonCallbackUI(GLFWwindow *win, int button, int action, int mods)
{
  if (button == GLFW_MOUSE_BUTTON_LEFT)
  {
    const int slot = 500;  // synthetic GLFW key slot for mouse left
    if (action == GLFW_PRESS)   { g_appleKeyDown[slot] = true;  g_appleKeyPressed[slot] = true; }
    if (action == GLFW_RELEASE) { g_appleKeyDown[slot] = false; g_appleKeyReleased[slot] = true; }
  }
  if (button == GLFW_MOUSE_BUTTON_RIGHT)
  {
    const int slot = 501;  // synthetic GLFW key slot for mouse right
    if (action == GLFW_PRESS)   { g_appleKeyDown[slot] = true;  g_appleKeyPressed[slot] = true; }
    if (action == GLFW_RELEASE) { g_appleKeyDown[slot] = false; g_appleKeyReleased[slot] = true; }
  }
  // Also forward to Screen-level mouse handling
  glfwMouseButtonCallback(win, button, action, mods);
}

static void glfwCursorPosCallback(GLFWwindow *, double xpos, double ypos)
{
  if (g_appleMouseGrabbed)
  {
    g_appleMouseDeltaX += (float)(xpos - g_lastCursorX);
    g_appleMouseDeltaY += (float)(ypos - g_lastCursorY);
  }
  g_lastCursorX = xpos;
  g_lastCursorY = ypos;
  g_cursorX = xpos;
  g_cursorY = ypos;
}

// ---- Mouse grab API for game code ----
void AppleMouse_SetGrabbed(bool grabbed)
{
  if (g_appleMouseGrabbed == grabbed) return;
  g_appleMouseGrabbed = grabbed;
  if (g_appleWindow != nullptr)
  {
    if (grabbed)
    {
      glfwSetInputMode(g_appleWindow, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
      // Seed last position so first delta after grab isn't huge
      glfwGetCursorPos(g_appleWindow, &g_lastCursorX, &g_lastCursorY);
      g_appleMouseDeltaX = 0.0f;
      g_appleMouseDeltaY = 0.0f;
    }
    else
    {
      glfwSetInputMode(g_appleWindow, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
    }
  }
}
bool  AppleMouse_IsGrabbed()    { return g_appleMouseGrabbed; }
float AppleMouse_GetDeltaX()    { return g_appleMouseDeltaX; }
float AppleMouse_GetDeltaY()    { return g_appleMouseDeltaY; }
void  AppleMouse_ClearDeltas()  { g_appleMouseDeltaX = 0.0f; g_appleMouseDeltaY = 0.0f; }

static void glfwMouseButtonCallback(GLFWwindow *, int button, int action, int)
{
  if (g_pMinecraft == nullptr || g_pMinecraft->screen == nullptr) return;

  ScreenSizeCalculator ssc(g_pMinecraft->options, g_iScreenWidth, g_iScreenHeight);
  int sx = (int)(g_cursorX * ssc.getWidth() / g_iScreenWidth);
  int sy = (int)(g_cursorY * ssc.getHeight() / g_iScreenHeight);

  g_pMinecraft->screen->handleMouseInput(sx, sy, button, action == GLFW_PRESS);
}

static void segfault_handler(int sig) {
    fprintf(stderr, "\n[MCE] CAUGHT SIGNAL %d\n", sig);
    void *array[30];
    int size = backtrace(array, 30);
    fprintf(stderr, "[MCE] Backtrace (%d frames):\n", size);
    backtrace_symbols_fd(array, size, STDERR_FILENO);
    _exit(139);
}

#include <iostream>

#define FIFTY_ONE_MB (1000000 * 51)
#define NUM_PROFILE_VALUES 5
#define NUM_PROFILE_SETTINGS 4

DWORD dwProfileSettingsA[NUM_PROFILE_VALUES] = {0, 0, 0, 0, 0};

int g_iScreenWidth = 1280;
int g_iScreenHeight = 720;
char g_Win64Username[17] = "Player";
wchar_t g_Win64UsernameW[17] = L"Player";

static void initialisePlayerIdentity()
{
  const char *userName = std::getenv("USER");
  if (userName == NULL || userName[0] == '\0')
  {
    userName = "Player";
  }

  std::strncpy(g_Win64Username, userName, sizeof(g_Win64Username) - 1);
  g_Win64Username[sizeof(g_Win64Username) - 1] = '\0';

  std::mbstate_t conversionState = {};
  const char *conversionSource = g_Win64Username;
  const size_t wideCapacity = sizeof(g_Win64UsernameW) / sizeof(g_Win64UsernameW[0]);
  std::wcsncpy(g_Win64UsernameW, L"Player", wideCapacity - 1);
  g_Win64UsernameW[wideCapacity - 1] = L'\0';
  std::mbsrtowcs(g_Win64UsernameW, &conversionSource, wideCapacity - 1, &conversionState);
  g_Win64UsernameW[wideCapacity - 1] = L'\0';
}

static void DefineActions(void)
{
  // Match the Windows64 controller maps so the shared menu/game logic sees the layout it expects.
  InputManager.SetGameJoypadMaps(MAP_STYLE_0, ACTION_MENU_A, _360_JOY_BUTTON_A);
  InputManager.SetGameJoypadMaps(MAP_STYLE_0, ACTION_MENU_B, _360_JOY_BUTTON_B);
  InputManager.SetGameJoypadMaps(MAP_STYLE_0, ACTION_MENU_X, _360_JOY_BUTTON_X);
  InputManager.SetGameJoypadMaps(MAP_STYLE_0, ACTION_MENU_Y, _360_JOY_BUTTON_Y);
  InputManager.SetGameJoypadMaps(MAP_STYLE_0, ACTION_MENU_OK, _360_JOY_BUTTON_A);
  InputManager.SetGameJoypadMaps(MAP_STYLE_0, ACTION_MENU_CANCEL, _360_JOY_BUTTON_B);
  InputManager.SetGameJoypadMaps(MAP_STYLE_0, ACTION_MENU_UP, _360_JOY_BUTTON_DPAD_UP | _360_JOY_BUTTON_LSTICK_UP);
  InputManager.SetGameJoypadMaps(MAP_STYLE_0, ACTION_MENU_DOWN, _360_JOY_BUTTON_DPAD_DOWN | _360_JOY_BUTTON_LSTICK_DOWN);
  InputManager.SetGameJoypadMaps(MAP_STYLE_0, ACTION_MENU_LEFT, _360_JOY_BUTTON_DPAD_LEFT | _360_JOY_BUTTON_LSTICK_LEFT);
  InputManager.SetGameJoypadMaps(MAP_STYLE_0, ACTION_MENU_RIGHT, _360_JOY_BUTTON_DPAD_RIGHT | _360_JOY_BUTTON_LSTICK_RIGHT);
  InputManager.SetGameJoypadMaps(MAP_STYLE_0, ACTION_MENU_PAGEUP, _360_JOY_BUTTON_LT);
  InputManager.SetGameJoypadMaps(MAP_STYLE_0, ACTION_MENU_PAGEDOWN, _360_JOY_BUTTON_RT);
  InputManager.SetGameJoypadMaps(MAP_STYLE_0, ACTION_MENU_RIGHT_SCROLL, _360_JOY_BUTTON_RB);
  InputManager.SetGameJoypadMaps(MAP_STYLE_0, ACTION_MENU_LEFT_SCROLL, _360_JOY_BUTTON_LB);
  InputManager.SetGameJoypadMaps(MAP_STYLE_0, ACTION_MENU_PAUSEMENU, _360_JOY_BUTTON_START);
  InputManager.SetGameJoypadMaps(MAP_STYLE_0, ACTION_MENU_STICK_PRESS, _360_JOY_BUTTON_LTHUMB);
  InputManager.SetGameJoypadMaps(MAP_STYLE_0, ACTION_MENU_OTHER_STICK_PRESS, _360_JOY_BUTTON_RTHUMB);
  InputManager.SetGameJoypadMaps(MAP_STYLE_0, ACTION_MENU_OTHER_STICK_UP, _360_JOY_BUTTON_RSTICK_UP);
  InputManager.SetGameJoypadMaps(MAP_STYLE_0, ACTION_MENU_OTHER_STICK_DOWN, _360_JOY_BUTTON_RSTICK_DOWN);
  InputManager.SetGameJoypadMaps(MAP_STYLE_0, ACTION_MENU_OTHER_STICK_LEFT, _360_JOY_BUTTON_RSTICK_LEFT);
  InputManager.SetGameJoypadMaps(MAP_STYLE_0, ACTION_MENU_OTHER_STICK_RIGHT, _360_JOY_BUTTON_RSTICK_RIGHT);
  InputManager.SetGameJoypadMaps(MAP_STYLE_0, MINECRAFT_ACTION_JUMP, _360_JOY_BUTTON_A);
  InputManager.SetGameJoypadMaps(MAP_STYLE_0, MINECRAFT_ACTION_FORWARD, _360_JOY_BUTTON_LSTICK_UP);
  InputManager.SetGameJoypadMaps(MAP_STYLE_0, MINECRAFT_ACTION_BACKWARD, _360_JOY_BUTTON_LSTICK_DOWN);
  InputManager.SetGameJoypadMaps(MAP_STYLE_0, MINECRAFT_ACTION_LEFT, _360_JOY_BUTTON_LSTICK_LEFT);
  InputManager.SetGameJoypadMaps(MAP_STYLE_0, MINECRAFT_ACTION_RIGHT, _360_JOY_BUTTON_LSTICK_RIGHT);
  InputManager.SetGameJoypadMaps(MAP_STYLE_0, MINECRAFT_ACTION_LOOK_LEFT, _360_JOY_BUTTON_RSTICK_LEFT);
  InputManager.SetGameJoypadMaps(MAP_STYLE_0, MINECRAFT_ACTION_LOOK_RIGHT, _360_JOY_BUTTON_RSTICK_RIGHT);
  InputManager.SetGameJoypadMaps(MAP_STYLE_0, MINECRAFT_ACTION_LOOK_UP, _360_JOY_BUTTON_RSTICK_UP);
  InputManager.SetGameJoypadMaps(MAP_STYLE_0, MINECRAFT_ACTION_LOOK_DOWN, _360_JOY_BUTTON_RSTICK_DOWN);
  InputManager.SetGameJoypadMaps(MAP_STYLE_0, MINECRAFT_ACTION_USE, _360_JOY_BUTTON_LT);
  InputManager.SetGameJoypadMaps(MAP_STYLE_0, MINECRAFT_ACTION_ACTION, _360_JOY_BUTTON_RT);
  InputManager.SetGameJoypadMaps(MAP_STYLE_0, MINECRAFT_ACTION_RIGHT_SCROLL, _360_JOY_BUTTON_RB);
  InputManager.SetGameJoypadMaps(MAP_STYLE_0, MINECRAFT_ACTION_LEFT_SCROLL, _360_JOY_BUTTON_LB);
  InputManager.SetGameJoypadMaps(MAP_STYLE_0, MINECRAFT_ACTION_INVENTORY, _360_JOY_BUTTON_Y);
  InputManager.SetGameJoypadMaps(MAP_STYLE_0, MINECRAFT_ACTION_PAUSEMENU, _360_JOY_BUTTON_START);
  InputManager.SetGameJoypadMaps(MAP_STYLE_0, MINECRAFT_ACTION_DROP, _360_JOY_BUTTON_B);
  InputManager.SetGameJoypadMaps(MAP_STYLE_0, MINECRAFT_ACTION_SNEAK_TOGGLE, _360_JOY_BUTTON_RTHUMB);
  InputManager.SetGameJoypadMaps(MAP_STYLE_0, MINECRAFT_ACTION_CRAFTING, _360_JOY_BUTTON_X);
  InputManager.SetGameJoypadMaps(MAP_STYLE_0, MINECRAFT_ACTION_RENDER_THIRD_PERSON, _360_JOY_BUTTON_LTHUMB);
  InputManager.SetGameJoypadMaps(MAP_STYLE_0, MINECRAFT_ACTION_GAME_INFO, _360_JOY_BUTTON_BACK);
  InputManager.SetGameJoypadMaps(MAP_STYLE_0, MINECRAFT_ACTION_DPAD_LEFT, _360_JOY_BUTTON_DPAD_LEFT);
  InputManager.SetGameJoypadMaps(MAP_STYLE_0, MINECRAFT_ACTION_DPAD_RIGHT, _360_JOY_BUTTON_DPAD_RIGHT);
  InputManager.SetGameJoypadMaps(MAP_STYLE_0, MINECRAFT_ACTION_DPAD_UP, _360_JOY_BUTTON_DPAD_UP);
  InputManager.SetGameJoypadMaps(MAP_STYLE_0, MINECRAFT_ACTION_DPAD_DOWN, _360_JOY_BUTTON_DPAD_DOWN);

  InputManager.SetGameJoypadMaps(MAP_STYLE_1, ACTION_MENU_A, _360_JOY_BUTTON_A);
  InputManager.SetGameJoypadMaps(MAP_STYLE_1, ACTION_MENU_B, _360_JOY_BUTTON_B);
  InputManager.SetGameJoypadMaps(MAP_STYLE_1, ACTION_MENU_X, _360_JOY_BUTTON_X);
  InputManager.SetGameJoypadMaps(MAP_STYLE_1, ACTION_MENU_Y, _360_JOY_BUTTON_Y);
  InputManager.SetGameJoypadMaps(MAP_STYLE_1, ACTION_MENU_OK, _360_JOY_BUTTON_A);
  InputManager.SetGameJoypadMaps(MAP_STYLE_1, ACTION_MENU_CANCEL, _360_JOY_BUTTON_B);
  InputManager.SetGameJoypadMaps(MAP_STYLE_1, ACTION_MENU_UP, _360_JOY_BUTTON_DPAD_UP | _360_JOY_BUTTON_LSTICK_UP);
  InputManager.SetGameJoypadMaps(MAP_STYLE_1, ACTION_MENU_DOWN, _360_JOY_BUTTON_DPAD_DOWN | _360_JOY_BUTTON_LSTICK_DOWN);
  InputManager.SetGameJoypadMaps(MAP_STYLE_1, ACTION_MENU_LEFT, _360_JOY_BUTTON_DPAD_LEFT | _360_JOY_BUTTON_LSTICK_LEFT);
  InputManager.SetGameJoypadMaps(MAP_STYLE_1, ACTION_MENU_RIGHT, _360_JOY_BUTTON_DPAD_RIGHT | _360_JOY_BUTTON_LSTICK_RIGHT);
  InputManager.SetGameJoypadMaps(MAP_STYLE_1, ACTION_MENU_PAGEUP, _360_JOY_BUTTON_LB);
  InputManager.SetGameJoypadMaps(MAP_STYLE_1, ACTION_MENU_PAGEDOWN, _360_JOY_BUTTON_RT);
  InputManager.SetGameJoypadMaps(MAP_STYLE_1, ACTION_MENU_RIGHT_SCROLL, _360_JOY_BUTTON_RB);
  InputManager.SetGameJoypadMaps(MAP_STYLE_1, ACTION_MENU_LEFT_SCROLL, _360_JOY_BUTTON_LB);
  InputManager.SetGameJoypadMaps(MAP_STYLE_1, ACTION_MENU_PAUSEMENU, _360_JOY_BUTTON_START);
  InputManager.SetGameJoypadMaps(MAP_STYLE_1, ACTION_MENU_STICK_PRESS, _360_JOY_BUTTON_LTHUMB);
  InputManager.SetGameJoypadMaps(MAP_STYLE_1, ACTION_MENU_OTHER_STICK_PRESS, _360_JOY_BUTTON_RTHUMB);
  InputManager.SetGameJoypadMaps(MAP_STYLE_1, ACTION_MENU_OTHER_STICK_UP, _360_JOY_BUTTON_RSTICK_UP);
  InputManager.SetGameJoypadMaps(MAP_STYLE_1, ACTION_MENU_OTHER_STICK_DOWN, _360_JOY_BUTTON_RSTICK_DOWN);
  InputManager.SetGameJoypadMaps(MAP_STYLE_1, ACTION_MENU_OTHER_STICK_LEFT, _360_JOY_BUTTON_RSTICK_LEFT);
  InputManager.SetGameJoypadMaps(MAP_STYLE_1, ACTION_MENU_OTHER_STICK_RIGHT, _360_JOY_BUTTON_RSTICK_RIGHT);
  InputManager.SetGameJoypadMaps(MAP_STYLE_1, MINECRAFT_ACTION_JUMP, _360_JOY_BUTTON_RB);
  InputManager.SetGameJoypadMaps(MAP_STYLE_1, MINECRAFT_ACTION_FORWARD, _360_JOY_BUTTON_LSTICK_UP);
  InputManager.SetGameJoypadMaps(MAP_STYLE_1, MINECRAFT_ACTION_BACKWARD, _360_JOY_BUTTON_LSTICK_DOWN);
  InputManager.SetGameJoypadMaps(MAP_STYLE_1, MINECRAFT_ACTION_LEFT, _360_JOY_BUTTON_LSTICK_LEFT);
  InputManager.SetGameJoypadMaps(MAP_STYLE_1, MINECRAFT_ACTION_RIGHT, _360_JOY_BUTTON_LSTICK_RIGHT);
  InputManager.SetGameJoypadMaps(MAP_STYLE_1, MINECRAFT_ACTION_LOOK_LEFT, _360_JOY_BUTTON_RSTICK_LEFT);
  InputManager.SetGameJoypadMaps(MAP_STYLE_1, MINECRAFT_ACTION_LOOK_RIGHT, _360_JOY_BUTTON_RSTICK_RIGHT);
  InputManager.SetGameJoypadMaps(MAP_STYLE_1, MINECRAFT_ACTION_LOOK_UP, _360_JOY_BUTTON_RSTICK_UP);
  InputManager.SetGameJoypadMaps(MAP_STYLE_1, MINECRAFT_ACTION_LOOK_DOWN, _360_JOY_BUTTON_RSTICK_DOWN);
  InputManager.SetGameJoypadMaps(MAP_STYLE_1, MINECRAFT_ACTION_USE, _360_JOY_BUTTON_RT);
  InputManager.SetGameJoypadMaps(MAP_STYLE_1, MINECRAFT_ACTION_ACTION, _360_JOY_BUTTON_LT);
  InputManager.SetGameJoypadMaps(MAP_STYLE_1, MINECRAFT_ACTION_RIGHT_SCROLL, _360_JOY_BUTTON_DPAD_RIGHT);
  InputManager.SetGameJoypadMaps(MAP_STYLE_1, MINECRAFT_ACTION_LEFT_SCROLL, _360_JOY_BUTTON_DPAD_LEFT);
  InputManager.SetGameJoypadMaps(MAP_STYLE_1, MINECRAFT_ACTION_INVENTORY, _360_JOY_BUTTON_Y);
  InputManager.SetGameJoypadMaps(MAP_STYLE_1, MINECRAFT_ACTION_PAUSEMENU, _360_JOY_BUTTON_START);
  InputManager.SetGameJoypadMaps(MAP_STYLE_1, MINECRAFT_ACTION_DROP, _360_JOY_BUTTON_B);
  InputManager.SetGameJoypadMaps(MAP_STYLE_1, MINECRAFT_ACTION_SNEAK_TOGGLE, _360_JOY_BUTTON_LTHUMB);
  InputManager.SetGameJoypadMaps(MAP_STYLE_1, MINECRAFT_ACTION_CRAFTING, _360_JOY_BUTTON_X);
  InputManager.SetGameJoypadMaps(MAP_STYLE_1, MINECRAFT_ACTION_RENDER_THIRD_PERSON, _360_JOY_BUTTON_RTHUMB);
  InputManager.SetGameJoypadMaps(MAP_STYLE_1, MINECRAFT_ACTION_GAME_INFO, _360_JOY_BUTTON_BACK);
  InputManager.SetGameJoypadMaps(MAP_STYLE_1, MINECRAFT_ACTION_DPAD_LEFT, _360_JOY_BUTTON_DPAD_LEFT);
  InputManager.SetGameJoypadMaps(MAP_STYLE_1, MINECRAFT_ACTION_DPAD_RIGHT, _360_JOY_BUTTON_DPAD_RIGHT);
  InputManager.SetGameJoypadMaps(MAP_STYLE_1, MINECRAFT_ACTION_DPAD_UP, _360_JOY_BUTTON_DPAD_UP);
  InputManager.SetGameJoypadMaps(MAP_STYLE_1, MINECRAFT_ACTION_DPAD_DOWN, _360_JOY_BUTTON_DPAD_DOWN);

  InputManager.SetGameJoypadMaps(MAP_STYLE_2, ACTION_MENU_A, _360_JOY_BUTTON_A);
  InputManager.SetGameJoypadMaps(MAP_STYLE_2, ACTION_MENU_B, _360_JOY_BUTTON_B);
  InputManager.SetGameJoypadMaps(MAP_STYLE_2, ACTION_MENU_X, _360_JOY_BUTTON_X);
  InputManager.SetGameJoypadMaps(MAP_STYLE_2, ACTION_MENU_Y, _360_JOY_BUTTON_Y);
  InputManager.SetGameJoypadMaps(MAP_STYLE_2, ACTION_MENU_OK, _360_JOY_BUTTON_A);
  InputManager.SetGameJoypadMaps(MAP_STYLE_2, ACTION_MENU_CANCEL, _360_JOY_BUTTON_B);
  InputManager.SetGameJoypadMaps(MAP_STYLE_2, ACTION_MENU_UP, _360_JOY_BUTTON_DPAD_UP | _360_JOY_BUTTON_LSTICK_UP);
  InputManager.SetGameJoypadMaps(MAP_STYLE_2, ACTION_MENU_DOWN, _360_JOY_BUTTON_DPAD_DOWN | _360_JOY_BUTTON_LSTICK_DOWN);
  InputManager.SetGameJoypadMaps(MAP_STYLE_2, ACTION_MENU_LEFT, _360_JOY_BUTTON_DPAD_LEFT | _360_JOY_BUTTON_LSTICK_LEFT);
  InputManager.SetGameJoypadMaps(MAP_STYLE_2, ACTION_MENU_RIGHT, _360_JOY_BUTTON_DPAD_RIGHT | _360_JOY_BUTTON_LSTICK_RIGHT);
  InputManager.SetGameJoypadMaps(MAP_STYLE_2, ACTION_MENU_PAGEUP, _360_JOY_BUTTON_DPAD_UP | _360_JOY_BUTTON_LB);
  InputManager.SetGameJoypadMaps(MAP_STYLE_2, ACTION_MENU_PAGEDOWN, _360_JOY_BUTTON_RT);
  InputManager.SetGameJoypadMaps(MAP_STYLE_2, ACTION_MENU_RIGHT_SCROLL, _360_JOY_BUTTON_RB);
  InputManager.SetGameJoypadMaps(MAP_STYLE_2, ACTION_MENU_LEFT_SCROLL, _360_JOY_BUTTON_LB);
  InputManager.SetGameJoypadMaps(MAP_STYLE_2, MINECRAFT_ACTION_JUMP, _360_JOY_BUTTON_LT);
  InputManager.SetGameJoypadMaps(MAP_STYLE_2, MINECRAFT_ACTION_FORWARD, _360_JOY_BUTTON_LSTICK_UP);
  InputManager.SetGameJoypadMaps(MAP_STYLE_2, MINECRAFT_ACTION_BACKWARD, _360_JOY_BUTTON_LSTICK_DOWN);
  InputManager.SetGameJoypadMaps(MAP_STYLE_2, MINECRAFT_ACTION_LEFT, _360_JOY_BUTTON_LSTICK_LEFT);
  InputManager.SetGameJoypadMaps(MAP_STYLE_2, MINECRAFT_ACTION_RIGHT, _360_JOY_BUTTON_LSTICK_RIGHT);
  InputManager.SetGameJoypadMaps(MAP_STYLE_2, MINECRAFT_ACTION_LOOK_LEFT, _360_JOY_BUTTON_RSTICK_LEFT);
  InputManager.SetGameJoypadMaps(MAP_STYLE_2, MINECRAFT_ACTION_LOOK_RIGHT, _360_JOY_BUTTON_RSTICK_RIGHT);
  InputManager.SetGameJoypadMaps(MAP_STYLE_2, MINECRAFT_ACTION_LOOK_UP, _360_JOY_BUTTON_RSTICK_UP);
  InputManager.SetGameJoypadMaps(MAP_STYLE_2, MINECRAFT_ACTION_LOOK_DOWN, _360_JOY_BUTTON_RSTICK_DOWN);
  InputManager.SetGameJoypadMaps(MAP_STYLE_2, MINECRAFT_ACTION_USE, _360_JOY_BUTTON_RT);
  InputManager.SetGameJoypadMaps(MAP_STYLE_2, MINECRAFT_ACTION_ACTION, _360_JOY_BUTTON_A);
  InputManager.SetGameJoypadMaps(MAP_STYLE_2, MINECRAFT_ACTION_RIGHT_SCROLL, _360_JOY_BUTTON_DPAD_RIGHT);
  InputManager.SetGameJoypadMaps(MAP_STYLE_2, MINECRAFT_ACTION_LEFT_SCROLL, _360_JOY_BUTTON_DPAD_LEFT);
  InputManager.SetGameJoypadMaps(MAP_STYLE_2, MINECRAFT_ACTION_INVENTORY, _360_JOY_BUTTON_Y);
  InputManager.SetGameJoypadMaps(MAP_STYLE_2, MINECRAFT_ACTION_PAUSEMENU, _360_JOY_BUTTON_START);
  InputManager.SetGameJoypadMaps(MAP_STYLE_2, MINECRAFT_ACTION_DROP, _360_JOY_BUTTON_B);
  InputManager.SetGameJoypadMaps(MAP_STYLE_2, MINECRAFT_ACTION_SNEAK_TOGGLE, _360_JOY_BUTTON_LB);
  InputManager.SetGameJoypadMaps(MAP_STYLE_2, MINECRAFT_ACTION_CRAFTING, _360_JOY_BUTTON_X);
  InputManager.SetGameJoypadMaps(MAP_STYLE_2, MINECRAFT_ACTION_RENDER_THIRD_PERSON, _360_JOY_BUTTON_LTHUMB);
  InputManager.SetGameJoypadMaps(MAP_STYLE_2, MINECRAFT_ACTION_GAME_INFO, _360_JOY_BUTTON_BACK);
  InputManager.SetGameJoypadMaps(MAP_STYLE_2, ACTION_MENU_PAUSEMENU, _360_JOY_BUTTON_START);
  InputManager.SetGameJoypadMaps(MAP_STYLE_2, ACTION_MENU_STICK_PRESS, _360_JOY_BUTTON_LTHUMB);
  InputManager.SetGameJoypadMaps(MAP_STYLE_2, ACTION_MENU_OTHER_STICK_PRESS, _360_JOY_BUTTON_RTHUMB);
  InputManager.SetGameJoypadMaps(MAP_STYLE_2, ACTION_MENU_OTHER_STICK_UP, _360_JOY_BUTTON_RSTICK_UP);
  InputManager.SetGameJoypadMaps(MAP_STYLE_2, ACTION_MENU_OTHER_STICK_DOWN, _360_JOY_BUTTON_RSTICK_DOWN);
  InputManager.SetGameJoypadMaps(MAP_STYLE_2, ACTION_MENU_OTHER_STICK_LEFT, _360_JOY_BUTTON_RSTICK_LEFT);
  InputManager.SetGameJoypadMaps(MAP_STYLE_2, ACTION_MENU_OTHER_STICK_RIGHT, _360_JOY_BUTTON_RSTICK_RIGHT);
  InputManager.SetGameJoypadMaps(MAP_STYLE_2, MINECRAFT_ACTION_DPAD_LEFT, _360_JOY_BUTTON_DPAD_LEFT);
  InputManager.SetGameJoypadMaps(MAP_STYLE_2, MINECRAFT_ACTION_DPAD_RIGHT, _360_JOY_BUTTON_DPAD_RIGHT);
  InputManager.SetGameJoypadMaps(MAP_STYLE_2, MINECRAFT_ACTION_DPAD_UP, _360_JOY_BUTTON_DPAD_UP);
  InputManager.SetGameJoypadMaps(MAP_STYLE_2, MINECRAFT_ACTION_DPAD_DOWN, _360_JOY_BUTTON_DPAD_DOWN);
}

int main()
{
  signal(SIGSEGV, segfault_handler);
  signal(SIGABRT, segfault_handler);
  signal(SIGBUS, segfault_handler);
  try
  {
    std::filesystem::current_path(std::filesystem::path(MCE_CLIENT_ROOT));
    std::cerr << "[MCE] Working directory: " << std::filesystem::current_path() << '\n';

    // ---- Window & Vulkan ----
    VulkanBootstrapApp::initialiseGlfw();
    GLFWwindow *window = VulkanBootstrapApp::createWindow("Minecraft Community Edition");

    int winW = 0, winH = 0;
    glfwGetWindowSize(window, &winW, &winH);
    if (winW <= 0) winW = 1280;
    if (winH <= 0) winH = 720;
    g_iScreenWidth = winW;
    g_iScreenHeight = winH;

    RenderManager.InitialiseForWindow(window);

    const float clearColour[4] = {0.05f, 0.06f, 0.09f, 1.0f};
    RenderManager.SetClearColour(clearColour);

    app.loadMediaArchive();
    app.loadStringTable();
    initialisePlayerIdentity();

    // ---- Profile manager ----
    ProfileManager.Initialise(
      TITLEID_MINECRAFT,
      app.m_dwOfferID,
      PROFILE_VERSION_10,
      NUM_PROFILE_VALUES,
      NUM_PROFILE_SETTINGS,
      dwProfileSettingsA,
      app.GAME_DEFINED_PROFILE_DATA_BYTES * XUSER_MAX_COUNT,
      &app.uiGameDefinedDataChangedBitmask);
    ProfileManager.SetPrimaryPad(0);
    ProfileManager.SetDebugFullOverride(true);

    // ---- Network ----
    g_NetworkManager.Initialise();

    // ---- UI controller ----
    ui.init(winW, winH);

    // ---- Input ----
    InputManager.Initialise(1, 3, MINECRAFT_ACTION_MAX, ACTION_MAX_MENU);
    DefineActions();
    InputManager.SetJoypadMapVal(0, 0);
    InputManager.SetKeyRepeatRate(0.3f, 0.2f);

    // ---- Thread-local storage for game subsystems ----
    Tesselator::CreateNewThreadStorage(1024 * 1024);
    AABB::CreateNewThreadStorage();
    Vec3::CreateNewThreadStorage();
    IntCache::CreateNewThreadStorage();
    Compression::CreateNewThreadStorage();
    OldChunkStorage::CreateNewThreadStorage();
    Level::enableLightingCache();
    Tile::CreateNewThreadStorage();

    // ---- Start the game ----
    // Minecraft::main() runs static constructors, then calls
    // Minecraft::start() → new Minecraft() → run() → init()
    std::cerr << "[MCE] Calling Minecraft::main()...\n";
    Minecraft::main();

    Minecraft *pMinecraft = Minecraft::GetInstance();
    if (pMinecraft == nullptr)
    {
      std::cerr << "[MCE] FATAL: Minecraft::GetInstance() returned null\n";
      return 1;
    }

    if (pMinecraft->user != nullptr)
    {
      pMinecraft->user->name = convStringToWstring(ProfileManager.GetGamertag(ProfileManager.GetPrimaryPad()));
    }

    app.InitGameSettings();

    // Mouse + keyboard input
    g_pMinecraft = pMinecraft;
    g_appleWindow = window;
	    glfwSetMouseButtonCallback(window, glfwMouseButtonCallbackUI);
	    glfwSetCursorPosCallback(window, glfwCursorPosCallback);
	    glfwSetKeyCallback(window, glfwKeyCallback);
	    glfwSetCharCallback(window, glfwCharCallback);

    // Console UI flow: postInit() navigates to eUIScene_SaveMessage first,
    // which auto-advances to eUIScene_MainMenu after a brief delay.

    // Default sound levels
    pMinecraft->options->set(Options::Option::MUSIC, 1.0f);
    pMinecraft->options->set(Options::Option::SOUND, 1.0f);

    std::cerr << "[MCE] Entering main loop...\n";

    // ---- Main game loop (mirrors Windows64 frame loop) ----
    bool lastGameStarted = app.GetGameStarted();
    while (!glfwWindowShouldClose(window) && pMinecraft->running)
    {
      AppleKeyboard_ClearEdges();
      // Note: mouse deltas are NOT cleared here — they accumulate across frames
      // and are cleared in Input::tick() after being consumed (20 tps < frame rate).
      glfwPollEvents();

      RenderManager.StartFrame();

      app.UpdateTime();
      InputManager.Tick();
      StorageManager.Tick();
      RenderManager.Tick();
      g_NetworkManager.DoWork();

      // Update window dimensions each frame (handles resize)
      glfwGetWindowSize(window, &winW, &winH);
      if (winW <= 0) winW = 1280;
      if (winH <= 0) winH = 720;
      g_iScreenWidth = winW;
      g_iScreenHeight = winH;

      const bool gameStarted = app.GetGameStarted();
      if (gameStarted != lastGameStarted)
      {
        std::cerr << "[MCE] game-started changed: " << (gameStarted ? 1 : 0)
                  << " level=" << pMinecraft->level
                  << " player=" << pMinecraft->player.get()
                  << " camera=" << pMinecraft->cameraTargetPlayer.get()
                  << " screen=" << pMinecraft->screen
                  << '\n';
        lastGameStarted = gameStarted;
      }

      if (gameStarted)
      {
        pMinecraft->run_middle();
        bool isPaused = ui.IsPauseMenuDisplayed(ProfileManager.GetPrimaryPad());
        app.SetAppPaused(
          g_NetworkManager.IsLocalGame() &&
          g_NetworkManager.GetPlayerCount() == 1 &&
          isPaused);

        // Auto-grab mouse when in-game with no menu displayed
        bool menuDisplayed = ui.GetMenuDisplayed(ProfileManager.GetPrimaryPad());
        if (!menuDisplayed && !g_appleMouseGrabbed)
          AppleMouse_SetGrabbed(true);
        else if (menuDisplayed && g_appleMouseGrabbed)
          AppleMouse_SetGrabbed(false);
      }
      else
      {
        // Release mouse when not in game
        if (g_appleMouseGrabbed)
          AppleMouse_SetGrabbed(false);

        if (pMinecraft->soundEngine)
          pMinecraft->soundEngine->tick(NULL, 0.0f);
        if (pMinecraft->textures)
          pMinecraft->textures->tick(true, false);
        IntCache::Reset();
        if (app.GetReallyChangingSessionType())
          pMinecraft->tickAllConnections();

        // Pre-game frame: UI scenes draw via IggyPlayerDraw → Tesselator → Vulkan.
        // No manual clear needed — Vulkan render pass clears to clearColour.
      }

      if (pMinecraft->soundEngine)
        pMinecraft->soundEngine->playMusicTick();

      ui.tick();
      ui.render();

      RenderManager.Present();

      ui.CheckMenuDisplayed();
      app.HandleXuiActions();
    }

    // ---- Cleanup ----
    std::cerr << "[MCE] Shutting down...\n";
    pMinecraft->run_end();
    ui.shutdown();
    RenderManager.Shutdown();
    glfwDestroyWindow(window);
    VulkanBootstrapApp::terminateGlfw();
    return 0;
  }
  catch (const std::exception &exception)
  {
    std::cerr << "[MCE] Fatal exception: " << exception.what() << '\n';
    return 1;
  }
}
