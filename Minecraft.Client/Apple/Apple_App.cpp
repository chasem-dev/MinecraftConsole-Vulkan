#include "stdafx.h"

#include "../Common/Consoles_App.h"
#include "../../Minecraft.Client/Minecraft.h"
#include "../User.h"

CConsoleMinecraftApp app;

CConsoleMinecraftApp::CConsoleMinecraftApp() : CMinecraftApp()
{
}

void CConsoleMinecraftApp::SetRichPresenceContext(int iPad, int contextId)
{
  ProfileManager.SetRichPresenceContextValue(iPad, CONTEXT_GAME_STATE, contextId);
}

void CConsoleMinecraftApp::StoreLaunchData()
{
}

void CConsoleMinecraftApp::ExitGame()
{
  Minecraft *minecraft = Minecraft::GetInstance();
  if (minecraft != NULL)
  {
    minecraft->stop();
  }
}

void CConsoleMinecraftApp::FatalLoadError()
{
  DebugPrintf("Fatal load error.\n");
}

void CConsoleMinecraftApp::CaptureSaveThumbnail()
{
}

void CConsoleMinecraftApp::GetSaveThumbnail(PBYTE *, DWORD *)
{
}

void CConsoleMinecraftApp::ReleaseSaveThumbnail()
{
}

void CConsoleMinecraftApp::GetScreenshot(int, PBYTE *, DWORD *)
{
}

void CConsoleMinecraftApp::TemporaryCreateGameStart()
{
}

int CConsoleMinecraftApp::GetLocalTMSFileIndex(WCHAR *, bool, eFileExtensionType)
{
  return -1;
}

int CConsoleMinecraftApp::LoadLocalTMSFile(WCHAR *)
{
  return -1;
}

int CConsoleMinecraftApp::LoadLocalTMSFile(WCHAR *, eFileExtensionType)
{
  return -1;
}

void CConsoleMinecraftApp::FreeLocalTMSFiles(eTMSFileType)
{
}
