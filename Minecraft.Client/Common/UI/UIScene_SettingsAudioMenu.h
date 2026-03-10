#pragma once

#include "UIScene.h"

class UIScene_SettingsAudioMenu : public UIScene
{
private:
	enum EControls
	{
		eControl_Music,
		eControl_Sound
	};

	UIControl_Slider m_sliderMusic, m_sliderSound; // Sliders
	UI_BEGIN_MAP_ELEMENTS_AND_NAMES(UIScene)
		UI_MAP_ELEMENT( m_sliderMusic, "Music")
		UI_MAP_ELEMENT( m_sliderSound, "Sound")
	UI_END_MAP_ELEMENTS_AND_NAMES()

#if defined(__APPLE__)
	int m_appleSelectedIndex;
	int m_appleMusicVolume;
	int m_appleSoundVolume;
#endif

public:
	UIScene_SettingsAudioMenu(int iPad, void *initData, UILayer *parentLayer);
	virtual ~UIScene_SettingsAudioMenu();

	virtual EUIScene getSceneType() { return eUIScene_SettingsAudioMenu;}

	virtual void updateTooltips();
	virtual void updateComponents();

protected:
	// TODO: This should be pure virtual in this class
	virtual wstring getMoviePath();

public:
	// INPUT
	virtual void handleInput(int iPad, int key, bool repeat, bool pressed, bool released, bool &handled);

	virtual void handleSliderMove(F64 sliderId, F64 currentValue);

#if defined(__APPLE__)
	int appleGetSelectedIndex();
	void appleSetSelectedIndex(int index);
	int appleGetVisibleRowCount();
	int appleGetControlIdForVisibleRow(int index);
	wstring appleGetLabelForVisibleRow(int index);
	bool appleIsSliderControl(int controlId);
	int appleGetSliderValue(int controlId);
	bool appleAdjustControl(int controlId, int delta);
#endif
};
