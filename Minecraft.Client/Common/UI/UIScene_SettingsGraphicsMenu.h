#pragma once

#include "UIScene.h"

class UIScene_SettingsGraphicsMenu : public UIScene
{
private:
	enum EControls
	{
		eControl_FancyGraphics,
		eControl_RenderDistance,
		eControl_SmoothLighting,
		eControl_Clouds,
		eControl_Particles,
		eControl_BedrockFog,
		eControl_CustomSkinAnim,
		eControl_FOV,
		eControl_Gamma,
		eControl_InterfaceOpacity
	};

	UIControl_CheckBox m_checkboxFancyGraphics, m_checkboxClouds, m_checkboxBedrockFog, m_checkboxCustomSkinAnim; // Checkboxes
	UIControl_Slider m_sliderGamma, m_sliderInterfaceOpacity; // Sliders
	UI_BEGIN_MAP_ELEMENTS_AND_NAMES(UIScene)
		UI_MAP_ELEMENT( m_checkboxFancyGraphics, "FancyGraphics")
		UI_MAP_ELEMENT( m_checkboxClouds, "Clouds")
		UI_MAP_ELEMENT( m_checkboxBedrockFog, "BedrockFog")
		UI_MAP_ELEMENT( m_checkboxCustomSkinAnim, "CustomSkinAnim")
		UI_MAP_ELEMENT( m_sliderGamma, "Gamma")
		UI_MAP_ELEMENT( m_sliderInterfaceOpacity, "InterfaceOpacity")
	UI_END_MAP_ELEMENTS_AND_NAMES()

	bool m_bNotInGame;
#if defined(__APPLE__)
	int m_appleSelectedIndex;
	int m_appleRenderDistance;
	bool m_appleSmoothLighting;
	int m_appleParticles;
	int m_appleFOV;
#endif
public:
	UIScene_SettingsGraphicsMenu(int iPad, void *initData, UILayer *parentLayer);
	virtual ~UIScene_SettingsGraphicsMenu();

	virtual EUIScene getSceneType() { return eUIScene_SettingsGraphicsMenu;}

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
	bool appleIsCheckboxControl(int controlId);
	bool appleIsCycleControl(int controlId);
	bool appleIsSliderControl(int controlId);
	bool appleGetCheckboxValue(int controlId);
	int appleGetSliderValue(int controlId);
	int appleGetSliderMin(int controlId);
	int appleGetSliderMax(int controlId);
	void appleActivateControl(int controlId);
	bool appleAdjustControl(int controlId, int delta);
#endif
};
