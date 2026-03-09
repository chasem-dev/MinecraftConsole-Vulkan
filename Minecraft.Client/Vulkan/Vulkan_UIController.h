#pragma once

#include "../Common/UI/UIController.h"

class ConsoleUIController : public UIController
{
public:
  static __int64 iggyAllocCount;

  void init(S32 w, S32 h);
  void tick();
  void render();
  void shutdown();

  void ReloadSkin();
  void StartReloadSkinThread();
  bool IsReloadingSkin();
  bool IsExpectingOrReloadingSkin();
  void CleanUpSkinReload();

  byteArray getMovieData(const wchar_t *filename);
  float getScreenWidth();
  float getScreenHeight();
  void getRenderDimensions(C4JRender::eViewportType viewport, S32 &width, S32 &height);
  void setupRenderPosition(C4JRender::eViewportType viewport);
  void setupRenderPosition(S32 xOrigin, S32 yOrigin);

  void beginIggyCustomDraw4J(IggyCustomDrawCallbackRegion *region, CustomDrawData *customDrawRegion) override;
  CustomDrawData *setupCustomDraw(UIScene *scene, IggyCustomDrawCallbackRegion *region) override;
  CustomDrawData *calculateCustomDraw(IggyCustomDrawCallbackRegion *region) override;
  void endCustomDraw(IggyCustomDrawCallbackRegion *region) override;
  void setupCustomDrawGameState();
  void setupCustomDrawMatrices(UIScene *scene, CustomDrawData *customDrawRegion);

  bool NavigateToScene(int iPad, EUIScene scene, void *initData = NULL, EUILayer layer = eUILayer_Scene, EUIGroup group = eUIGroup_PAD);
  bool NavigateBack(int iPad, bool forceUsePad = false, EUIScene eScene = eUIScene_COUNT, EUILayer eLayer = eUILayer_COUNT);
  void NavigateToHomeMenu();
  UIScene *GetTopScene(int iPad, EUILayer layer = eUILayer_Scene, EUIGroup group = eUIGroup_PAD);

  size_t RegisterForCallbackId(UIScene *scene);
  void UnregisterCallbackId(size_t id);
  UIScene *GetSceneFromCallbackId(size_t id);
  void EnterCallbackIdCriticalSection();
  void LeaveCallbackIdCriticalSection();

  void registerSubstitutionTexture(const wstring &textureName, PBYTE pbData, DWORD dwLength);
  void unregisterSubstitutionTexture(const wstring &textureName, bool deleteData);

  void CloseAllPlayersScenes();
  void CloseUIScenes(int iPad, bool forceIPad = false);

  bool IsPauseMenuDisplayed(int iPad);
  bool IsContainerMenuDisplayed(int iPad);
  bool IsIgnorePlayerJoinMenuDisplayed(int iPad);
  bool IsIgnoreAutosaveMenuDisplayed(int iPad);
  void SetIgnoreAutosaveMenuDisplayed(int iPad, bool displayed);
  bool IsSceneInStack(int iPad, EUIScene eScene);
  bool GetMenuDisplayed(int iPad);
  void CheckMenuDisplayed();
  void AnimateKeyPress(int iPad, int iAction, bool bRepeat, bool bPressed, bool bReleased);
  void OverrideSFX(int iPad, int iAction, bool bVal);

  void SetTooltipText(unsigned int iPad, unsigned int tooltip, int iTextID);
  void SetEnableTooltips(unsigned int iPad, BOOL bVal);
  void ShowTooltip(unsigned int iPad, unsigned int tooltip, bool show);
  void SetTooltips(unsigned int iPad, int iA, int iB = -1, int iX = -1, int iY = -1, int iLT = -1, int iRT = -1, int iLB = -1, int iRB = -1, int iLS = -1, bool forceUpdate = false);
  void RefreshTooltips(unsigned int iPad);

  void PlayUISFX(ESoundEffect eSound);
  void DisplayGamertag(unsigned int iPad, bool show);
  void SetSelectedItem(unsigned int iPad, const wstring &name);
  void UpdateSelectedItemPos(unsigned int iPad);

  void HandleDLCMountingComplete();
  void HandleDLCInstalled(int iPad);
  void HandleInventoryUpdated(int iPad);
  void HandleGameTick();

  void SetTutorial(int iPad, Tutorial *tutorial);
  void SetTutorialDescription(int iPad, TutorialPopupInfo *info);
  void RemoveInteractSceneReference(int iPad, UIScene *scene);
  void SetTutorialVisible(int iPad, bool visible);
  bool IsTutorialVisible(int iPad);

  void UpdatePlayerBasePositions();
  void SetEmptyQuadrantLogo(int iSection);
  void HideAllGameUIElements();
  void ShowOtherPlayersBaseScene(unsigned int iPad, bool show);

  void ShowTrialTimer(bool show);
  void SetTrialTimerLimitSecs(unsigned int uiSeconds);
  void ReduceTrialTimerValue();
  void ShowAutosaveCountdownTimer(bool show);
  void UpdateAutosaveCountdownTimer(unsigned int uiSeconds);
  void ShowSavingMessage(unsigned int iPad, C4JStorage::ESavingMessage eVal);
  void ShowPlayerDisplayname(bool show);
  bool PressStartPlaying(unsigned int iPad);
  void ShowPressStart(unsigned int iPad);
  void HidePressStart();
  void ClearPressStart();

  void SetWinUserIndex(unsigned int iPad);
  unsigned int GetWinUserIndex();

  void logDebugString(const string &text);
  UIScene *FindScene(EUIScene sceneType);
  void setFontCachingCalculationBuffer(int length);
  void TouchBoxAdd(UIControl *pControl, UIScene *pUIScene);

  GDrawTexture *getSubstitutionTexture(int textureId) override;
  void destroySubstitutionTexture(void *destroyCallBackData, GDrawTexture *handle) override;

protected:
  void setTileOrigin(S32 xPos, S32 yPos) override;
};

extern ConsoleUIController ui;
