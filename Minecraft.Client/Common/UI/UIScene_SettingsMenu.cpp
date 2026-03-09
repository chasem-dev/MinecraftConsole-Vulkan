#include "stdafx.h"
#include "UI.h"
#include "UIScene_SettingsMenu.h"
#include "../../Minecraft.h"

#if defined(__APPLE__)
namespace
{
bool AppleShouldShowSettingsMenuButton(int controlId, int iPad)
{
	switch(controlId)
	{
	case BUTTON_ALL_OPTIONS:
	case BUTTON_ALL_CONTROL:
	case BUTTON_ALL_UI:
	case BUTTON_ALL_RESETTODEFAULTS:
		return true;
	case BUTTON_ALL_AUDIO:
	case BUTTON_ALL_GRAPHICS:
		return ProfileManager.GetPrimaryPad() == iPad;
	default:
		return false;
	}
}
}
#endif

UIScene_SettingsMenu::UIScene_SettingsMenu(int iPad, void *initData, UILayer *parentLayer) : UIScene(iPad, parentLayer)
{
	// Setup all the Iggy references we need for this scene
	initialiseMovie();
	
	bool bNotInGame=(Minecraft::GetInstance()->level==NULL);

	m_buttons[BUTTON_ALL_OPTIONS].init(app.GetString(IDS_OPTIONS),BUTTON_ALL_OPTIONS);
	m_buttons[BUTTON_ALL_AUDIO].init(app.GetString(IDS_AUDIO),BUTTON_ALL_AUDIO);
	m_buttons[BUTTON_ALL_CONTROL].init(app.GetString(IDS_CONTROL),BUTTON_ALL_CONTROL);
	m_buttons[BUTTON_ALL_GRAPHICS].init(app.GetString(IDS_GRAPHICS),BUTTON_ALL_GRAPHICS);
	m_buttons[BUTTON_ALL_UI].init(app.GetString(IDS_USER_INTERFACE),BUTTON_ALL_UI);
	m_buttons[BUTTON_ALL_RESETTODEFAULTS].init(app.GetString(IDS_RESET_TO_DEFAULTS),BUTTON_ALL_RESETTODEFAULTS);

	if(ProfileManager.GetPrimaryPad()!=m_iPad)
	{
		removeControl( &m_buttons[BUTTON_ALL_AUDIO], true);
		removeControl( &m_buttons[BUTTON_ALL_GRAPHICS], true);
	}

	doHorizontalResizeCheck();

	if(app.GetLocalPlayerCount()>1)
	{
#if TO_BE_IMPLEMENTED
		app.AdjustSplitscreenScene(m_hObj,&m_OriginalPosition,m_iPad,false);
#endif
	}

#if defined(__APPLE__)
	m_appleSelectedIndex = 0;
	appleSetSelectedIndex(0);
#endif
}

UIScene_SettingsMenu::~UIScene_SettingsMenu()
{
}

wstring UIScene_SettingsMenu::getMoviePath()
{
	if(app.GetLocalPlayerCount() > 1)
	{
		return L"SettingsMenuSplit";
	}
	else
	{
		return L"SettingsMenu";
	}
}

void UIScene_SettingsMenu::handleReload()
{
	if(ProfileManager.GetPrimaryPad()!=m_iPad)
	{
		removeControl( &m_buttons[BUTTON_ALL_AUDIO], true);
		removeControl( &m_buttons[BUTTON_ALL_GRAPHICS], true);
	}

	doHorizontalResizeCheck();

#if defined(__APPLE__)
	appleSetSelectedIndex(m_appleSelectedIndex);
#endif
}

void UIScene_SettingsMenu::updateTooltips()
{
	ui.SetTooltips( m_iPad, IDS_TOOLTIPS_SELECT,IDS_TOOLTIPS_BACK);
}

void UIScene_SettingsMenu::updateComponents()
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

void UIScene_SettingsMenu::handleInput(int iPad, int key, bool repeat, bool pressed, bool released, bool &handled)
{
	//app.DebugPrintf("UIScene_DebugOverlay handling input for pad %d, key %d, down- %s, pressed- %s, released- %s\n", iPad, key, down?"TRUE":"FALSE", pressed?"TRUE":"FALSE", released?"TRUE":"FALSE");
	ui.AnimateKeyPress(m_iPad, key, repeat, pressed, released);

	switch(key)
	{
	case ACTION_MENU_CANCEL:
		if(pressed)
		{
			// if the profile data has been changed, then force a profile write
			// It seems we're allowed to break the 5 minute rule if it's the result of a user action

			app.CheckGameSettingsChanged(true,iPad);          
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
			int controlId = AppleMouse_IsOverButton() ? AppleMouse_GetHoveredControlId() : appleGetControlIdForVisibleButton(appleGetSelectedIndex());
			if(controlId >= 0)
			{
				handlePress((F64)controlId, 0);
			}
		}
		break;
#else
		sendInputToMovie(key, repeat, pressed, released);
		break;
	case ACTION_MENU_UP:
	case ACTION_MENU_DOWN:
		sendInputToMovie(key, repeat, pressed, released);
		break;
#endif
#if defined(__APPLE__)
	case ACTION_MENU_UP:
		if(pressed && !repeat)
		{
			appleSetSelectedIndex(appleGetSelectedIndex() - 1);
			ui.PlayUISFX(eSFX_Press);
		}
		break;
	case ACTION_MENU_DOWN:
		if(pressed && !repeat)
		{
			appleSetSelectedIndex(appleGetSelectedIndex() + 1);
			ui.PlayUISFX(eSFX_Press);
		}
		break;
#endif
	}
}

void UIScene_SettingsMenu::handlePress(F64 controlId, F64 childId)
{
	//CD - Added for audio
	ui.PlayUISFX(eSFX_Press);

	switch((int)controlId)
	{
	case BUTTON_ALL_OPTIONS:
		ui.NavigateToScene(m_iPad, eUIScene_SettingsOptionsMenu);
		break;
	case BUTTON_ALL_AUDIO:
		ui.NavigateToScene(m_iPad, eUIScene_SettingsAudioMenu);
		break;
	case BUTTON_ALL_CONTROL:
		ui.NavigateToScene(m_iPad, eUIScene_SettingsControlMenu);
		break;
	case BUTTON_ALL_GRAPHICS:
		ui.NavigateToScene(m_iPad, eUIScene_SettingsGraphicsMenu);
		break;
	case BUTTON_ALL_UI:
		ui.NavigateToScene(m_iPad, eUIScene_SettingsUIMenu);
		break;
	case BUTTON_ALL_RESETTODEFAULTS:
		{
			// check they really want to do this
			UINT uiIDA[2];
			uiIDA[0]=IDS_CONFIRM_CANCEL;
			uiIDA[1]=IDS_CONFIRM_OK;

			ui.RequestMessageBox(IDS_DEFAULTS_TITLE, IDS_DEFAULTS_TEXT, uiIDA, 2, m_iPad,&UIScene_SettingsMenu::ResetDefaultsDialogReturned,this, app.GetStringTable(), NULL, 0, false);
		}	
		break;
	}
}

#if defined(__APPLE__)
int UIScene_SettingsMenu::appleGetSelectedIndex()
{
	int count = appleGetVisibleButtonCount();
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

void UIScene_SettingsMenu::appleSetSelectedIndex(int index)
{
	int count = appleGetVisibleButtonCount();
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

int UIScene_SettingsMenu::appleGetVisibleButtonCount()
{
	int count = 0;
	for(int controlId = 0; controlId < BUTTONS_ALL_MAX; ++controlId)
	{
		if(AppleShouldShowSettingsMenuButton(controlId, m_iPad))
		{
			++count;
		}
	}
	return count;
}

int UIScene_SettingsMenu::appleGetControlIdForVisibleButton(int index)
{
	int visibleIndex = 0;
	for(int controlId = 0; controlId < BUTTONS_ALL_MAX; ++controlId)
	{
		if(!AppleShouldShowSettingsMenuButton(controlId, m_iPad))
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

const wchar_t *UIScene_SettingsMenu::appleGetLabelForVisibleButton(int index)
{
	int controlId = appleGetControlIdForVisibleButton(index);
	if(controlId < 0 || controlId >= BUTTONS_ALL_MAX)
	{
		return L"";
	}
	return m_buttons[controlId].getLabel();
}
#endif

int UIScene_SettingsMenu::ResetDefaultsDialogReturned(void *pParam,int iPad,C4JStorage::EMessageResult result)
{
	UIScene_SettingsMenu* pClass = (UIScene_SettingsMenu*)pParam;

	// results switched for this dialog
	if(result==C4JStorage::EMessage_ResultDecline) 
	{
#if (defined __PS3__ || defined __ORBIS__ || defined _DURANGO || defined __PSVITA__)
		app.SetDefaultOptions(StorageManager.GetDashboardProfileSettings(pClass->m_iPad),pClass->m_iPad);
#else
		app.SetDefaultOptions(ProfileManager.GetDashboardProfileSettings(pClass->m_iPad),pClass->m_iPad);
#endif
		// if the profile data has been changed, then force a profile write
		// It seems we're allowed to break the 5 minute rule if it's the result of a user action
		app.CheckGameSettingsChanged(true,iPad);
	}
	return 0;
}
