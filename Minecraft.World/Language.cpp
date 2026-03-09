#include "stdafx.h"
#include "Language.h"

// 4J - TODO - properly implement

Language *Language::singleton = new Language();

Language::Language()
{
}

Language *Language::getInstance()
{
	return singleton;
}

/* 4J Jev, creates 2 identical functions.
wstring Language::getElement(const wstring& elementId)
{
	return elementId;
} */

wstring Language::getElement(const wstring elementId, ...)
{
//#ifdef __PSVITA__		// 4J - vita doesn't like having a reference type as the last parameter passed to va_start - we shouldn't need this method anyway
//	return L"";
//#else
	va_list args;
	va_start(args, elementId);
	return getElement(elementId, args);
//#endif
}

wstring Language::getElement(const wstring& elementId, va_list args)
{
#ifdef __APPLE__
	// Use the app's StringTable for translations on macOS
	extern wstring LanguageGetStringFromApp(const wstring &id);
	wstring result = LanguageGetStringFromApp(elementId);
	if (!result.empty()) return result;

	// Fallback: Console StringTable uses integer IDs, but the classic Java
	// Screen system uses wstring keys.  Provide English translations for all
	// keys the classic screens reference.
	static const struct { const wchar_t *key; const wchar_t *val; } kFallback[] = {
		// TitleScreen
		{L"menu.singleplayer", L"Singleplayer"},
		{L"menu.multiplayer",  L"Multiplayer"},
		{L"menu.mods",         L"Mods"},
		{L"menu.options",      L"Options"},
		{L"menu.quit",         L"Quit Game"},
		// Common GUI
		{L"gui.done",   L"Done"},
		{L"gui.cancel", L"Cancel"},
		{L"gui.yes",    L"Yes"},
		{L"gui.no",     L"No"},
		{L"gui.toMenu", L"Back to title screen"},
		// Options
		{L"options.title",      L"Options"},
		{L"options.video",      L"Video Settings..."},
		{L"options.controls",   L"Controls..."},
		{L"options.videoTitle",  L"Video Settings"},
		{L"options.music",       L"Music"},
		{L"options.sound",       L"Sound"},
		{L"options.invertMouse", L"Invert Mouse"},
		{L"options.sensitivity", L"Sensitivity"},
		{L"options.renderDistance", L"Render Distance"},
		{L"options.viewBobbing",   L"View Bobbing"},
		{L"options.ao",            L"Smooth Lighting"},
		{L"options.anaglyph",      L"3D Anaglyph"},
		{L"options.difficulty",    L"Difficulty"},
		{L"options.graphics",      L"Graphics"},
		{L"options.guiScale",      L"GUI Scale"},
		{L"options.fov",           L"FOV"},
		{L"options.gamma",         L"Brightness"},
		{L"options.particles",     L"Particles"},
		// Controls
		{L"controls.title", L"Controls"},
		// Select World
		{L"selectWorld.title",     L"Select World"},
		{L"selectWorld.empty",     L"empty"},
		{L"selectWorld.world",     L"World"},
		{L"selectWorld.select",    L"Play Selected World"},
		{L"selectWorld.create",    L"Create New World"},
		{L"selectWorld.delete",    L"Delete"},
		{L"selectWorld.rename",    L"Rename"},
		{L"selectWorld.deleteQuestion", L"Are you sure you want to delete this world?"},
		{L"selectWorld.deleteWarning",  L"will be lost forever! (A long time!)"},
		{L"selectWorld.renameTitle",    L"Rename World"},
		{L"selectWorld.renameButton",   L"Rename"},
		{L"selectWorld.conversion",     L"Must be converted!"},
		{L"selectWorld.newWorld",       L"New World"},
		{L"selectWorld.enterName",      L"World Name"},
		{L"selectWorld.resultFolder",   L"Will be saved in:"},
		{L"selectWorld.seedInfo",       L"Leave blank for a random seed"},
		{L"selectWorld.gameMode",       L"Game Mode"},
		// Create world
		{L"createWorld.customize.flat.title", L"Superflat Customization"},
		// Multiplayer
		{L"multiplayer.title",  L"Play Multiplayer"},
		{L"multiplayer.ipinfo", L"Enter the IP of a server to connect to it:"},
		{L"multiplayer.connect", L"Connect"},
		{L"multiplayer.stopSleeping", L"Leave Bed"},
		{L"multiplayer.downloadingTerrain", L"Downloading terrain"},
		// Connect
		{L"connect.connecting", L"Connecting to the server..."},
		{L"connect.authorizing", L"Logging in..."},
		// Death
		{L"deathScreen.respawn", L"Respawn"},
		{L"deathScreen.titleScreen", L"Title screen"},
		{L"deathScreen.title", L"You died!"},
		// Stats
		{L"stat.generalButton",  L"General"},
		{L"stat.blocksButton",   L"Blocks"},
		{L"stat.itemsButton",    L"Items"},
		// Demo
		{L"demo.help.title",     L"Minecraft Demo Mode"},
		{L"demo.help.movementShort", L"Use WASD and mouse to move"},
		{L"demo.help.buy",       L"Purchase the full game!"},
		// Game mode names
		{L"gameMode.survival", L"Survival Mode"},
		{L"gameMode.creative", L"Creative Mode"},
		{L"gameMode.hardcore", L"Hardcore Mode!"},
		// Difficulty
		{L"options.difficulty.peaceful", L"Peaceful"},
		{L"options.difficulty.easy",     L"Easy"},
		{L"options.difficulty.normal",   L"Normal"},
		{L"options.difficulty.hard",     L"Hard"},
	};
	for (size_t i = 0; i < sizeof(kFallback) / sizeof(kFallback[0]); ++i)
	{
		if (elementId == kFallback[i].key) return wstring(kFallback[i].val);
	}
#endif
	return elementId;
}

wstring Language::getElementName(const wstring& elementId)
{
	return elementId;
}

wstring Language::getElementDescription(const wstring& elementId)
{
	return elementId;
}