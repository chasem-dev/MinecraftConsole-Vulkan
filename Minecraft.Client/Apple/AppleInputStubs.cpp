#include "stdafx.h"

#include "../Windows64/4JLibs/inc/4J_Input.h"

#include <array>
#include <cstring>

namespace
{
constexpr int kMaxPads = XUSER_MAX_COUNT + 8;
constexpr int kMaxMaps = 8;
constexpr int kMaxActions = 256;

std::array<std::array<unsigned int, kMaxActions>, kMaxMaps> g_actionMaps{};
std::array<unsigned char, kMaxPads> g_padMaps{};
std::array<bool, kMaxPads> g_menuDisplayed{};
float g_repeatDelaySeconds = 0.3f;
float g_repeatRateSeconds = 0.2f;
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

EKeyboardResult C_4JInput::RequestKeyboard(LPCWSTR, LPCWSTR, DWORD, UINT, int (*)(LPVOID, const bool), LPVOID, C_4JInput::EKeyboardMode)
{
  return EKeyboard_Cancelled;
}

void C_4JInput::GetText(uint16_t *utf16String)
{
  if (utf16String != NULL)
  {
    utf16String[0] = 0;
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
