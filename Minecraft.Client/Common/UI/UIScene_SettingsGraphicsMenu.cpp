#include "stdafx.h"
#include "UI.h"
#include "UIScene_SettingsGraphicsMenu.h"

#if defined(__APPLE__)
#include "../../Options.h"
#include "../../LevelRenderer.h"

namespace
{
static int clampAppleGraphicsValue(int value, int minValue, int maxValue)
{
	if(value < minValue) return minValue;
	if(value > maxValue) return maxValue;
	return value;
}

static const wchar_t *kRenderDistanceNames[] = { L"Far", L"Normal", L"Short", L"Tiny" };
static const wchar_t *kParticleNames[] = { L"All", L"Decreased", L"Minimal" };
}
#endif

UIScene_SettingsGraphicsMenu::UIScene_SettingsGraphicsMenu(int iPad, void *initData, UILayer *parentLayer) : UIScene(iPad, parentLayer)
{
	// Setup all the Iggy references we need for this scene
	initialiseMovie();

	m_bNotInGame=(Minecraft::GetInstance()->level==NULL);

	m_checkboxFancyGraphics.init(L"Fancy Graphics",eControl_FancyGraphics,Minecraft::GetInstance()->options->fancyGraphics);
	m_checkboxClouds.init(app.GetString(IDS_CHECKBOX_RENDER_CLOUDS),eControl_Clouds,(app.GetGameSettings(m_iPad,eGameSetting_Clouds)!=0));
	m_checkboxBedrockFog.init(app.GetString(IDS_CHECKBOX_RENDER_BEDROCKFOG),eControl_BedrockFog,(app.GetGameSettings(m_iPad,eGameSetting_BedrockFog)!=0));
	m_checkboxCustomSkinAnim.init(app.GetString(IDS_CHECKBOX_CUSTOM_SKIN_ANIM),eControl_CustomSkinAnim,(app.GetGameSettings(m_iPad,eGameSetting_CustomSkinAnim)!=0));

	WCHAR TempString[256];

	swprintf( (WCHAR *)TempString, 256, L"%ls: %d%%", app.GetString( IDS_SLIDER_GAMMA ),app.GetGameSettings(m_iPad,eGameSetting_Gamma));
	m_sliderGamma.init(TempString,eControl_Gamma,0,100,app.GetGameSettings(m_iPad,eGameSetting_Gamma));

	swprintf( (WCHAR *)TempString, 256, L"%ls: %d%%", app.GetString( IDS_SLIDER_INTERFACEOPACITY ),app.GetGameSettings(m_iPad,eGameSetting_InterfaceOpacity));
	m_sliderInterfaceOpacity.init(TempString,eControl_InterfaceOpacity,0,100,app.GetGameSettings(m_iPad,eGameSetting_InterfaceOpacity));

	doHorizontalResizeCheck();

	bool bInGame=(Minecraft::GetInstance()->level!=NULL);
	bool bIsPrimaryPad=(ProfileManager.GetPrimaryPad()==m_iPad);
	if(bInGame)
	{
		if(bIsPrimaryPad)
		{
			if(!g_NetworkManager.IsHost())
			{
				removeControl(&m_checkboxBedrockFog, true);
			}
		}
		else
		{
			removeControl(&m_checkboxBedrockFog, true);
			removeControl(&m_checkboxCustomSkinAnim, true);
		}
	}

	if(app.GetLocalPlayerCount()>1)
	{
#if TO_BE_IMPLEMENTED
		app.AdjustSplitscreenScene(m_hObj,&m_OriginalPosition,m_iPad);
#endif
	}

#if defined(__APPLE__)
	Options *opts = Minecraft::GetInstance()->options;
	m_appleSelectedIndex = 0;
	m_appleRenderDistance = opts->viewDistance;
	m_appleSmoothLighting = opts->ambientOcclusion;
	m_appleParticles = opts->particles;
	m_appleFOV = (int)(opts->fov * 100.0f);
	appleSetSelectedIndex(0);
#endif
}

UIScene_SettingsGraphicsMenu::~UIScene_SettingsGraphicsMenu()
{
}

wstring UIScene_SettingsGraphicsMenu::getMoviePath()
{
	if(app.GetLocalPlayerCount() > 1)
	{
		return L"SettingsGraphicsMenuSplit";
	}
	else
	{
		return L"SettingsGraphicsMenu";
	}
}

void UIScene_SettingsGraphicsMenu::updateTooltips()
{
	ui.SetTooltips( m_iPad, IDS_TOOLTIPS_SELECT,IDS_TOOLTIPS_BACK);
}

void UIScene_SettingsGraphicsMenu::updateComponents()
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

void UIScene_SettingsGraphicsMenu::handleInput(int iPad, int key, bool repeat, bool pressed, bool released, bool &handled)
{
	ui.AnimateKeyPress(iPad, key, repeat, pressed, released);
	switch(key)
	{
	case ACTION_MENU_CANCEL:
		if(pressed)
		{
#if defined(__APPLE__)
			// Apply Options-based settings
			{
				Minecraft *mc = Minecraft::GetInstance();
				Options *opts = mc->options;

				bool needReload = false;

				if(opts->fancyGraphics != m_checkboxFancyGraphics.IsChecked())
				{
					opts->fancyGraphics = m_checkboxFancyGraphics.IsChecked();
					needReload = true;
				}
				if(opts->viewDistance != m_appleRenderDistance)
				{
					opts->viewDistance = m_appleRenderDistance;
					needReload = true;
				}
				if(opts->ambientOcclusion != m_appleSmoothLighting)
				{
					opts->ambientOcclusion = m_appleSmoothLighting;
					needReload = true;
				}
				if(opts->particles != m_appleParticles)
				{
					opts->particles = m_appleParticles;
				}
				opts->fov = (float)m_appleFOV / 100.0f;

				if(needReload)
				{
					mc->levelRenderer->allChanged();
				}
			}
#endif

			// check the checkboxes
			app.SetGameSettings(m_iPad,eGameSetting_Clouds,m_checkboxClouds.IsChecked()?1:0);
			app.SetGameSettings(m_iPad,eGameSetting_BedrockFog,m_checkboxBedrockFog.IsChecked()?1:0);
			app.SetGameSettings(m_iPad,eGameSetting_CustomSkinAnim,m_checkboxCustomSkinAnim.IsChecked()?1:0);

			navigateBack();
			handled = true;
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

void UIScene_SettingsGraphicsMenu::handleSliderMove(F64 sliderId, F64 currentValue)
{
	WCHAR TempString[256];
	int value = (int)currentValue;
	switch((int)sliderId)
	{
	case eControl_Gamma:
		m_sliderGamma.handleSliderMove(value);
		app.SetGameSettings(m_iPad,eGameSetting_Gamma,value);
		swprintf( (WCHAR *)TempString, 256, L"%ls: %d%%", app.GetString( IDS_SLIDER_GAMMA ),value);
		m_sliderGamma.setLabel(TempString);
		break;
	case eControl_InterfaceOpacity:
		m_sliderInterfaceOpacity.handleSliderMove(value);
		app.SetGameSettings(m_iPad,eGameSetting_InterfaceOpacity,value);
		swprintf( (WCHAR *)TempString, 256, L"%ls: %d%%", app.GetString( IDS_SLIDER_INTERFACEOPACITY ),value);
		m_sliderInterfaceOpacity.setLabel(TempString);
		break;
#if defined(__APPLE__)
	case eControl_FOV:
		m_appleFOV = value;
		break;
#endif
	}
}

#if defined(__APPLE__)
int UIScene_SettingsGraphicsMenu::appleGetSelectedIndex()
{
	int count = appleGetVisibleRowCount();
	if(count <= 0) return 0;
	if(m_appleSelectedIndex < 0) return 0;
	if(m_appleSelectedIndex >= count) return count - 1;
	return m_appleSelectedIndex;
}

void UIScene_SettingsGraphicsMenu::appleSetSelectedIndex(int index)
{
	int count = appleGetVisibleRowCount();
	if(count <= 0) { m_appleSelectedIndex = 0; return; }
	if(index < 0) index = 0;
	if(index >= count) index = count - 1;
	m_appleSelectedIndex = index;
}

int UIScene_SettingsGraphicsMenu::appleGetVisibleRowCount()
{
	// Always visible: FancyGraphics, RenderDistance, SmoothLighting, Clouds, Particles, FOV, Gamma, InterfaceOpacity
	int count = 8;

	bool bInGame = (Minecraft::GetInstance()->level != NULL);
	bool bIsPrimaryPad = (ProfileManager.GetPrimaryPad() == m_iPad);
	if(!bInGame || (bIsPrimaryPad && g_NetworkManager.IsHost()))
		++count; // BedrockFog
	if(!bInGame || bIsPrimaryPad)
		++count; // CustomSkinAnim

	return count;
}

int UIScene_SettingsGraphicsMenu::appleGetControlIdForVisibleRow(int index)
{
	int visibleIndex = 0;
	int controls[] =
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

	bool bInGame = (Minecraft::GetInstance()->level != NULL);
	bool bIsPrimaryPad = (ProfileManager.GetPrimaryPad() == m_iPad);

	for(unsigned int i = 0; i < sizeof(controls) / sizeof(controls[0]); ++i)
	{
		int controlId = controls[i];
		bool visible = true;
		if(controlId == eControl_BedrockFog)
			visible = !bInGame || (bIsPrimaryPad && g_NetworkManager.IsHost());
		else if(controlId == eControl_CustomSkinAnim)
			visible = !bInGame || bIsPrimaryPad;

		if(!visible) continue;
		if(visibleIndex == index) return controlId;
		++visibleIndex;
	}
	return -1;
}

wstring UIScene_SettingsGraphicsMenu::appleGetLabelForVisibleRow(int index)
{
	int controlId = appleGetControlIdForVisibleRow(index);
	switch(controlId)
	{
	case eControl_FancyGraphics:
	{
		wstring label = L"Graphics";
		label += m_checkboxFancyGraphics.IsChecked() ? L": Fancy" : L": Fast";
		return label;
	}
	case eControl_RenderDistance:
	{
		wstring label = L"Render Distance: ";
		label += kRenderDistanceNames[m_appleRenderDistance & 3];
		return label;
	}
	case eControl_SmoothLighting:
	{
		wstring label = L"Smooth Lighting";
		label += m_appleSmoothLighting ? L": On" : L": Off";
		return label;
	}
	case eControl_Clouds:
	{
		wstring label = m_checkboxClouds.getLabel();
		label += m_checkboxClouds.IsChecked() ? L": On" : L": Off";
		return label;
	}
	case eControl_Particles:
	{
		wstring label = L"Particles: ";
		label += kParticleNames[m_appleParticles % 3];
		return label;
	}
	case eControl_BedrockFog:
	{
		wstring label = m_checkboxBedrockFog.getLabel();
		label += m_checkboxBedrockFog.IsChecked() ? L": On" : L": Off";
		return label;
	}
	case eControl_CustomSkinAnim:
	{
		wstring label = m_checkboxCustomSkinAnim.getLabel();
		label += m_checkboxCustomSkinAnim.IsChecked() ? L": On" : L": Off";
		return label;
	}
	case eControl_FOV:
	{
		if(m_appleFOV == 0)
			return L"FOV: Normal";
		else if(m_appleFOV == 100)
			return L"FOV: Quake Pro";
		else
		{
			WCHAR buf[64];
			swprintf(buf, 64, L"FOV: %d", 70 + (int)(m_appleFOV * 40.0f / 100.0f));
			return wstring(buf);
		}
	}
	case eControl_Gamma:
		return m_sliderGamma.getLabel();
	case eControl_InterfaceOpacity:
		return m_sliderInterfaceOpacity.getLabel();
	default:
		return L"";
	}
}

bool UIScene_SettingsGraphicsMenu::appleIsCheckboxControl(int controlId)
{
	return controlId == eControl_FancyGraphics ||
	       controlId == eControl_SmoothLighting ||
	       controlId == eControl_Clouds ||
	       controlId == eControl_BedrockFog ||
	       controlId == eControl_CustomSkinAnim;
}

bool UIScene_SettingsGraphicsMenu::appleIsCycleControl(int controlId)
{
	return controlId == eControl_RenderDistance ||
	       controlId == eControl_Particles;
}

bool UIScene_SettingsGraphicsMenu::appleIsSliderControl(int controlId)
{
	return controlId == eControl_FOV ||
	       controlId == eControl_Gamma ||
	       controlId == eControl_InterfaceOpacity;
}

bool UIScene_SettingsGraphicsMenu::appleGetCheckboxValue(int controlId)
{
	switch(controlId)
	{
	case eControl_FancyGraphics: return m_checkboxFancyGraphics.IsChecked();
	case eControl_SmoothLighting: return m_appleSmoothLighting;
	case eControl_Clouds: return m_checkboxClouds.IsChecked();
	case eControl_BedrockFog: return m_checkboxBedrockFog.IsChecked();
	case eControl_CustomSkinAnim: return m_checkboxCustomSkinAnim.IsChecked();
	default: return false;
	}
}

int UIScene_SettingsGraphicsMenu::appleGetSliderValue(int controlId)
{
	switch(controlId)
	{
	case eControl_FOV: return m_appleFOV;
	case eControl_Gamma: return app.GetGameSettings(m_iPad, eGameSetting_Gamma);
	case eControl_InterfaceOpacity: return app.GetGameSettings(m_iPad, eGameSetting_InterfaceOpacity);
	default: return 0;
	}
}

int UIScene_SettingsGraphicsMenu::appleGetSliderMin(int controlId) { return 0; }
int UIScene_SettingsGraphicsMenu::appleGetSliderMax(int controlId) { return 100; }

void UIScene_SettingsGraphicsMenu::appleActivateControl(int controlId)
{
	if(appleIsCheckboxControl(controlId))
	{
		bool nextValue = !appleGetCheckboxValue(controlId);
		switch(controlId)
		{
		case eControl_FancyGraphics:
			m_checkboxFancyGraphics.setChecked(nextValue);
			break;
		case eControl_SmoothLighting:
			m_appleSmoothLighting = nextValue;
			break;
		case eControl_Clouds:
			m_checkboxClouds.setChecked(nextValue);
			break;
		case eControl_BedrockFog:
			m_checkboxBedrockFog.setChecked(nextValue);
			break;
		case eControl_CustomSkinAnim:
			m_checkboxCustomSkinAnim.setChecked(nextValue);
			break;
		}
		ui.PlayUISFX(eSFX_Press);
		return;
	}

	if(appleIsCycleControl(controlId))
	{
		appleAdjustControl(controlId, 1);
		ui.PlayUISFX(eSFX_Press);
		return;
	}

	if(appleIsSliderControl(controlId))
	{
		appleAdjustControl(controlId, 10);
		ui.PlayUISFX(eSFX_Press);
	}
}

bool UIScene_SettingsGraphicsMenu::appleAdjustControl(int controlId, int delta)
{
	if(appleIsCycleControl(controlId))
	{
		switch(controlId)
		{
		case eControl_RenderDistance:
		{
			int newVal = (m_appleRenderDistance + delta + 4) & 3;
			if(newVal == m_appleRenderDistance) return false;
			m_appleRenderDistance = newVal;
			return true;
		}
		case eControl_Particles:
		{
			int newVal = (m_appleParticles + delta + 3) % 3;
			if(newVal == m_appleParticles) return false;
			m_appleParticles = newVal;
			return true;
		}
		}
		return false;
	}

	if(appleIsSliderControl(controlId))
	{
		int value = appleGetSliderValue(controlId);
		int newValue = clampAppleGraphicsValue(value + delta, appleGetSliderMin(controlId), appleGetSliderMax(controlId));
		if(newValue == value) return false;

		if(controlId == eControl_FOV)
		{
			m_appleFOV = newValue;
		}
		else
		{
			handleSliderMove((F64)controlId, (F64)newValue);
		}
		return true;
	}

	return false;
}
#endif
