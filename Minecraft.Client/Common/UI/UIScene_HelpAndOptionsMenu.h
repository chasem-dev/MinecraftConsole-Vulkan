#pragma once

#include "UIScene.h"

#define BUTTON_HAO_CHANGESKIN			0
#define BUTTON_HAO_HOWTOPLAY			1
#define BUTTON_HAO_CONTROLS				2
#define BUTTON_HAO_SETTINGS				3
#define BUTTON_HAO_CREDITS				4
#define BUTTON_HAO_REINSTALL			5
#define BUTTON_HAO_DEBUG				6
#define BUTTONS_HAO_MAX			BUTTON_HAO_DEBUG + 1

class UIScene_HelpAndOptionsMenu : public UIScene
{
private:
	UIControl_Button m_buttons[BUTTONS_HAO_MAX];
	UI_BEGIN_MAP_ELEMENTS_AND_NAMES(UIScene)
		UI_MAP_ELEMENT( m_buttons[BUTTON_HAO_CHANGESKIN], "Button1")
		UI_MAP_ELEMENT( m_buttons[BUTTON_HAO_HOWTOPLAY], "Button2")
		UI_MAP_ELEMENT( m_buttons[BUTTON_HAO_CONTROLS], "Button3")
		UI_MAP_ELEMENT( m_buttons[BUTTON_HAO_SETTINGS], "Button4")
		UI_MAP_ELEMENT( m_buttons[BUTTON_HAO_CREDITS], "Button5")
		UI_MAP_ELEMENT( m_buttons[BUTTON_HAO_REINSTALL], "Button6")
		UI_MAP_ELEMENT( m_buttons[BUTTON_HAO_DEBUG], "Button7")
	UI_END_MAP_ELEMENTS_AND_NAMES()

	bool m_bNotInGame;
#if defined(__APPLE__)
	int m_appleSelectedIndex;
#endif
public:
	UIScene_HelpAndOptionsMenu(int iPad, void *initData, UILayer *parentLayer);
	virtual ~UIScene_HelpAndOptionsMenu();

	virtual EUIScene getSceneType() { return eUIScene_HelpAndOptionsMenu;}
	
	virtual void updateTooltips();
	virtual void updateComponents();

protected:
	// TODO: This should be pure virtual in this class
	virtual wstring getMoviePath();

public:
	virtual void handleReload();

	// INPUT
	virtual void handleInput(int iPad, int key, bool repeat, bool pressed, bool released, bool &handled);

#if defined(__APPLE__)
	int appleGetSelectedIndex();
	void appleSetSelectedIndex(int index);
	int appleGetVisibleButtonCount();
	int appleGetControlIdForVisibleButton(int index);
	const wchar_t *appleGetLabelForVisibleButton(int index);
#endif

protected:
	void handlePress(F64 controlId, F64 childId);
};
