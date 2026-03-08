# Coding Conventions

**Analysis Date:** 2026-03-08

## Naming Patterns

**Files:**
- Use PascalCase for class files: `GameRenderer.cpp`, `LevelChunk.h`, `EntityTracker.cpp`
- Each class gets a `.cpp` and `.h` pair in the same directory
- Java-style package aggregate headers use dot-separated names: `net.minecraft.world.entity.h`, `net.minecraft.world.level.tile.h` (these bundle related class headers into one include)
- Platform-specific files use platform prefix: `PS3_App.cpp`, `Orbis_Minecraft.cpp`, `Windows64_UIController.cpp`, `Vulkan_UIController.cpp`
- Precompiled header is always `stdafx.h`
- Stubs/shims use descriptive suffixes: `stubs.h`, `AppleStubs.h`, `OrbisStubs.h`, `AppleLinkerStubs.cpp`

**Classes:**
- PascalCase class names matching Java/Mojang originals: `Entity`, `LevelChunk`, `GameRenderer`, `ParticleEngine`
- 4J Studios console-specific classes use `C` prefix or `Console` prefix: `CMinecraftApp`, `ConsoleGameMode`, `ConsoleSchematicFile`
- UI classes use `UIScene_` or `UIControl_` prefix: `UIScene_PauseMenu`, `UIControl_Label`
- XUI (Xbox UI) classes use `XUI_` prefix: `XUI_HUD`, `XUI_Scene_Inventory`
- Model classes: `{EntityName}Model` (e.g., `ChickenModel`, `DragonModel`)
- Renderer classes: `{EntityName}Renderer` (e.g., `ChickenRenderer`, `PlayerRenderer`)
- Particle classes: `{Name}Particle` (e.g., `FlameParticle`, `SmokeParticle`)

**Functions:**
- camelCase for methods: `getSmallId()`, `setScreen()`, `respawnPlayer()`, `addLocalPlayer()`
- `_init()` for two-phase initialization (called from constructors): `Player::_init()`, `Mob::_init()`, `Connection::_init()`
- Static constructors: `staticCtor()` (e.g., `SharedConstants::staticCtor()`, `Packet::staticCtor()`)
- Getters use `get` prefix: `getLevel()`, `getConnection()`, `getAspectRatio()`
- Boolean getters use `is` prefix: `isClientSide()`, `isTutorial()`, `isRenderChunkEmpty()`
- Use `static` factory methods named `create` for packet factory pattern: `LoginPacket::create`

**Variables:**
- camelCase for most member variables: `entityId`, `walkDist`, `rainLevel`
- Old/previous frame values use `o` prefix: `oRainLevel`, `oThunderLevel`, `xo`, `yo`, `zo`, `yRotO`, `xRotO`
- `m_` prefix for 4J-added member variables: `m_instance`, `m_connectionFailed[]`, `m_pendingTextureRequests`, `m_bDisableAddNewTileEntities`
- Boolean `m_` members sometimes use `m_b` prefix: `m_bVisible`, `m_bUpdateOpacity`, `m_bDisableAddNewTileEntities`
- `s_` prefix for static members added by 4J: `s_iHTMLFontSizesA`
- Windows-style unsigned int prefix: `ui` in local vars (e.g., `uiFlags`, `uiMask`)
- Constants use UPPER_SNAKE_CASE: `MAX_TICK_TILES_PER_TICK`, `TOTAL_AIR_SUPPLY`, `TICKS_PER_DAY`
- `static const` for compile-time constants inside classes

**Enums:**
- Enum names use `e` prefix with PascalCase: `eINSTANCEOF`, `eXuiAction`, `eFont`, `EEntityDamageType`
- Enum values use `e` or `eTYPE_` prefix with UPPER_SNAKE_CASE: `eTYPE_ZOMBIE`, `eAppAction_Idle`, `eFont_European`
- Two naming styles coexist: `eTYPE_PLAYER` and `eType_ITEM` (inconsistent casing in the older enum)

**Typedefs:**
- Array typedefs use `{Type}Array` suffix: `byteArray`, `intArray`, `BiomeArray`, `LevelChunkArray`
- Use `typedef` for named structs: `typedef struct _JoinFromInviteData { ... } JoinFromInviteData;`

## Code Style

**Formatting:**
- No automated formatter configured (no `.clang-format`, `.editorconfig`, or equivalent)
- Tabs used for indentation in most files
- Braces on same line for functions and control structures (K&R style predominantly)
- Occasional inconsistency between files (some use Allman style)

**Linting:**
- No linting tools configured
- MSVC warnings selectively disabled via `#pragma warning(disable : 4250)` for known diamond inheritance issues

**Platform Guards:**
- Use preprocessor defines for platform branching: `_XBOX`, `__PS3__`, `_DURANGO`, `__ORBIS__`, `__PSVITA__`, `_WINDOWS64`, `__APPLE__`
- Guard blocks follow `#ifdef ... #elif defined ... #else ... #endif` pattern
- The `stdafx.h` precompiled header handles platform-specific include routing
- New platforms (macOS) add `__APPLE__` guards matching the `__ORBIS__` pattern where possible

**Header Guards:**
- Use `#pragma once` exclusively (no `#ifndef` guards)

## Import Organization

**Order:**
1. `#include "stdafx.h"` -- always first in `.cpp` files (precompiled header)
2. Own header (e.g., `#include "GameRenderer.h"`)
3. Other same-project headers (e.g., `#include "Textures.h"`, `#include "Chunk.h"`)
4. Cross-project headers with relative paths (e.g., `#include "../Minecraft.World/net.minecraft.world.level.h"`)
5. Third-party/system headers (included transitively via `stdafx.h`)

**Path Conventions:**
- Client-to-World includes use `../Minecraft.World/` relative path
- World-to-Client includes use `../Minecraft.Client/` relative path (bidirectional coupling exists)
- Java-package-style aggregate headers group related includes: include `net.minecraft.world.entity.h` instead of individual entity headers
- Platform-specific includes are routed through `stdafx.h` conditional blocks

**Path Aliases:**
- None. All includes use relative paths.

## Error Handling

**Patterns:**
- Java-style exception classes that derive from `std::exception`: `EOFException`, `IllegalArgumentException`, `IOException`, `RuntimeException` (defined in `Minecraft.World/Exceptions.h`)
- Exception classes carry `wstring information` members (wide strings)
- `assert()` used liberally for programmer errors and invariant checks
- `__debugbreak()` used for critical failures that should never happen (e.g., running out of entity IDs in `Entity::getSmallId()`)
- `app.DebugPrintf()` for runtime diagnostic output
- Error codes use Windows HRESULT pattern: `SUCCEEDED(hr)`, `FAILED(hr)`, `S_OK`, `E_FAIL`
- In final builds, `printf`/`wprintf`/`OutputDebugString` are redefined to `BREAKTHECOMPILE` to prevent accidental debug output (see `stdafx.h` line 377)
- No structured error propagation -- most functions return void or use boolean success flags

**Resource Management:**
- `shared_ptr` and `weak_ptr` used for entity ownership (e.g., `shared_ptr<Entity>`, `weak_ptr<Entity> rider`)
- Raw pointers still widely used for non-owned references (e.g., `Level *level`, `Random *random`)
- `CRITICAL_SECTION` used for thread synchronization (Windows threading API)
- `InitializeCriticalSection()`/`DeleteCriticalSection()` in constructors/destructors
- Manual `new`/`delete` still common alongside smart pointers

## Logging

**Framework:** Custom `app.DebugPrintf()` (global `CConsoleMinecraftApp app` instance)

**Patterns:**
- Debug output via `app.DebugPrintf("message\n")` -- printf-style formatting
- Commented-out debug prints are common: `// app.DebugPrintf("getSmallId: Removed entities found\n");`
- `OutputDebugString()` / `OutputDebugStringA()` used on Windows platforms
- All debug output is disabled in final builds via macro redefinition

## Comments

**When to Comment:**
- `// 4J` prefix marks all changes made by 4J Studios to the original Mojang code. This is the dominant comment pattern. Examples:
  - `// 4J - added` or `// 4J Added` for new code
  - `// 4J - removed` for deleted functionality
  - `// 4J Stu` for changes by developer "Stu"
  - `// 4J-PB` for changes by developer "PB"
  - `// 4J : WESTY :` for changes by developer "Westy"
  - `// 4J-JEV` for changes by developer "Jev"
  - `// 4J-RR` for changes by developer "RR"
  - `// 4J TODO` for known incomplete work
- Version markers in aggregate headers: `// 1.8.2`, `// 1.0.1`, `// TU9`, `// 1.2.3`
- `@TODO:` or `TODO` for outstanding work items
- Commented-out code is left in place with `//` rather than being deleted

**JSDoc/TSDoc:**
- Not applicable (C++ codebase). No Doxygen or other doc-comment convention is used.

## Function Design

**Size:**
- No enforced size limit. Large files exist: `TileRenderer.cpp` (7694 lines), `Minecraft.cpp` (5087 lines), `Level.cpp` (4752 lines)
- Functions tend to be long, especially rendering and game logic code

**Parameters:**
- `const wstring&` for string parameters (wide strings used throughout)
- `shared_ptr<T>` passed by value for shared ownership
- Raw pointers for non-owning references
- `int iPad` parameter name used for player/controller index (originally iPad index on Xbox, name persists)
- Default parameter values used: `void setLevel(..., bool doForceStatsSave = true, bool bPrimaryPlayerSignedOut = false)`

**Return Values:**
- `bool` for success/failure
- Raw pointers for nullable lookups (return `NULL` on failure)
- `void` for side-effectful operations
- `wstring` for string data (always wide strings)

## Module Design

**Exports:**
- No module system. All symbols are internal to the compilation unit.
- `public`/`private`/`protected` access specifiers used within classes (sometimes toggled multiple times within a single class declaration)

**Barrel Files:**
- The `net.minecraft.*.h` headers serve as aggregate/barrel includes, grouping related class headers by Java package name

## String Handling

**Convention:** Use `wstring` (wide strings) throughout the codebase.
- String literals use `L""` prefix: `L"/mob/char.png"`, `L"KEYNAME"`
- Helper functions in `Minecraft.World/StringHelpers.h`: `toLower()`, `trimString()`, `replaceAll()`, `equalsIgnoreCase()`
- Conversion helpers: `convStringToWstring()`, `wstringtofilename()`, `filenametowstring()`
- Templated `_toString<T>()` and `_fromString<T>()` for number/string conversion

## Platform Abstraction

**Pattern:** Platform-specific code lives in platform-named subdirectories under `Minecraft.Client/`:
- `Xbox/`, `PS3/`, `Durango/`, `Orbis/`, `PSVita/`, `Windows64/`, `Apple/`, `Vulkan/`
- Each platform provides its own implementations of: `*_App.cpp`, `*_UIController.cpp`, `*_Minecraft.cpp`, sound engine, network manager, leaderboard manager
- The `stdafx.h` precompiled header routes includes to the correct platform implementation via `#ifdef` chains
- Stub/shim files provide no-op implementations for missing platform APIs (e.g., `AppleStubs.h` stubs Windows API, Miles Sound, DirectXMath)
- `stubs.h` in `Minecraft.Client/` provides stub implementations of OpenGL and Java API wrappers (Keyboard, Mouse, Display, etc.)

## Macro Conventions

**Naming:** UPPER_SNAKE_CASE for macros
- Bitmask macros use `MAKE_*` and `GET_*` prefixes: `MAKE_SLOTDISPLAY_DATA_BITMASK()`, `GET_SLOTDISPLAY_ALPHA_FROM_DATA_BITMASK()`
- UI mapping macros use `UI_` prefix: `UI_BEGIN_MAP_ELEMENTS_AND_NAMES()`, `UI_MAP_ELEMENT()`
- Platform detection: `_XBOX`, `__PS3__`, `_DURANGO`, `__ORBIS__`, `__PSVITA__`, `_WINDOWS64`, `__APPLE__`
- `AUTO_VAR(_var, _val)` macro wraps `auto` for platforms that may not support C++11 `auto`

---

*Convention analysis: 2026-03-08*
