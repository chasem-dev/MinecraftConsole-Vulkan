#include "stdafx.h"
#include "Vulkan_UIController.h"

#ifdef __APPLE__
#include "Apple/GDrawGLContext.h"
#include "gdraw_wgl.h"
#endif

#include <algorithm>
#include <cstdio>

namespace
{
void syncIggyAllocCount()
{
  ConsoleUIController::iggyAllocCount = UIController::iggyAllocCount;
}

void setIdentityMatrix(float matrix[16])
{
  std::fill(matrix, matrix + 16, 0.0f);
  matrix[0] = 1.0f;
  matrix[5] = 1.0f;
  matrix[10] = 1.0f;
  matrix[15] = 1.0f;
}
}

ConsoleUIController ui;
__int64 ConsoleUIController::iggyAllocCount = 0;

void ConsoleUIController::init(S32 w, S32 h)
{
  preInit(w, h);

#ifdef __APPLE__
  // Create an offscreen CGL OpenGL context for GDraw/Iggy rendering.
  // The Vulkan window stays as the primary display surface.
  if (GDrawGLContext_Create(w, h) == 0)
  {
    GDrawGLContext_MakeCurrent();

    // Set resource limits (matching other console platforms)
    gdraw_GL_SetResourceLimits(GDRAW_GL_RESOURCE_vertexbuffer, 5000, 16 * 1024 * 1024);
    gdraw_GL_SetResourceLimits(GDRAW_GL_RESOURCE_texture, 5000, 128 * 1024 * 1024);
    gdraw_GL_SetResourceLimits(GDRAW_GL_RESOURCE_rendertarget, 10, 32 * 1024 * 1024);

    gdraw_funcs = gdraw_GL_CreateContext(w, h, 0);
    if (gdraw_funcs != nullptr)
    {
      IggySetGDraw(gdraw_funcs);
      std::fprintf(stderr, "[MCE] GDraw GL initialized — Iggy rendering active\n");
    }
    else
    {
      std::fprintf(stderr, "[MCE] WARNING: gdraw_GL_CreateContext failed — "
                           "UI will not render Flash content\n");
    }
  }
  else
  {
    gdraw_funcs = nullptr;
    std::fprintf(stderr, "[MCE] WARNING: GDrawGLContext_Create failed — "
                         "no GL context for Iggy\n");
  }
#else
  gdraw_funcs = nullptr;
#endif

  postInit();
  syncIggyAllocCount();
}

void ConsoleUIController::tick()
{
  UIController::tick();
  syncIggyAllocCount();
}

void ConsoleUIController::render()
{
  renderScenes();
  syncIggyAllocCount();
}

void ConsoleUIController::shutdown()
{
#ifdef __APPLE__
  if (gdraw_funcs != nullptr)
  {
    GDrawGLContext_MakeCurrent();
    gdraw_GL_DestroyContext();
    gdraw_funcs = nullptr;
  }
  GDrawGLContext_Destroy();
#endif
}

void ConsoleUIController::ReloadSkin()
{
  UIController::ReloadSkin();
  syncIggyAllocCount();
}

void ConsoleUIController::StartReloadSkinThread()
{
  UIController::StartReloadSkinThread();
  syncIggyAllocCount();
}

bool ConsoleUIController::IsReloadingSkin()
{
  return UIController::IsReloadingSkin();
}

bool ConsoleUIController::IsExpectingOrReloadingSkin()
{
  return UIController::IsExpectingOrReloadingSkin();
}

void ConsoleUIController::CleanUpSkinReload()
{
  UIController::CleanUpSkinReload();
  syncIggyAllocCount();
}

byteArray ConsoleUIController::getMovieData(const wchar_t *filename)
{
  byteArray data = UIController::getMovieData(filename != nullptr ? wstring(filename) : wstring());
  syncIggyAllocCount();
  return data;
}

float ConsoleUIController::getScreenWidth()
{
  return UIController::getScreenWidth();
}

float ConsoleUIController::getScreenHeight()
{
  return UIController::getScreenHeight();
}

void ConsoleUIController::getRenderDimensions(C4JRender::eViewportType viewport, S32 &width, S32 &height)
{
  UIController::getRenderDimensions(viewport, width, height);
}

void ConsoleUIController::setupRenderPosition(C4JRender::eViewportType viewport)
{
  UIController::setupRenderPosition(viewport);
}

void ConsoleUIController::setupRenderPosition(S32 xOrigin, S32 yOrigin)
{
  UIController::setupRenderPosition(xOrigin, yOrigin);
}

void ConsoleUIController::beginIggyCustomDraw4J(IggyCustomDrawCallbackRegion *region, CustomDrawData *customDrawRegion)
{
#ifdef __APPLE__
  if (gdraw_funcs != nullptr && region != nullptr && customDrawRegion != nullptr)
  {
    GDrawGLContext_MakeCurrent();
    gdraw_GL_BeginCustomDraw(region, customDrawRegion->mat);
  }
#endif
}

CustomDrawData *ConsoleUIController::setupCustomDraw(UIScene *scene, IggyCustomDrawCallbackRegion *region)
{
  CustomDrawData *customDrawRegion = calculateCustomDraw(region);
  setupCustomDrawGameStateAndMatrices(scene, customDrawRegion);
  return customDrawRegion;
}

CustomDrawData *ConsoleUIController::calculateCustomDraw(IggyCustomDrawCallbackRegion *region)
{
  if (region == nullptr)
  {
    return nullptr;
  }

  CustomDrawData *customDrawRegion = new CustomDrawData();
  customDrawRegion->x0 = region->x0;
  customDrawRegion->x1 = region->x1;
  customDrawRegion->y0 = region->y0;
  customDrawRegion->y1 = region->y1;
  setIdentityMatrix(customDrawRegion->mat);
  return customDrawRegion;
}

void ConsoleUIController::endCustomDraw(IggyCustomDrawCallbackRegion *region)
{
#ifdef __APPLE__
  if (gdraw_funcs != nullptr && region != nullptr)
  {
    gdraw_GL_EndCustomDraw(region);
  }
#endif
  endCustomDrawGameStateAndMatrices();
}

void ConsoleUIController::setupCustomDrawGameState()
{
  UIController::setupCustomDrawGameState();
}

void ConsoleUIController::setupCustomDrawMatrices(UIScene *scene, CustomDrawData *customDrawRegion)
{
  UIController::setupCustomDrawMatrices(scene, customDrawRegion);
}

bool ConsoleUIController::NavigateToScene(int iPad, EUIScene scene, void *initData, EUILayer layer, EUIGroup group)
{
  return UIController::NavigateToScene(iPad, scene, initData, layer, group);
}

bool ConsoleUIController::NavigateBack(int iPad, bool forceUsePad, EUIScene eScene, EUILayer eLayer)
{
  return UIController::NavigateBack(iPad, forceUsePad, eScene, eLayer);
}

void ConsoleUIController::NavigateToHomeMenu()
{
  UIController::NavigateToHomeMenu();
}

UIScene *ConsoleUIController::GetTopScene(int iPad, EUILayer layer, EUIGroup group)
{
  return UIController::GetTopScene(iPad, layer, group);
}

size_t ConsoleUIController::RegisterForCallbackId(UIScene *scene)
{
  return UIController::RegisterForCallbackId(scene);
}

void ConsoleUIController::UnregisterCallbackId(size_t id)
{
  UIController::UnregisterCallbackId(id);
}

UIScene *ConsoleUIController::GetSceneFromCallbackId(size_t id)
{
  return UIController::GetSceneFromCallbackId(id);
}

void ConsoleUIController::EnterCallbackIdCriticalSection()
{
  UIController::EnterCallbackIdCriticalSection();
}

void ConsoleUIController::LeaveCallbackIdCriticalSection()
{
  UIController::LeaveCallbackIdCriticalSection();
}

void ConsoleUIController::registerSubstitutionTexture(const wstring &textureName, PBYTE pbData, DWORD dwLength)
{
  UIController::registerSubstitutionTexture(textureName, pbData, dwLength);
}

void ConsoleUIController::unregisterSubstitutionTexture(const wstring &textureName, bool deleteData)
{
  UIController::unregisterSubstitutionTexture(textureName, deleteData);
}

void ConsoleUIController::CloseAllPlayersScenes()
{
  UIController::CloseAllPlayersScenes();
}

void ConsoleUIController::CloseUIScenes(int iPad, bool forceIPad)
{
  UIController::CloseUIScenes(iPad, forceIPad);
}

bool ConsoleUIController::IsPauseMenuDisplayed(int iPad)
{
  return UIController::IsPauseMenuDisplayed(iPad);
}

bool ConsoleUIController::IsContainerMenuDisplayed(int iPad)
{
  return UIController::IsContainerMenuDisplayed(iPad);
}

bool ConsoleUIController::IsIgnorePlayerJoinMenuDisplayed(int iPad)
{
  return UIController::IsIgnorePlayerJoinMenuDisplayed(iPad);
}

bool ConsoleUIController::IsIgnoreAutosaveMenuDisplayed(int iPad)
{
  return UIController::IsIgnoreAutosaveMenuDisplayed(iPad);
}

void ConsoleUIController::SetIgnoreAutosaveMenuDisplayed(int iPad, bool displayed)
{
  UIController::SetIgnoreAutosaveMenuDisplayed(iPad, displayed);
}

bool ConsoleUIController::IsSceneInStack(int iPad, EUIScene eScene)
{
  return UIController::IsSceneInStack(iPad, eScene);
}

bool ConsoleUIController::GetMenuDisplayed(int iPad)
{
  return UIController::GetMenuDisplayed(iPad);
}

void ConsoleUIController::CheckMenuDisplayed()
{
  UIController::CheckMenuDisplayed();
}

void ConsoleUIController::AnimateKeyPress(int iPad, int iAction, bool bRepeat, bool bPressed, bool bReleased)
{
  UIController::AnimateKeyPress(iPad, iAction, bRepeat, bPressed, bReleased);
}

void ConsoleUIController::OverrideSFX(int iPad, int iAction, bool bVal)
{
  UIController::OverrideSFX(iPad, iAction, bVal);
}

void ConsoleUIController::SetTooltipText(unsigned int iPad, unsigned int tooltip, int iTextID)
{
  UIController::SetTooltipText(iPad, tooltip, iTextID);
}

void ConsoleUIController::SetEnableTooltips(unsigned int iPad, BOOL bVal)
{
  UIController::SetEnableTooltips(iPad, bVal);
}

void ConsoleUIController::ShowTooltip(unsigned int iPad, unsigned int tooltip, bool show)
{
  UIController::ShowTooltip(iPad, tooltip, show);
}

void ConsoleUIController::SetTooltips(unsigned int iPad, int iA, int iB, int iX, int iY, int iLT, int iRT, int iLB, int iRB, int iLS, bool forceUpdate)
{
  UIController::SetTooltips(iPad, iA, iB, iX, iY, iLT, iRT, iLB, iRB, iLS, forceUpdate);
}

void ConsoleUIController::RefreshTooltips(unsigned int iPad)
{
  UIController::RefreshTooltips(iPad);
}

void ConsoleUIController::PlayUISFX(ESoundEffect eSound)
{
  UIController::PlayUISFX(eSound);
}

void ConsoleUIController::DisplayGamertag(unsigned int iPad, bool show)
{
  UIController::DisplayGamertag(iPad, show);
}

void ConsoleUIController::SetSelectedItem(unsigned int iPad, const wstring &name)
{
  UIController::SetSelectedItem(iPad, name);
}

void ConsoleUIController::UpdateSelectedItemPos(unsigned int iPad)
{
  UIController::UpdateSelectedItemPos(iPad);
}

void ConsoleUIController::HandleDLCMountingComplete()
{
  UIController::HandleDLCMountingComplete();
}

void ConsoleUIController::HandleDLCInstalled(int iPad)
{
  UIController::HandleDLCInstalled(iPad);
}

void ConsoleUIController::HandleInventoryUpdated(int iPad)
{
  UIController::HandleInventoryUpdated(iPad);
}

void ConsoleUIController::HandleGameTick()
{
  UIController::HandleGameTick();
}

void ConsoleUIController::SetTutorial(int iPad, Tutorial *tutorial)
{
  UIController::SetTutorial(iPad, tutorial);
}

void ConsoleUIController::SetTutorialDescription(int iPad, TutorialPopupInfo *info)
{
  UIController::SetTutorialDescription(iPad, info);
}

void ConsoleUIController::RemoveInteractSceneReference(int iPad, UIScene *scene)
{
  UIController::RemoveInteractSceneReference(iPad, scene);
}

void ConsoleUIController::SetTutorialVisible(int iPad, bool visible)
{
  UIController::SetTutorialVisible(iPad, visible);
}

bool ConsoleUIController::IsTutorialVisible(int iPad)
{
  return UIController::IsTutorialVisible(iPad);
}

void ConsoleUIController::UpdatePlayerBasePositions()
{
  UIController::UpdatePlayerBasePositions();
}

void ConsoleUIController::SetEmptyQuadrantLogo(int iSection)
{
  UIController::SetEmptyQuadrantLogo(iSection);
}

void ConsoleUIController::HideAllGameUIElements()
{
  UIController::HideAllGameUIElements();
}

void ConsoleUIController::ShowOtherPlayersBaseScene(unsigned int iPad, bool show)
{
  UIController::ShowOtherPlayersBaseScene(iPad, show);
}

void ConsoleUIController::ShowTrialTimer(bool show)
{
  UIController::ShowTrialTimer(show);
}

void ConsoleUIController::SetTrialTimerLimitSecs(unsigned int uiSeconds)
{
  UIController::SetTrialTimerLimitSecs(uiSeconds);
}

void ConsoleUIController::ReduceTrialTimerValue()
{
  UIController::ReduceTrialTimerValue();
}

void ConsoleUIController::ShowAutosaveCountdownTimer(bool show)
{
  UIController::ShowAutosaveCountdownTimer(show);
}

void ConsoleUIController::UpdateAutosaveCountdownTimer(unsigned int uiSeconds)
{
  UIController::UpdateAutosaveCountdownTimer(uiSeconds);
}

void ConsoleUIController::ShowSavingMessage(unsigned int iPad, C4JStorage::ESavingMessage eVal)
{
  UIController::ShowSavingMessage(iPad, eVal);
}

void ConsoleUIController::ShowPlayerDisplayname(bool show)
{
  UIController::ShowPlayerDisplayname(show);
}

bool ConsoleUIController::PressStartPlaying(unsigned int iPad)
{
  return UIController::PressStartPlaying(iPad);
}

void ConsoleUIController::ShowPressStart(unsigned int iPad)
{
  UIController::ShowPressStart(iPad);
}

void ConsoleUIController::HidePressStart()
{
  UIController::HidePressStart();
}

void ConsoleUIController::ClearPressStart()
{
  UIController::ClearPressStart();
}

void ConsoleUIController::SetWinUserIndex(unsigned int iPad)
{
  UIController::SetWinUserIndex(iPad);
}

unsigned int ConsoleUIController::GetWinUserIndex()
{
  return UIController::GetWinUserIndex();
}

void ConsoleUIController::logDebugString(const string &text)
{
  UIController::logDebugString(text);
}

UIScene *ConsoleUIController::FindScene(EUIScene sceneType)
{
  return UIController::FindScene(sceneType);
}

void ConsoleUIController::setFontCachingCalculationBuffer(int length)
{
  UIController::setFontCachingCalculationBuffer(length);
}

void ConsoleUIController::TouchBoxAdd(UIControl *, UIScene *)
{
}

void ConsoleUIController::setTileOrigin(S32 xPos, S32 yPos)
{
#ifdef __APPLE__
  if (gdraw_funcs != nullptr)
  {
    gdraw_GL_SetTileOrigin(xPos, yPos, 0);
  }
#endif
}

GDrawTexture *ConsoleUIController::getSubstitutionTexture(int)
{
  return nullptr;
}

void ConsoleUIController::destroySubstitutionTexture(void *, GDrawTexture *)
{
}
