# Codebase Structure

**Analysis Date:** 2026-03-08

## Directory Layout

```
MinecraftCommunityEdition/
├── CMakeLists.txt                    # Root CMake (delegates to MacOSX/)
├── MinecraftConsoles.sln             # Visual Studio solution (Xbox/PS/Win/Durango targets)
├── Minecraft.Client.vcxproj          # VS project for client library
├── .planning/codebase/               # Architecture/planning docs
├── .github/ISSUE_TEMPLATE/           # GitHub issue templates
├── .gitlab-ci.yml                    # Legacy GitLab CI config
├── Minecraft.Client/                 # Client: rendering, UI, platform, server
│   ├── stdafx.h                      # Precompiled header (platform selection)
│   ├── Minecraft.h / .cpp            # Core game class
│   ├── MinecraftServer.h / .cpp      # Integrated server
│   ├── GameRenderer.h / .cpp         # Main render pipeline
│   ├── LevelRenderer.h / .cpp        # World/chunk rendering
│   ├── Tesselator.h / .cpp           # Vertex batching (OpenGL-style API)
│   ├── *Renderer.{h,cpp}             # Per-entity-type renderers (~30)
│   ├── *Model.{h,cpp}                # 3D mob/entity models (~25)
│   ├── *Particle.{h,cpp}             # Particle effect types (~25)
│   ├── *Screen.{h,cpp}               # GUI screens (~20)
│   ├── ClientConnection.{h,cpp}      # Client-side packet handler
│   ├── PlayerConnection.{h,cpp}      # Server-side packet handler
│   ├── ServerLevel.h / .cpp          # Server-side world
│   ├── ServerPlayer.h / .cpp         # Server-side player
│   ├── MultiPlayerLevel.h / .cpp     # Client-side world
│   ├── MultiPlayerLocalPlayer.h/.cpp # Client-side local player
│   ├── Common/                       # Shared console-edition code
│   │   ├── Consoles_App.{h,cpp}      # CMinecraftApp base class (huge: ~37K+294K lines)
│   │   ├── App_enums.h               # Action/state enums
│   │   ├── App_structs.h             # Shared data structures
│   │   ├── App_Defines.h             # Shared #defines
│   │   ├── Audio/                    # Sound engine abstraction
│   │   ├── Colours/                  # Colour table system
│   │   ├── DLC/                      # DLC pack management
│   │   ├── GameRules/                # Console-specific game rules, schematics
│   │   ├── Leaderboards/             # Leaderboard base class
│   │   ├── Media/                    # Localization, fonts, graphics assets
│   │   ├── Network/                  # Platform-independent networking
│   │   │   ├── GameNetworkManager.{h,cpp}
│   │   │   ├── PlatformNetworkManagerInterface.h
│   │   │   ├── PlatformNetworkManagerStub.{h,cpp}
│   │   │   ├── LANSessionManager.{h,cpp}
│   │   │   ├── SessionInfo.h
│   │   │   └── Sony/                 # Sony-specific network code
│   │   ├── Telemetry/                # Analytics/telemetry
│   │   ├── Trial/                    # Trial/demo mode logic
│   │   ├── Tutorial/                 # In-game tutorial system (~35 files)
│   │   ├── UI/                       # Console UI framework (~115 files)
│   │   │   ├── UIScene*.{h,cpp}      # Scene implementations (menus, HUD)
│   │   │   ├── UIControl*.{h,cpp}    # UI control widgets
│   │   │   ├── UIComponent*.{h,cpp}  # UI components (chat, tooltips, etc.)
│   │   │   ├── IUIScene_*.{h,cpp}    # Interface scene implementations
│   │   │   ├── UIEnums.h             # EUIScene, EUILayer, EUIGroup enums
│   │   │   └── UIController.cpp      # Shared UI controller logic
│   │   ├── XUI/                      # Xbox XUI-based UI framework (~90 files)
│   │   └── zlib/                     # Embedded zlib compression
│   ├── Xbox/                         # Xbox 360 platform layer
│   │   ├── Xbox_App.{h,cpp}
│   │   ├── Xbox_UIController.{h,cpp}
│   │   ├── Audio/
│   │   ├── Network/
│   │   ├── Leaderboards/
│   │   └── 4JLibs/inc/               # 4J middleware headers
│   ├── Durango/                      # Xbox One platform layer
│   │   ├── Durango_App.{h,cpp}
│   │   ├── Durango_UIController.{h,cpp}
│   │   ├── Network/
│   │   ├── Leaderboards/
│   │   ├── Sentient/                 # Telemetry
│   │   └── 4JLibs/inc/
│   ├── PS3/                          # PlayStation 3 platform layer
│   │   ├── PS3_App.{h,cpp}
│   │   ├── PS3_UIController.{h,cpp}
│   │   ├── SPU_Tasks/                # Cell SPU offload jobs (~15 tasks)
│   │   └── 4JLibs/inc/
│   ├── Orbis/                        # PlayStation 4 platform layer
│   │   ├── Orbis_App.{h,cpp}
│   │   ├── Orbis_UIController.{h,cpp}
│   │   ├── Network/
│   │   └── 4JLibs/inc/
│   ├── PSVita/                       # PS Vita platform layer
│   │   ├── PSVita_App.{h,cpp}
│   │   ├── PSVita_UIController.{h,cpp}
│   │   ├── Network/
│   │   └── 4JLibs/inc/
│   ├── Windows64/                    # Windows 64-bit platform layer
│   │   ├── Windows64_App.{h,cpp}
│   │   ├── Windows64_Minecraft.cpp
│   │   ├── Windows64_UIController.{h,cpp}
│   │   ├── Network/
│   │   └── 4JLibs/inc/
│   ├── Apple/                        # macOS compatibility stubs
│   │   ├── AppleStubs.h              # Windows API, Miles, DirectXMath stubs
│   │   ├── AppleStubs.cpp
│   │   ├── AppleLinkerStubs.cpp      # Linker symbol stubs
│   │   └── AppleInputStubs.cpp       # Input system stubs
│   ├── Vulkan/                       # Vulkan/MoltenVK rendering backend
│   │   ├── VulkanBootstrapApp.h      # Vulkan instance, swapchain, pipeline
│   │   ├── VulkanBootstrapAppCore.cpp
│   │   ├── VulkanBootstrapAppRender.cpp
│   │   ├── VulkanRenderManager.cpp   # C4JRender implementation for Vulkan
│   │   ├── Vulkan_UIController.{h,cpp} # ConsoleUIController stub
│   │   └── 4JLibs/inc/4J_Render.h    # Vulkan-specific render header
│   └── MacOSX/                       # macOS build target
│       ├── CMakeLists.txt            # CMake build (collects from .vcxproj)
│       ├── main.cpp                  # macOS entry point + game loop
│       ├── README.md
│       └── Shaders/                  # GLSL/SPIR-V shaders
└── Minecraft.World/                  # Game logic library (~1560 files)
    ├── stdafx.h                      # Precompiled header (includes Client headers!)
    ├── Level.{h,cpp}                 # World/level base class
    ├── LevelChunk.{h,cpp}            # Chunk data storage
    ├── Entity.{h,cpp}                # Entity base class
    ├── Mob.{h,cpp}                   # Mobile entity base
    ├── Player.{h,cpp}                # Player base class
    ├── Tile.{h,cpp}                  # Block/tile base class
    ├── Item.{h,cpp}                  # Item base class
    ├── *Tile.{h,cpp}                 # Specific block types (~80)
    ├── *Item.{h,cpp}                 # Specific item types (~30)
    ├── *Packet.{h,cpp}              # Network packets (~50+)
    ├── *Goal.{h,cpp}                 # AI behavior goals (~30)
    ├── *Command.{h,cpp}              # Server commands (~10)
    ├── *Layer.{h,cpp}                # Biome generation layers (~15)
    ├── *Enchantment.{h,cpp}          # Enchantment types (~10)
    ├── *Recipe.{h,cpp}               # Crafting recipes
    ├── *Dimension.{h,cpp}            # Dimension types (Overworld, Hell, End)
    ├── net.minecraft.*.h             # Header aggregation files (~55 files)
    ├── Connection.cpp                # Low-level network connection
    ├── Packet.cpp                    # Packet base class + registration
    ├── Random.{h,cpp}                # Java-compatible PRNG
    ├── Mth.{h,cpp}                   # Math utilities
    ├── compression.{h,cpp}           # Compression utilities
    ├── C4JThread.{h,cpp}             # Threading abstraction
    ├── Definitions.h                 # Type aliases, constants, array types
    ├── x64headers/                   # Compatibility headers for non-Xbox
    │   ├── extraX64.h                # Windows API type stubs
    │   ├── xuiapp.h                  # XUI app framework stubs
    │   ├── qnet.h                    # QNet networking stubs
    │   └── xmcore.h                  # XMCore stubs
    └── ConsoleSaveFile*.{h,cpp}      # Save file format handling
```

## Directory Purposes

**`Minecraft.Client/` (root files):**
- Purpose: Client-side rendering, game state management, and server integration
- Contains: ~520 files (256 .cpp + 264 .h) totaling ~85K lines
- Key files: `Minecraft.h` (core game class), `GameRenderer.h` (render pipeline), `LevelRenderer.h` (world render), `Tesselator.h` (vertex batching), `ClientConnection.h` (client-side networking), `MinecraftServer.h` (integrated server)

**`Minecraft.World/`:**
- Purpose: Platform-independent game logic shared between client and server
- Contains: ~1560 files (715 .cpp + 845 .h) totaling ~162K lines
- Key files: `Level.h` (world base), `Entity.h` (entity base), `Tile.h` (block base), `Packet.cpp` (network protocol), `Connection.cpp` (socket layer)

**`Minecraft.Client/Common/`:**
- Purpose: Console-edition shared code (not in original Java Minecraft)
- Contains: Application framework, UI system, DLC, game rules, tutorial, networking, telemetry
- Key files: `Consoles_App.h` (37K lines - the largest header), `Consoles_App.cpp` (294K lines - the largest source file)

**`Minecraft.Client/Common/UI/`:**
- Purpose: Console UI framework using Iggy (Flash/SWF) for scene rendering
- Contains: ~115 files implementing menu scenes, controls, components
- Key files: `UIScene.h` (base scene class), `UIEnums.h` (scene/layer/group enums), `UIController.cpp` (shared controller logic)

**`Minecraft.Client/Common/XUI/`:**
- Purpose: Xbox XUI-specific UI implementation (legacy Xbox 360 UI framework)
- Contains: ~90 files with XUI scene and control wrappers
- Note: Excluded from macOS build (replaced by Common/UI/)

**`Minecraft.Client/Common/Network/`:**
- Purpose: Game-level networking (session management, player tracking)
- Contains: `GameNetworkManager` (platform-independent), `PlatformNetworkManager` interface, LAN/NAT/STUN
- Key files: `GameNetworkManager.h` (main network API), `PlatformNetworkManagerInterface.h` (platform abstraction)

**`Minecraft.Client/{Platform}/`:**
- Purpose: Platform-specific implementations (one per target platform)
- Pattern: Each contains `*_App.{h,cpp}`, `*_UIController.{h,cpp}`, `Network/`, `Leaderboards/`, `4JLibs/inc/`
- Platforms: Xbox (360), Durango (Xbox One), PS3, Orbis (PS4), PSVita, Windows64, Apple (macOS stubs), Vulkan (render backend), MacOSX (build target)

**`Minecraft.World/x64headers/`:**
- Purpose: Stub/compatibility headers providing Xbox API types on non-Xbox platforms
- Contains: `extraX64.h` (DWORD, CRITICAL_SECTION, TLS, etc.), `xuiapp.h`, `qnet.h`, `xmcore.h`

**`Minecraft.Client/Common/Media/`:**
- Purpose: Localized assets, fonts, and graphics resources
- Contains: Language-specific subdirectories (de-DE, es-ES, fr-FR, etc.), font files, UI graphics

## Key File Locations

**Entry Points:**
- `Minecraft.Client/MacOSX/main.cpp`: macOS entry point (GLFW + Vulkan game loop)
- `Minecraft.Client/Windows64/Windows64_App.cpp`: Windows64 entry + game start
- `Minecraft.Client/Minecraft.cpp`: Core `Minecraft` class implementation
- `Minecraft.Client/MinecraftServer.cpp`: Integrated server implementation

**Configuration:**
- `CMakeLists.txt`: Root CMake config (macOS only)
- `Minecraft.Client/MacOSX/CMakeLists.txt`: macOS build config (source collection from vcxproj)
- `MinecraftConsoles.sln`: Visual Studio solution (all platforms)
- `Minecraft.Client.vcxproj`: Client VS project file
- `Minecraft.Client/stdafx.h`: Client precompiled header (platform selection)
- `Minecraft.World/stdafx.h`: World precompiled header (includes client headers)
- `Minecraft.Client/Options.h`: Game options/settings

**Core Game Logic:**
- `Minecraft.World/Level.h`: World/level base class
- `Minecraft.World/Entity.h`: Entity base class
- `Minecraft.World/Tile.h`: Block/tile base class
- `Minecraft.World/Item.h`: Item base class
- `Minecraft.World/Mob.h`: Mobile entity base
- `Minecraft.World/Player.h`: Player base class
- `Minecraft.World/Packet.cpp`: Network packet base + registration
- `Minecraft.World/Connection.cpp`: Raw network connection

**Rendering:**
- `Minecraft.Client/GameRenderer.h`: Main render pipeline
- `Minecraft.Client/LevelRenderer.h`: Chunk/world rendering
- `Minecraft.Client/Tesselator.h`: Vertex batching API
- `Minecraft.Client/EntityRenderDispatcher.h`: Entity render dispatch
- `Minecraft.Client/TileRenderer.cpp`: Block rendering
- `Minecraft.Client/Chunk.h`: Render chunk (16x16x16 tile region)

**Platform Abstraction:**
- `Minecraft.Client/Apple/AppleStubs.h`: macOS compatibility stubs
- `Minecraft.Client/Apple/AppleLinkerStubs.cpp`: Linker-level stubs
- `Minecraft.Client/Vulkan/VulkanBootstrapApp.h`: Vulkan rendering backend
- `Minecraft.Client/Vulkan/VulkanRenderManager.cpp`: C4JRender → Vulkan bridge
- `Minecraft.Client/Vulkan/Vulkan_UIController.h`: macOS UI controller stub

**Application Framework:**
- `Minecraft.Client/Common/Consoles_App.h`: CMinecraftApp base class
- `Minecraft.Client/Common/Consoles_App.cpp`: CMinecraftApp implementation (largest file)
- `Minecraft.Client/Common/App_enums.h`: Action/state enumerations
- `Minecraft.Client/Windows64/Windows64_App.h`: CConsoleMinecraftApp subclass (reused by macOS)

**Networking:**
- `Minecraft.Client/Common/Network/GameNetworkManager.h`: Game network API
- `Minecraft.Client/Common/Network/PlatformNetworkManagerInterface.h`: Platform network interface
- `Minecraft.Client/ClientConnection.h`: Client packet handler
- `Minecraft.Client/PlayerConnection.h`: Server packet handler
- `Minecraft.Client/ServerConnection.cpp`: Server-side connection manager

**Testing:**
- Not detected. No test files, test frameworks, or test directories exist in the codebase.

## Naming Conventions

**Files:**
- PascalCase for all source files: `EntityRenderDispatcher.cpp`, `LevelRenderer.h`
- Platform files prefixed with platform name: `Xbox_App.cpp`, `Orbis_UIController.h`, `Windows64_Minecraft.cpp`
- Java package convention for header aggregation: `net.minecraft.world.level.tile.h`
- Particle files: `{Name}Particle.cpp` (e.g., `FlameParticle.cpp`)
- Renderer files: `{EntityName}Renderer.cpp` (e.g., `CreeperRenderer.cpp`)
- Model files: `{EntityName}Model.cpp` (e.g., `CreeperModel.cpp`)
- Screen files: `{Purpose}Screen.cpp` (e.g., `DeathScreen.cpp`, `PauseScreen.cpp`)

**Directories:**
- PascalCase: `Minecraft.Client/`, `Minecraft.World/`
- Platform names: `Xbox/`, `Durango/`, `PS3/`, `Orbis/`, `PSVita/`, `Windows64/`, `Apple/`, `Vulkan/`, `MacOSX/`
- Feature grouping: `Common/UI/`, `Common/Network/`, `Common/DLC/`, `Common/Tutorial/`

**Classes:**
- PascalCase: `Minecraft`, `GameRenderer`, `LevelRenderer`, `EntityRenderDispatcher`
- 4J framework classes prefixed with `C`: `CMinecraftApp`, `CConsoleMinecraftApp`, `CGameNetworkManager`, `CPlatformNetworkManager`
- UI classes prefixed with `UI`: `UIScene`, `UIControl`, `UILayer`
- XUI classes prefixed with `XUI_`: `XUI_HUD`, `XUI_PauseMenu`
- Interface classes prefixed with `I`: `INetworkPlayer`, `IUIController`

**Members:**
- `m_` prefix for member variables: `m_instance`, `m_bGameStarted`, `m_eGameMode`
- `s_` prefix for static members: `s_pPlatformNetworkManager`, `s_bServerHalted`
- Hungarian notation on some variables: `b` (bool), `e` (enum), `ui` (unsigned int), `dw` (DWORD), `p` (pointer), `i` (int), `uc` (unsigned char)
- `iPad` parameter name for player/controller index (0-3), throughout entire codebase

## Where to Add New Code

**New Platform Port:**
1. Create platform directory: `Minecraft.Client/{PlatformName}/`
2. Implement: `{Platform}_App.{h,cpp}` (subclass `CMinecraftApp` via `CConsoleMinecraftApp`)
3. Implement: `{Platform}_UIController.{h,cpp}` (implement `ConsoleUIController` interface)
4. Implement: `4JLibs/inc/4J_Render.h` (implement `C4JRender` rendering backend)
5. Add platform stubs if needed (like `Apple/AppleStubs.h`)
6. Add `#elif defined __PLATFORM__` branches to both `stdafx.h` files
7. Add entry point with game loop (see `MacOSX/main.cpp` as template)
8. Update CMakeLists.txt or add platform-specific build config

**New Entity/Mob:**
- World logic: `Minecraft.World/{MobName}.{h,cpp}` (extend `Mob`, `Monster`, or `Animal`)
- AI goals: `Minecraft.World/{GoalName}Goal.{h,cpp}` (extend `Goal`)
- Renderer: `Minecraft.Client/{MobName}Renderer.{h,cpp}` (extend `MobRenderer`)
- Model: `Minecraft.Client/{MobName}Model.{h,cpp}` (extend `Model`)
- Register in `EntityRenderDispatcher.cpp` for rendering
- Add header to appropriate `net.minecraft.world.entity.*.h` aggregation file

**New Block/Tile:**
- Implementation: `Minecraft.World/{BlockName}Tile.{h,cpp}` (extend `Tile`)
- Tile entity (if needed): `Minecraft.World/{BlockName}TileEntity.{h,cpp}` (extend `TileEntity`)
- Renderer (if needed): `Minecraft.Client/{BlockName}Renderer.{h,cpp}`
- Add header to `Minecraft.World/net.minecraft.world.level.tile.h`

**New Item:**
- Implementation: `Minecraft.World/{ItemName}Item.{h,cpp}` (extend `Item`)
- Add header to `Minecraft.World/net.minecraft.world.item.h`

**New Network Packet:**
- Implementation: `Minecraft.World/{PacketName}Packet.{h,cpp}` (extend `Packet`)
- Add handler to `Minecraft.Client/ClientConnection.h` (client-side)
- Add handler to `Minecraft.Client/PlayerConnection.h` (server-side)
- Register in `Minecraft.World/Packet.cpp`

**New GUI Screen (legacy Java-style):**
- Implementation: `Minecraft.Client/{ScreenName}Screen.{h,cpp}` (extend `Screen`)

**New UI Scene (console-style):**
- Implementation: `Minecraft.Client/Common/UI/UIScene_{SceneName}.{h,cpp}` (extend `UIScene`)
- Add enum value to `EUIScene` in `Minecraft.Client/Common/UI/UIEnums.h`
- Register navigation in `ConsoleUIController` implementation

**New Particle:**
- Implementation: `Minecraft.Client/{ParticleName}Particle.{h,cpp}` (extend `Particle`)
- Add enum value to `ePARTICLE_TYPE` in `Minecraft.World/ParticleTypes.h`
- Register in `Minecraft.Client/ParticleEngine.cpp`

**New Game Rule:**
- Implementation: `Minecraft.Client/Common/GameRules/{RuleName}RuleDefinition.{h,cpp}` (extend `GameRuleDefinition`)

**Shared Utilities:**
- Math: `Minecraft.World/Mth.h`
- String helpers: `Minecraft.World/StringHelpers.h`
- Threading: `Minecraft.World/C4JThread.{h,cpp}`
- Random: `Minecraft.World/Random.{h,cpp}`
- Compression: `Minecraft.World/compression.{h,cpp}`

## Special Directories

**`build-macos/`:**
- Purpose: CMake build output directory for macOS target
- Generated: Yes
- Committed: No (in .gitignore)

**`Minecraft.Client/Common/Media/`:**
- Purpose: Localization files, fonts, and UI graphics
- Generated: No (checked-in assets)
- Committed: Yes
- Contains: Language directories (de-DE, es-ES, fr-FR, etc.), font/, Graphics/

**`Minecraft.Client/Common/DummyTexturePack/`:**
- Purpose: Fallback/placeholder texture pack
- Generated: No
- Committed: Yes

**`Minecraft.Client/MacOSX/Shaders/`:**
- Purpose: GLSL vertex/fragment shaders compiled to SPIR-V for Vulkan
- Generated: SPIR-V is generated at build time from GLSL sources
- Committed: GLSL sources committed, SPIR-V output is not

**`Minecraft.World/x64headers/`:**
- Purpose: Compatibility/stub headers providing Xbox API types on non-Xbox platforms
- Generated: No (hand-written stubs)
- Committed: Yes
- Key constraint: These headers define critical types (`CRITICAL_SECTION`, `DWORD`, `TLS*` functions) that the entire codebase depends on

---

*Structure analysis: 2026-03-08*
