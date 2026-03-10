#include "stdafx.h"
#include "UI.h"
#include "UIScene_SettingsAudioMenu.h"

#if defined(__APPLE__)
namespace
{
static int clampAudioValue(int value, int minValue, int maxValue)
{
	if(value < minValue) return minValue;
	if(value > maxValue) return maxValue;
	return value;
}
}
#endif

UIScene_SettingsAudioMenu::UIScene_SettingsAudioMenu(int iPad, void *initData, UILayer *parentLayer) : UIScene(iPad, parentLayer)
{
	// Setup all the Iggy references we need for this scene
	initialiseMovie();

	WCHAR TempString[256];
	swprintf( (WCHAR *)TempString, 256, L"%ls: %d%%", app.GetString( IDS_SLIDER_MUSIC ),app.GetGameSettings(m_iPad,eGameSetting_MusicVolume));
	m_sliderMusic.init(TempString,eControl_Music,0,100,app.GetGameSettings(m_iPad,eGameSetting_MusicVolume));

	swprintf( (WCHAR *)TempString, 256, L"%ls: %d%%", app.GetString( IDS_SLIDER_SOUND ),app.GetGameSettings(m_iPad,eGameSetting_SoundFXVolume));
	m_sliderSound.init(TempString,eControl_Sound,0,100,app.GetGameSettings(m_iPad,eGameSetting_SoundFXVolume));

	doHorizontalResizeCheck();

	if(app.GetLocalPlayerCount()>1)
	{
#if TO_BE_IMPLEMENTED
		app.AdjustSplitscreenScene(m_hObj,&m_OriginalPosition,m_iPad);
#endif
	}

#if defined(__APPLE__)
	m_appleSelectedIndex = 0;
	m_appleMusicVolume = app.GetGameSettings(m_iPad, eGameSetting_MusicVolume);
	m_appleSoundVolume = app.GetGameSettings(m_iPad, eGameSetting_SoundFXVolume);
	appleSetSelectedIndex(0);
#endif
}

UIScene_SettingsAudioMenu::~UIScene_SettingsAudioMenu()
{
}

wstring UIScene_SettingsAudioMenu::getMoviePath()
{
	if(app.GetLocalPlayerCount() > 1)
	{
		return L"SettingsAudioMenuSplit";
	}
	else
	{
		return L"SettingsAudioMenu";
	}
}

void UIScene_SettingsAudioMenu::updateTooltips()
{
	ui.SetTooltips( m_iPad, IDS_TOOLTIPS_SELECT,IDS_TOOLTIPS_BACK);
}

void UIScene_SettingsAudioMenu::updateComponents()
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

		if( app.GetLocalPlayerCount() == 1 ) m_parentLayer->showComponent(m_iPad,eUIComponent_Logo,true);
		else m_parentLayer->showComponent(m_iPad,eUIComponent_Logo,false);
	}
}

void UIScene_SettingsAudioMenu::handleInput(int iPad, int key, bool repeat, bool pressed, bool released, bool &handled)
{
	//app.DebugPrintf("UIScene_DebugOverlay handling input for pad %d, key %d, down- %s, pressed- %s, released- %s\n", iPad, key, down?"TRUE":"FALSE", pressed?"TRUE":"FALSE", released?"TRUE":"FALSE");
	ui.AnimateKeyPress(m_iPad, key, repeat, pressed, released);

	switch(key)
	{
	case ACTION_MENU_CANCEL:
		if(pressed)
		{
#if defined(__APPLE__)
			// Apply volume changes
			app.SetGameSettings(m_iPad, eGameSetting_MusicVolume, m_appleMusicVolume);
			app.SetGameSettings(m_iPad, eGameSetting_SoundFXVolume, m_appleSoundVolume);
#endif
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
			int controlId = appleGetControlIdForVisibleRow(appleGetSelectedIndex());
			if(controlId >= 0)
			{
				appleAdjustControl(controlId, 10);
				ui.PlayUISFX(eSFX_Press);
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
				ui.PlayUISFX(eSFX_Scroll);
			}
		}
		break;
#endif
	}
}

void UIScene_SettingsAudioMenu::handleSliderMove(F64 sliderId, F64 currentValue)
{
	WCHAR TempString[256];
	int value = (int)currentValue;
	switch((int)sliderId)
	{
	case eControl_Music:
		m_sliderMusic.handleSliderMove(value);

		app.SetGameSettings(m_iPad,eGameSetting_MusicVolume,value);
		swprintf( (WCHAR *)TempString, 256, L"%ls: %d%%", app.GetString( IDS_SLIDER_MUSIC ),value);
		m_sliderMusic.setLabel(TempString);

#if defined(__APPLE__)
		m_appleMusicVolume = value;
#endif
		break;
	case eControl_Sound:
		m_sliderSound.handleSliderMove(value);

		app.SetGameSettings(m_iPad,eGameSetting_SoundFXVolume,value);
		swprintf( (WCHAR *)TempString, 256, L"%ls: %d%%", app.GetString( IDS_SLIDER_SOUND ),value);
		m_sliderSound.setLabel(TempString);

#if defined(__APPLE__)
		m_appleSoundVolume = value;
#endif
		break;
	}
}

#if defined(__APPLE__)
int UIScene_SettingsAudioMenu::appleGetSelectedIndex()
{
	int count = appleGetVisibleRowCount();
	if(count <= 0) return 0;
	if(m_appleSelectedIndex < 0) return 0;
	if(m_appleSelectedIndex >= count) return count - 1;
	return m_appleSelectedIndex;
}

void UIScene_SettingsAudioMenu::appleSetSelectedIndex(int index)
{
	int count = appleGetVisibleRowCount();
	if(count <= 0) { m_appleSelectedIndex = 0; return; }
	if(index < 0) index = 0;
	if(index >= count) index = count - 1;
	m_appleSelectedIndex = index;
}

int UIScene_SettingsAudioMenu::appleGetVisibleRowCount()
{
	return 2; // Music, Sound
}

int UIScene_SettingsAudioMenu::appleGetControlIdForVisibleRow(int index)
{
	switch(index)
	{
	case 0: return eControl_Music;
	case 1: return eControl_Sound;
	default: return -1;
	}
}

wstring UIScene_SettingsAudioMenu::appleGetLabelForVisibleRow(int index)
{
	int controlId = appleGetControlIdForVisibleRow(index);
	WCHAR buf[128];
	switch(controlId)
	{
	case eControl_Music:
		swprintf(buf, 128, L"Music: %d%%", m_appleMusicVolume);
		return wstring(buf);
	case eControl_Sound:
		swprintf(buf, 128, L"Sound: %d%%", m_appleSoundVolume);
		return wstring(buf);
	default:
		return L"";
	}
}

bool UIScene_SettingsAudioMenu::appleIsSliderControl(int controlId)
{
	return controlId == eControl_Music || controlId == eControl_Sound;
}

int UIScene_SettingsAudioMenu::appleGetSliderValue(int controlId)
{
	switch(controlId)
	{
	case eControl_Music: return m_appleMusicVolume;
	case eControl_Sound: return m_appleSoundVolume;
	default: return 0;
	}
}

bool UIScene_SettingsAudioMenu::appleAdjustControl(int controlId, int delta)
{
	int value = appleGetSliderValue(controlId);
	int newValue = clampAudioValue(value + delta, 0, 100);
	if(newValue == value) return false;

	switch(controlId)
	{
	case eControl_Music:
		m_appleMusicVolume = newValue;
		handleSliderMove((F64)eControl_Music, (F64)newValue);
		break;
	case eControl_Sound:
		m_appleSoundVolume = newValue;
		handleSliderMove((F64)eControl_Sound, (F64)newValue);
		break;
	}
	return true;
}
#endif
