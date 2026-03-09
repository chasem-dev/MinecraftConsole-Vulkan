#include "stdafx.h"

#include "../Windows64/4JLibs/inc/4J_Input.h"

#include <array>
#include <cstring>
#include <string>

namespace
{
constexpr int kMaxPads = XUSER_MAX_COUNT + 8;
constexpr int kMaxMaps = 8;
constexpr int kMaxActions = 256;
constexpr int kGlfwKeyEnter = 257;
constexpr int kGlfwKeyEscape = 256;
constexpr int kGlfwKeyBackspace = 259;

std::array<std::array<unsigned int, kMaxActions>, kMaxMaps> g_actionMaps{};
std::array<unsigned char, kMaxPads> g_padMaps{};
std::array<bool, kMaxPads> g_menuDisplayed{};
float g_repeatDelaySeconds = 0.3f;
float g_repeatRateSeconds = 0.2f;

bool g_keyboardActive = false;
std::wstring g_keyboardTitle;
std::wstring g_keyboardText;
std::wstring g_lastKeyboardText;
UINT g_keyboardMaxChars = 0;
int (*g_keyboardCallback)(LPVOID, const bool) = nullptr;
LPVOID g_keyboardCallbackParam = nullptr;

void finishKeyboardRequest(bool accepted)
{
  if (!g_keyboardActive)
  {
    return;
  }

  if (accepted)
  {
    g_lastKeyboardText = g_keyboardText;
  }

  g_keyboardActive = false;

  int (*callback)(LPVOID, const bool) = g_keyboardCallback;
  LPVOID callbackParam = g_keyboardCallbackParam;
  g_keyboardCallback = nullptr;
  g_keyboardCallbackParam = nullptr;

  if (callback != nullptr)
  {
    callback(callbackParam, accepted);
  }
}
}

C_4JInput InputManager;

void C_4JInput::Initialise(int, unsigned char, unsigned char, unsigned char)
{
  for (std::array<unsigned int, kMaxActions> &map : g_actionMaps)
  {
    map.fill(0);
  }
  g_padMaps.fill(0);
  g_menuDisplayed.fill(false);
}

void C_4JInput::Tick(void)
{
}

void C_4JInput::SetDeadzoneAndMovementRange(unsigned int, unsigned int)
{
}

void C_4JInput::SetGameJoypadMaps(unsigned char ucMap, unsigned char ucAction, unsigned int uiActionVal)
{
  if (ucMap < kMaxMaps)
  {
    g_actionMaps[ucMap][ucAction] = uiActionVal;
  }
}

unsigned int C_4JInput::GetGameJoypadMaps(unsigned char ucMap, unsigned char ucAction)
{
  if (ucMap < kMaxMaps)
  {
    return g_actionMaps[ucMap][ucAction];
  }
  return 0;
}

void C_4JInput::SetJoypadMapVal(int iPad, unsigned char ucMap)
{
  if (iPad >= 0 && iPad < kMaxPads)
  {
    g_padMaps[iPad] = ucMap;
  }
}

unsigned char C_4JInput::GetJoypadMapVal(int iPad)
{
  if (iPad >= 0 && iPad < kMaxPads)
  {
    return g_padMaps[iPad];
  }
  return 0;
}

void C_4JInput::SetJoypadSensitivity(int, float)
{
}

unsigned int C_4JInput::GetValue(int, unsigned char, bool)
{
  return 0;
}

bool C_4JInput::ButtonPressed(int, unsigned char)
{
  return false;
}

bool C_4JInput::ButtonReleased(int, unsigned char)
{
  return false;
}

bool C_4JInput::ButtonDown(int, unsigned char)
{
  return false;
}

void C_4JInput::SetJoypadStickAxisMap(int, unsigned int, unsigned int)
{
}

void C_4JInput::SetJoypadStickTriggerMap(int, unsigned int, unsigned int)
{
}

void C_4JInput::SetKeyRepeatRate(float fRepeatDelaySecs, float fRepeatRateSecs)
{
  g_repeatDelaySeconds = fRepeatDelaySecs;
  g_repeatRateSeconds = fRepeatRateSecs;
}

void C_4JInput::SetDebugSequence(const char *, int (*)(LPVOID), LPVOID)
{
}

FLOAT C_4JInput::GetIdleSeconds(int)
{
  return 0.0f;
}

bool C_4JInput::IsPadConnected(int iPad)
{
  return iPad >= 0 && iPad < XUSER_MAX_COUNT;
}

float C_4JInput::GetJoypadStick_LX(int, bool)
{
  return 0.0f;
}

float C_4JInput::GetJoypadStick_LY(int, bool)
{
  return 0.0f;
}

float C_4JInput::GetJoypadStick_RX(int, bool)
{
  return 0.0f;
}

float C_4JInput::GetJoypadStick_RY(int, bool)
{
  return 0.0f;
}

unsigned char C_4JInput::GetJoypadLTrigger(int, bool)
{
  return 0;
}

unsigned char C_4JInput::GetJoypadRTrigger(int, bool)
{
  return 0;
}

void C_4JInput::SetMenuDisplayed(int iPad, bool bVal)
{
  if (iPad >= 0 && iPad < kMaxPads)
  {
    g_menuDisplayed[iPad] = bVal;
  }
}

EKeyboardResult C_4JInput::RequestKeyboard(
  LPCWSTR Title,
  LPCWSTR Text,
  DWORD,
  UINT uiMaxChars,
  int (*Func)(LPVOID, const bool),
  LPVOID lpParam,
  C_4JInput::EKeyboardMode)
{
  g_keyboardTitle = (Title != nullptr) ? Title : L"";
  g_keyboardText = (Text != nullptr) ? Text : L"";
  g_lastKeyboardText = g_keyboardText;
  g_keyboardMaxChars = uiMaxChars;
  g_keyboardCallback = Func;
  g_keyboardCallbackParam = lpParam;
  g_keyboardActive = true;
  return EKeyboard_Pending;
}

void C_4JInput::GetText(uint16_t *utf16String)
{
  if (utf16String != NULL)
  {
    const std::wstring &source = g_keyboardActive ? g_keyboardText : g_lastKeyboardText;
    size_t i = 0;
    for (; i < source.size(); ++i)
    {
      utf16String[i] = static_cast<uint16_t>(source[i]);
    }
    utf16String[i] = 0;
  }
}

bool C_4JInput::VerifyStrings(WCHAR **, int, int (*)(LPVOID, STRING_VERIFY_RESPONSE *), LPVOID)
{
  return true;
}

void C_4JInput::CancelQueuedVerifyStrings(int (*)(LPVOID, STRING_VERIFY_RESPONSE *), LPVOID)
{
}

void C_4JInput::CancelAllVerifyInProgress(void)
{
}

extern "C" void AppleInput_HandleChar(unsigned int codepoint)
{
  if (!g_keyboardActive)
  {
    return;
  }

  if (codepoint < 32 || codepoint > 0xffff)
  {
    return;
  }

  if (g_keyboardMaxChars > 0 && g_keyboardText.size() >= g_keyboardMaxChars)
  {
    return;
  }

  g_keyboardText.push_back(static_cast<wchar_t>(codepoint));
}

extern "C" void AppleInput_HandleKey(int key, int action)
{
  if (!g_keyboardActive)
  {
    return;
  }

  if (action != 1 && action != 2)
  {
    return;
  }

  if (key == kGlfwKeyBackspace)
  {
    if (!g_keyboardText.empty())
    {
      g_keyboardText.pop_back();
    }
    return;
  }

  if (key == kGlfwKeyEnter)
  {
    finishKeyboardRequest(true);
    return;
  }

  if (key == kGlfwKeyEscape)
  {
    finishKeyboardRequest(false);
  }
}

extern "C" bool AppleInput_IsKeyboardActive()
{
  return g_keyboardActive;
}

extern "C" const wchar_t *AppleInput_GetKeyboardTitle()
{
  return g_keyboardTitle.c_str();
}

extern "C" const wchar_t *AppleInput_GetKeyboardText()
{
  return g_keyboardText.c_str();
}
