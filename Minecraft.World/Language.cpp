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
		// Achievement popup
		{L"achievement.get", L"Achievement Get!"},
		// Achievement names
		{L"achievement.openInventory",       L"Taking Inventory"},
		{L"achievement.mineWood",            L"Getting Wood"},
		{L"achievement.buildWorkBench",      L"Benchmarking"},
		{L"achievement.buildPickaxe",        L"Time to Mine!"},
		{L"achievement.buildFurnace",        L"Hot Topic"},
		{L"achievement.acquireIron",         L"Acquire Hardware"},
		{L"achievement.buildHoe",            L"Time to Farm!"},
		{L"achievement.makeBread",           L"Bake Bread"},
		{L"achievement.bakeCake",            L"The Lie"},
		{L"achievement.buildBetterPickaxe",  L"Getting an Upgrade"},
		{L"achievement.cookFish",            L"Delicious Fish"},
		{L"achievement.onARail",             L"On A Rail"},
		{L"achievement.buildSword",          L"Time to Strike!"},
		{L"achievement.killEnemy",           L"Monster Hunter"},
		{L"achievement.killCow",             L"Cow Tipper"},
		{L"achievement.flyPig",              L"When Pigs Fly"},
		{L"achievement.snipeSkeleton",       L"Sniper Duel"},
		{L"achievement.diamonds",            L"DIAMONDS!"},
		{L"achievement.ghast",               L"Return to Sender"},
		{L"achievement.blazeRod",            L"Into Fire"},
		{L"achievement.potion",              L"Local Brewery"},
		{L"achievement.theEnd",              L"The End?"},
		{L"achievement.theEnd2",             L"The End."},
		{L"achievement.enchantments",        L"Enchanter"},
		{L"achievement.overkill",            L"Overkill"},
		{L"achievement.bookcase",            L"Librarian"},
		{L"achievement.leaderOfThePack",     L"Leader of the Pack"},
		{L"achievement.MOARTools",           L"MOAR Tools"},
		{L"achievement.dispenseWithThis",    L"Dispense With This"},
		{L"achievement.InToTheNether",       L"Into The Nether"},
		{L"achievement.mine100Blocks",       L"Mine 100 Blocks"},
		{L"achievement.kill10Creepers",      L"Kill 10 Creepers"},
		{L"achievement.eatPorkChop",         L"Pork Chop"},
		{L"achievement.play100Days",         L"Passing the Time"},
		{L"achievement.arrowKillCreeper",    L"Archer"},
		{L"achievement.socialPost",          L"Sharing is Caring"},
		{L"achievement.adventuringTime",     L"Adventuring Time"},
		{L"achievement.repopulation",        L"Repopulation"},
		{L"achievement.diamondsToYou",       L"Diamonds to you!"},
		{L"achievement.theHaggler",          L"The Haggler"},
		{L"achievement.potPlanter",          L"Pot Planter"},
		{L"achievement.itsASign",            L"It's a Sign!"},
		{L"achievement.ironBelly",           L"Iron Belly"},
		{L"achievement.haveAShearfulDay",    L"Have a Shearful Day"},
		{L"achievement.rainbowCollection",   L"Rainbow Collection"},
		{L"achievement.stayingFrosty",       L"Stayin' Frosty"},
		{L"achievement.chestfulOfCobblestone", L"Chestful of Cobblestone"},
		{L"achievement.renewableEnergy",     L"Renewable Energy"},
		{L"achievement.musicToMyEars",       L"Music to my Ears"},
		{L"achievement.bodyGuard",           L"Body Guard"},
		{L"achievement.ironMan",             L"Iron Man"},
		{L"achievement.zombieDoctor",        L"Zombie Doctor"},
		{L"achievement.lionTamer",           L"Lion Tamer"},
		// Achievement descriptions
		{L"achievement.openInventory.desc",       L"Press 'E' to open your inventory."},
		{L"achievement.mineWood.desc",            L"Attack a tree until a block of wood pops out."},
		{L"achievement.buildWorkBench.desc",      L"Craft a workbench with four blocks of planks."},
		{L"achievement.buildPickaxe.desc",        L"Use planks and sticks to make a pickaxe."},
		{L"achievement.buildFurnace.desc",        L"Construct a furnace out of eight cobblestone blocks."},
		{L"achievement.acquireIron.desc",         L"Smelt an iron ingot."},
		{L"achievement.buildHoe.desc",            L"Use planks and sticks to make a hoe."},
		{L"achievement.makeBread.desc",           L"Turn wheat into bread."},
		{L"achievement.bakeCake.desc",            L"Wheat, sugar, milk and eggs!"},
		{L"achievement.buildBetterPickaxe.desc",  L"Construct a better pickaxe."},
		{L"achievement.cookFish.desc",            L"Catch and cook a fish!"},
		{L"achievement.onARail.desc",             L"Travel by minecart at least 1 km from where you started."},
		{L"achievement.buildSword.desc",          L"Use planks and sticks to make a sword."},
		{L"achievement.killEnemy.desc",           L"Attack and destroy a monster."},
		{L"achievement.killCow.desc",             L"Harvest some leather."},
		{L"achievement.flyPig.desc",              L"Fly a pig off a cliff."},
		{L"achievement.snipeSkeleton.desc",       L"Kill a skeleton with an arrow from more than 50 meters."},
		{L"achievement.diamonds.desc",            L"Acquire diamonds with your iron tools."},
		{L"achievement.ghast.desc",               L"Destroy a Ghast with a fireball."},
		{L"achievement.blazeRod.desc",            L"Relieve a Blaze of its rod."},
		{L"achievement.potion.desc",              L"Brew a potion."},
		{L"achievement.theEnd.desc",              L"Locate the End."},
		{L"achievement.theEnd2.desc",             L"Defeat the Ender Dragon."},
		{L"achievement.enchantments.desc",        L"Use a book, obsidian and diamonds to construct an enchantment table."},
		{L"achievement.overkill.desc",            L"Deal nine hearts of damage in a single hit."},
		{L"achievement.bookcase.desc",            L"Build some bookshelves to improve your enchantment table."},
		{L"achievement.leaderOfThePack.desc",     L"Befriend five wolves."},
		{L"achievement.MOARTools.desc",           L"Construct one type of each tool."},
		{L"achievement.dispenseWithThis.desc",    L"Construct a Dispenser."},
		{L"achievement.InToTheNether.desc",       L"Construct a Nether Portal."},
		{L"achievement.mine100Blocks.desc",       L"Mine 100 blocks."},
		{L"achievement.kill10Creepers.desc",      L"Kill 10 creepers."},
		{L"achievement.eatPorkChop.desc",         L"Cook and eat a pork chop."},
		{L"achievement.play100Days.desc",         L"Play for 100 days."},
		{L"achievement.arrowKillCreeper.desc",    L"Kill a creeper with arrows."},
		{L"achievement.socialPost.desc",          L"Take a screenshot and share it."},
		{L"achievement.adventuringTime.desc",     L"Discover all biomes."},
		{L"achievement.repopulation.desc",        L"Breed two cows with wheat."},
		{L"achievement.diamondsToYou.desc",       L"Throw diamonds at another player."},
		{L"achievement.theHaggler.desc",          L"Acquire or spend 30 Emeralds by trading."},
		{L"achievement.potPlanter.desc",          L"Craft and place a Flower Pot."},
		{L"achievement.itsASign.desc",            L"Craft and place a Sign."},
		{L"achievement.ironBelly.desc",           L"Stop starvation using Rotten Flesh."},
		{L"achievement.haveAShearfulDay.desc",    L"Use Shears to obtain wool from a sheep."},
		{L"achievement.rainbowCollection.desc",   L"Gather all 16 colors of wool."},
		{L"achievement.stayingFrosty.desc",       L"Swim in lava while having the Fire Resistance effect."},
		{L"achievement.chestfulOfCobblestone.desc", L"Mine 1,728 Cobblestone and place it in a chest."},
		{L"achievement.renewableEnergy.desc",     L"Smelt wood trunks using charcoal to make more charcoal."},
		{L"achievement.musicToMyEars.desc",       L"Play a music disc in a Jukebox."},
		{L"achievement.bodyGuard.desc",           L"Create an Iron Golem."},
		{L"achievement.ironMan.desc",             L"Wear a full suit of Iron Armor."},
		{L"achievement.zombieDoctor.desc",        L"Cure a zombie villager."},
		{L"achievement.lionTamer.desc",           L"Tame an Ocelot."},
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