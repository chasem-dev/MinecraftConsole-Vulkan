#include "stdafx.h"
#include "UI.h"
#include "UIScene_SettingsOptionsMenu.h"

#if defined(__APPLE__)
namespace
{
static int clampAppleOptionValue(int value, int minValue, int maxValue)
{
	if(value < minValue)
	{
		return minValue;
	}
	if(value > maxValue)
	{
		return maxValue;
	}
	return value;
}
}
#endif

int UIScene_SettingsOptionsMenu::m_iDifficultySettingA[4]=
{
	IDS_DIFFICULTY_PEACEFUL,
	IDS_DIFFICULTY_EASY,
	IDS_DIFFICULTY_NORMAL,
	IDS_DIFFICULTY_HARD
};

int UIScene_SettingsOptionsMenu::m_iDifficultyTitleSettingA[4]=
{
	IDS_DIFFICULTY_TITLE_PEACEFUL,
	IDS_DIFFICULTY_TITLE_EASY,
	IDS_DIFFICULTY_TITLE_NORMAL,
	IDS_DIFFICULTY_TITLE_HARD
};

UIScene_SettingsOptionsMenu::UIScene_SettingsOptionsMenu(int iPad, void *initData, UILayer *parentLayer) : UIScene(iPad, parentLayer)
{
	// Setup all the Iggy references we need for this scene
	initialiseMovie();
	
	m_bNotInGame=(Minecraft::GetInstance()->level==NULL);

	m_checkboxViewBob.init(app.GetString(IDS_VIEW_BOBBING),eControl_ViewBob,(app.GetGameSettings(m_iPad,eGameSetting_ViewBob)!=0));
	m_checkboxShowHints.init(app.GetString(IDS_HINTS),eControl_ShowHints,(app.GetGameSettings(m_iPad,eGameSetting_Hints)!=0));
	m_checkboxShowTooltips.init(app.GetString(IDS_IN_GAME_TOOLTIPS),eControl_ShowTooltips,(app.GetGameSettings(m_iPad,eGameSetting_Tooltips)!=0));
	m_checkboxInGameGamertags.init(app.GetString(IDS_IN_GAME_GAMERTAGS),eControl_InGameGamertags,(app.GetGameSettings(m_iPad,eGameSetting_GamertagsVisible)!=0));

	// check if we should display the mash-up option
	if(m_bNotInGame && app.GetMashupPackWorlds(m_iPad)!=0xFFFFFFFF)
	{
		// the mash-up option is needed
		m_bMashUpWorldsUnhideOption=true;
		m_checkboxMashupWorlds.init(app.GetString(IDS_UNHIDE_MASHUP_WORLDS),eControl_ShowMashUpWorlds,false);
	}
	else
	{
		//m_checkboxMashupWorlds.init(L"",eControl_ShowMashUpWorlds,false);
		removeControl(&m_checkboxMashupWorlds, true);
		m_bMashUpWorldsUnhideOption=false;
	}

	unsigned char ucValue=app.GetGameSettings(m_iPad,eGameSetting_Autosave);

	wchar_t autosaveLabels[9][256];
	for(unsigned int i = 0; i < 9; ++i)
	{
		if(i==0)
		{
			swprintf( autosaveLabels[i], 256, L"%ls", app.GetString( IDS_SLIDER_AUTOSAVE_OFF ));		
		}
		else
		{
			swprintf( autosaveLabels[i], 256, L"%ls: %d %ls", app.GetString( IDS_SLIDER_AUTOSAVE ),i*15, app.GetString( IDS_MINUTES ));		
		}

	}
	m_sliderAutosave.setAllPossibleLabels(9,autosaveLabels);
	m_sliderAutosave.init(autosaveLabels[ucValue],eControl_Autosave,0,8,ucValue);
#if defined(__APPLE__)
	m_appleAutosaveValue = ucValue;
#endif

#if defined(_XBOX_ONE) || defined(__ORBIS__)
	removeControl(&m_sliderAutosave,true);
#endif

	ucValue = app.GetGameSettings(m_iPad,eGameSetting_Difficulty);
	wchar_t difficultyLabels[4][256];
	for(unsigned int i = 0; i < 4; ++i)
	{
		swprintf( difficultyLabels[i], 256, L"%ls: %ls", app.GetString( IDS_SLIDER_DIFFICULTY ),app.GetString(m_iDifficultyTitleSettingA[i]));	
	}
	m_sliderDifficulty.setAllPossibleLabels(4,difficultyLabels);
	m_sliderDifficulty.init(difficultyLabels[ucValue],eControl_Difficulty,0,3,ucValue);
#if defined(__APPLE__)
	m_appleDifficultyValue = ucValue;
#endif

 	wstring wsText=app.GetString(m_iDifficultySettingA[app.GetGameSettings(m_iPad,eGameSetting_Difficulty)]);
	EHTMLFontSize size = eHTMLSize_Normal;
	if(!RenderManager.IsHiDef() && !RenderManager.IsWidescreen())
	{
		size = eHTMLSize_Splitscreen;
	}
	wchar_t startTags[64];
	swprintf(startTags,64,L"<font color=\"#%08x\">",app.GetHTMLColour(eHTMLColor_White));
 	wsText= startTags + wsText;

	m_labelDifficultyText.init(wsText);

	// If you are in-game, only the game host can change in-game gamertags, and you can't change difficulty
	// only the primary player gets to change the autosave and difficulty settings
	bool bRemoveDifficulty=false;
	bool bRemoveAutosave=false;
	bool bRemoveInGameGamertags=false;
	float fRemoveHeight=0.0f,fWidth,fHeight;
	
	bool bNotInGame=(Minecraft::GetInstance()->level==NULL);
	bool bPrimaryPlayer = ProfileManager.GetPrimaryPad()==m_iPad;
	if(!bPrimaryPlayer)
	{
		bRemoveDifficulty=true;
		bRemoveAutosave=true;
		bRemoveInGameGamertags=true;
	}

	if(!bNotInGame) // in the game
	{ 
		bRemoveDifficulty=true;
		if(!g_NetworkManager.IsHost())
		{
			bRemoveAutosave=true;
			bRemoveInGameGamertags=true;	
		}
	}
	if(bRemoveDifficulty)
	{
		m_labelDifficultyText.setVisible( false );
		removeControl(&m_sliderDifficulty, true);
	}

	if(bRemoveAutosave)
	{
		removeControl(&m_sliderAutosave, true);
	}

	if(bRemoveInGameGamertags)
	{
		removeControl(&m_checkboxInGameGamertags, true);
	}

	doHorizontalResizeCheck();

	if(app.GetLocalPlayerCount()>1)
	{
#if TO_BE_IMPLEMENTED
		app.AdjustSplitscreenScene(m_hObj,&m_OriginalPosition,m_iPad);
#endif
	}

#if defined(__APPLE__)
	m_appleSelectedIndex = 0;
	appleSetSelectedIndex(0);
#endif
}

UIScene_SettingsOptionsMenu::~UIScene_SettingsOptionsMenu()
{
}

wstring UIScene_SettingsOptionsMenu::getMoviePath()
{
	if(app.GetLocalPlayerCount() > 1)
	{
		return L"SettingsOptionsMenuSplit";
	}
	else
	{
		return L"SettingsOptionsMenu";
	}
}

void UIScene_SettingsOptionsMenu::updateTooltips()
{
	ui.SetTooltips( m_iPad, IDS_TOOLTIPS_SELECT,IDS_TOOLTIPS_BACK);
}

void UIScene_SettingsOptionsMenu::updateComponents()
{
	bool bNotInGame=(Minecraft::GetInstance()->level==NULL);
	if(bNotInGame)
	{
		m_parentLayer->showComponent(m_iPad,eUIComponent_Panorama,true);
		m_parentLayer->showComponent(m_iPad,eUIComponent_Logo,true);
	}
	else
	{
		m_parentLayer->showComponent(m_iPad,eUIComponent_Panorama,false);

		if( app.GetLocalPlayerCount() == 1 ) m_parentLayer->showComponent(m_iPad,eUIComponent_Logo,RenderManager.IsHiDef());
		else m_parentLayer->showComponent(m_iPad,eUIComponent_Logo,false);

	}
}

void UIScene_SettingsOptionsMenu::handleInput(int iPad, int key, bool repeat, bool pressed, bool released, bool &handled)
{
	ui.AnimateKeyPress(iPad, key, repeat, pressed, released);
	switch(key)
	{
	case ACTION_MENU_CANCEL:
		if(pressed)
		{
			// check the checkboxes
			app.SetGameSettings(m_iPad,eGameSetting_ViewBob,m_checkboxViewBob.IsChecked()?1:0);
			app.SetGameSettings(m_iPad,eGameSetting_GamertagsVisible,m_checkboxInGameGamertags.IsChecked()?1:0);
			app.SetGameSettings(m_iPad,eGameSetting_Hints,m_checkboxShowHints.IsChecked()?1:0);
			app.SetGameSettings(m_iPad,eGameSetting_Tooltips,m_checkboxShowTooltips.IsChecked()?1:0);

			// the mashup option will only be shown if some worlds have been previously hidden
			if(m_bMashUpWorldsUnhideOption && m_checkboxMashupWorlds.IsChecked())
			{
				// unhide all worlds
				app.EnableMashupPackWorlds(m_iPad);
			}

			// 4J-PB - don't action changes here or we might write to the profile on backing out here and then get a change in the settings all, and write again on backing out there
			//app.CheckGameSettingsChanged(true,pInputData->UserIndex);

			navigateBack();
		}
		break;
	case ACTION_MENU_OK:
#ifdef __ORBIS__
	case ACTION_MENU_TOUCHPAD_PRESS:
#endif
#if defined(__APPLE__)
		if(pressed)
		{
			extern int AppleMouse_GetHoveredControlId();
			extern bool AppleMouse_IsOverButton();
			int controlId = AppleMouse_IsOverButton() ? AppleMouse_GetHoveredControlId() : appleGetControlIdForVisibleRow(appleGetSelectedIndex());
			if(controlId >= 0)
			{
				appleActivateControl(controlId);
			}
		}
		break;
#else
		sendInputToMovie(key, repeat, pressed, released);
		break;
#endif
	case ACTION_MENU_UP:
	case ACTION_MENU_DOWN:
#if defined(__APPLE__)
		if(pressed && !repeat)
		{
			int delta = key == ACTION_MENU_UP ? -1 : 1;
			appleSetSelectedIndex(appleGetSelectedIndex() + delta);
			ui.PlayUISFX(eSFX_Press);
		}
		break;
#else
	case ACTION_MENU_LEFT:
	case ACTION_MENU_RIGHT:
		sendInputToMovie(key, repeat, pressed, released);
		break;
#endif
#if defined(__APPLE__)
	case ACTION_MENU_LEFT:
	case ACTION_MENU_RIGHT:
		if(pressed)
		{
			int controlId = appleGetControlIdForVisibleRow(appleGetSelectedIndex());
			int delta = key == ACTION_MENU_LEFT ? -1 : 1;
			if(appleAdjustControl(controlId, delta))
			{
				ui.PlayUISFX(eSFX_Press);
			}
		}
		break;
#endif
	}
}

void UIScene_SettingsOptionsMenu::handleSliderMove(F64 sliderId, F64 currentValue)
{
	WCHAR TempString[256];
	int value = (int)currentValue;
	switch((int)sliderId)
	{
	case eControl_Autosave:
		m_sliderAutosave.handleSliderMove(value);
#if defined(__APPLE__)
		m_appleAutosaveValue = value;
#endif

		app.SetGameSettings(m_iPad,eGameSetting_Autosave,value);
		// Update the autosave timer
		app.SetAutosaveTimerTime();

		break;
	case eControl_Difficulty:
		m_sliderDifficulty.handleSliderMove(value);
#if defined(__APPLE__)
		m_appleDifficultyValue = value;
#endif

		app.SetGameSettings(m_iPad,eGameSetting_Difficulty,value);
		
		wstring wsText=app.GetString(m_iDifficultySettingA[value]);
		EHTMLFontSize size = eHTMLSize_Normal;
		if(!RenderManager.IsHiDef() && !RenderManager.IsWidescreen())
		{
			size = eHTMLSize_Splitscreen;
		}
		wchar_t startTags[64];
		swprintf(startTags,64,L"<font color=\"#%08x\">",app.GetHTMLColour(eHTMLColor_White));
		wsText= startTags + wsText;
		m_labelDifficultyText.setLabel(wsText.c_str());
		break;
	}
}

#if defined(__APPLE__)
int UIScene_SettingsOptionsMenu::appleGetSelectedIndex()
{
	int count = appleGetVisibleRowCount();
	if(count <= 0)
	{
		return 0;
	}
	if(m_appleSelectedIndex < 0)
	{
		return 0;
	}
	if(m_appleSelectedIndex >= count)
	{
		return count - 1;
	}
	return m_appleSelectedIndex;
}

void UIScene_SettingsOptionsMenu::appleSetSelectedIndex(int index)
{
	int count = appleGetVisibleRowCount();
	if(count <= 0)
	{
		m_appleSelectedIndex = 0;
		return;
	}
	if(index < 0)
	{
		index = 0;
	}
	if(index >= count)
	{
		index = count - 1;
	}
	m_appleSelectedIndex = index;
}

int UIScene_SettingsOptionsMenu::appleGetVisibleRowCount()
{
	int count = 3;
	if(ProfileManager.GetPrimaryPad() == m_iPad && (m_bNotInGame || g_NetworkManager.IsHost()))
	{
		++count;
	}
	if(m_bMashUpWorldsUnhideOption)
	{
		++count;
	}
	if(ProfileManager.GetPrimaryPad() == m_iPad && (m_bNotInGame || g_NetworkManager.IsHost()))
	{
		++count;
	}
	if(ProfileManager.GetPrimaryPad() == m_iPad && m_bNotInGame)
	{
		++count;
	}
	return count;
}

int UIScene_SettingsOptionsMenu::appleGetControlIdForVisibleRow(int index)
{
	int visibleIndex = 0;
	int controls[] =
	{
		eControl_ViewBob,
		eControl_ShowHints,
		eControl_ShowTooltips,
		eControl_InGameGamertags,
		eControl_ShowMashUpWorlds,
		eControl_Autosave,
		eControl_Difficulty
	};

	for(unsigned int i = 0; i < sizeof(controls) / sizeof(controls[0]); ++i)
	{
		int controlId = controls[i];
		bool visible = false;
		switch(controlId)
		{
		case eControl_ViewBob:
		case eControl_ShowHints:
		case eControl_ShowTooltips:
			visible = true;
			break;
		case eControl_InGameGamertags:
			visible = ProfileManager.GetPrimaryPad() == m_iPad && (m_bNotInGame || g_NetworkManager.IsHost());
			break;
		case eControl_ShowMashUpWorlds:
			visible = m_bMashUpWorldsUnhideOption;
			break;
		case eControl_Autosave:
			visible = ProfileManager.GetPrimaryPad() == m_iPad && (m_bNotInGame || g_NetworkManager.IsHost());
			break;
		case eControl_Difficulty:
			visible = ProfileManager.GetPrimaryPad() == m_iPad && m_bNotInGame;
			break;
		}

		if(!visible)
		{
			continue;
		}

		if(visibleIndex == index)
		{
			return controlId;
		}
		++visibleIndex;
	}

	return -1;
}

const wchar_t *UIScene_SettingsOptionsMenu::appleGetLabelForVisibleRow(int index)
{
	int controlId = appleGetControlIdForVisibleRow(index);
	switch(controlId)
	{
	case eControl_ViewBob:
		return m_checkboxViewBob.getLabel();
	case eControl_ShowHints:
		return m_checkboxShowHints.getLabel();
	case eControl_ShowTooltips:
		return m_checkboxShowTooltips.getLabel();
	case eControl_InGameGamertags:
		return m_checkboxInGameGamertags.getLabel();
	case eControl_ShowMashUpWorlds:
		return m_checkboxMashupWorlds.getLabel();
	case eControl_Autosave:
		return m_sliderAutosave.getLabel();
	case eControl_Difficulty:
		return m_sliderDifficulty.getLabel();
	default:
		return L"";
	}
}

bool UIScene_SettingsOptionsMenu::appleIsCheckboxControl(int controlId)
{
	return controlId >= eControl_ViewBob && controlId <= eControl_ShowMashUpWorlds;
}

bool UIScene_SettingsOptionsMenu::appleIsSliderControl(int controlId)
{
	return controlId == eControl_Autosave || controlId == eControl_Difficulty;
}

bool UIScene_SettingsOptionsMenu::appleGetCheckboxValue(int controlId)
{
	switch(controlId)
	{
	case eControl_ViewBob:
		return m_checkboxViewBob.IsChecked();
	case eControl_ShowHints:
		return m_checkboxShowHints.IsChecked();
	case eControl_ShowTooltips:
		return m_checkboxShowTooltips.IsChecked();
	case eControl_InGameGamertags:
		return m_checkboxInGameGamertags.IsChecked();
	case eControl_ShowMashUpWorlds:
		return m_checkboxMashupWorlds.IsChecked();
	default:
		return false;
	}
}

int UIScene_SettingsOptionsMenu::appleGetSliderValue(int controlId)
{
	switch(controlId)
	{
	case eControl_Autosave:
		return m_appleAutosaveValue;
	case eControl_Difficulty:
		return m_appleDifficultyValue;
	default:
		return 0;
	}
}

int UIScene_SettingsOptionsMenu::appleGetSliderMin(int controlId)
{
	switch(controlId)
	{
	case eControl_Autosave:
	case eControl_Difficulty:
		return 0;
	default:
		return 0;
	}
}

int UIScene_SettingsOptionsMenu::appleGetSliderMax(int controlId)
{
	switch(controlId)
	{
	case eControl_Autosave:
		return 8;
	case eControl_Difficulty:
		return 3;
	default:
		return 0;
	}
}

void UIScene_SettingsOptionsMenu::appleActivateControl(int controlId)
{
	if(appleIsCheckboxControl(controlId))
	{
		bool nextValue = !appleGetCheckboxValue(controlId);
		switch(controlId)
		{
		case eControl_ViewBob:
			m_checkboxViewBob.setChecked(nextValue);
			break;
		case eControl_ShowHints:
			m_checkboxShowHints.setChecked(nextValue);
			break;
		case eControl_ShowTooltips:
			m_checkboxShowTooltips.setChecked(nextValue);
			break;
		case eControl_InGameGamertags:
			m_checkboxInGameGamertags.setChecked(nextValue);
			break;
		case eControl_ShowMashUpWorlds:
			m_checkboxMashupWorlds.setChecked(nextValue);
			break;
		}
		ui.PlayUISFX(eSFX_Press);
		return;
	}

	if(appleIsSliderControl(controlId) && appleAdjustControl(controlId, 1))
	{
		ui.PlayUISFX(eSFX_Press);
	}
}

bool UIScene_SettingsOptionsMenu::appleAdjustControl(int controlId, int delta)
{
	if(!appleIsSliderControl(controlId) || delta == 0)
	{
		return false;
	}

	int value = appleGetSliderValue(controlId);
	int newValue = clampAppleOptionValue(value + delta, appleGetSliderMin(controlId), appleGetSliderMax(controlId));
	if(newValue == value)
	{
		return false;
	}

	handleSliderMove((F64)controlId, (F64)newValue);
	return true;
}
#endif
