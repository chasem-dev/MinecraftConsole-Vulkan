# Architecture

**Analysis Date:** 2026-03-08

## Pattern Overview

**Overall:** Monolithic client-server architecture with a platform abstraction layer (PAL). The codebase is a C++ port of Minecraft Java Edition (circa 1.6.4) by 4J Studios, targeting multiple console platforms. It uses a two-library split: `Minecraft.World` (game logic, shared) and `Minecraft.Client` (rendering, UI, platform, networking). Both client and server run in the same process (integrated server model).

**Key Characteristics:**
- Java-to-C++ port retaining Java-like class hierarchies (e.g., `net.minecraft.*` header aggregation files)
- Multi-platform console support via `#ifdef` preprocessor branching and platform-specific subdirectories
- 4J Studios middleware layer (`C4JRender`, `C4JStorage`, `C4JProfile`, `C4JInput`) abstracts platform APIs
- Split-screen multiplayer support (up to 4 players) is fundamental to the architecture, not bolted on
- Precompiled header chain (`stdafx.h`) selects platform types, stubs, and 4J libraries per `#define`
- macOS/Vulkan port (community edition) adds a new platform layer alongside existing Xbox/PS3/PS4/Vita/Durango/Windows64

## Layers

**Platform Abstraction Layer (PAL):**
- Purpose: Provides platform-specific implementations for rendering, input, storage, networking, audio, and UI
- Location: `Minecraft.Client/{Xbox,Durango,PS3,Orbis,PSVita,Windows64,Apple,Vulkan,MacOSX}/`
- Contains: Platform app classes (`*_App.cpp`), UI controllers (`*_UIController.cpp`), network managers, leaderboards, sound engines
- Depends on: 4J middleware headers (`4JLibs/inc/4J_*.h`), platform SDKs
- Used by: `Minecraft.Client/Common/Consoles_App.h` (base class), game loop entry points

**4J Middleware Layer:**
- Purpose: Abstracts platform-specific APIs behind a common interface (render, storage, input, profile, networking)
- Location: `Minecraft.Client/{platform}/4JLibs/inc/4J_Render.h`, `4J_Storage.h`, `4J_Input.h`, `4J_Profile.h`
- Contains: `C4JRender` (RenderManager), `C4JStorage` (StorageManager), `C4JInput` (InputManager), `C_4JProfile` (ProfileManager) singleton APIs
- Depends on: Platform SDKs (DirectX, GNM, Vulkan/MoltenVK, etc.)
- Used by: All game code through global singletons (`RenderManager`, `StorageManager`, `InputManager`, `ProfileManager`)

**Common Console Layer:**
- Purpose: Shared console-edition code that is platform-independent but console-specific (not in original Java)
- Location: `Minecraft.Client/Common/`
- Contains: Application framework (`Consoles_App.h/.cpp`), UI system (`Common/UI/`), XUI framework (`Common/XUI/`), DLC management (`Common/DLC/`), game rules (`Common/GameRules/`), tutorial system (`Common/Tutorial/`), networking (`Common/Network/`), audio (`Common/Audio/`), telemetry (`Common/Telemetry/`), trial mode (`Common/Trial/`)
- Depends on: 4J Middleware, Minecraft.World
- Used by: Platform layer (inherits from `CMinecraftApp`), game client

**Client Rendering Layer:**
- Purpose: All visual rendering - entities, tiles, particles, GUI screens, level rendering
- Location: `Minecraft.Client/` (root-level `.cpp/.h` files)
- Contains: Entity renderers (`*Renderer.cpp`), entity models (`*Model.cpp`), particles (`*Particle.cpp`), GUI screens (`*Screen.cpp`), textures (`Texture*.cpp`), tesselator (`Tesselator.cpp`), level rendering (`LevelRenderer.cpp`, `GameRenderer.cpp`, `Chunk.cpp`)
- Depends on: Minecraft.World (entities, level, tiles), 4J Middleware (C4JRender)
- Used by: Game loop (`Minecraft::run_middle()`)

**Game Logic Layer (Minecraft.World):**
- Purpose: Core game simulation - entities, tiles/blocks, items, level/world, AI, networking protocol, crafting, dimensions
- Location: `Minecraft.World/`
- Contains: Entity hierarchy (`Entity.cpp`, `Mob.cpp`, `Player.cpp`, animals, monsters), tiles/blocks (`*Tile.cpp`), items (`*Item.cpp`), level management (`Level.cpp`, `LevelChunk.cpp`), AI goals (`*Goal.cpp`), network packets (`*Packet.cpp`), world generation (`RandomLevelSource.cpp`, biomes), inventory/crafting, NBT serialization
- Depends on: 4J Middleware (through stdafx.h), platform stubs
- Used by: Minecraft.Client (rendering, UI, networking)

**Server Layer:**
- Purpose: Integrated game server running in-process with the client
- Location: `Minecraft.Client/MinecraftServer.h`, `Minecraft.Client/ServerLevel.h`, `Minecraft.Client/ServerPlayer.h`, `Minecraft.Client/PlayerConnection.h`, `Minecraft.Client/ServerConnection.cpp`
- Contains: Server tick loop, player list management, entity tracking, chunk map, server-side packet handling
- Depends on: Minecraft.World (Level, Entity), Common/Network (GameNetworkManager)
- Used by: Game network thread (`MinecraftServer::run()`)

**Networking Layer:**
- Purpose: Multiplayer session management, packet routing, platform-specific network backends
- Location: `Minecraft.Client/Common/Network/` (shared), `Minecraft.Client/{platform}/Network/` (platform-specific)
- Contains: `CGameNetworkManager` (game-side interface), `CPlatformNetworkManager` (platform interface), `INetworkPlayer`, QNet integration, LAN/session discovery
- Depends on: 4J Middleware, QNet library (`qnet.h`), platform network SDKs
- Used by: Server layer, client connection layer

## Data Flow

**Game Loop (per frame):**

1. Platform entry point calls `glfwPollEvents()` / platform equivalent
2. `RenderManager.StartFrame()` begins GPU frame
3. `app.UpdateTime()` advances timing
4. `InputManager.Tick()` polls controller/keyboard state
5. `StorageManager.Tick()` processes async save/load operations
6. `RenderManager.Tick()` updates render state
7. `g_NetworkManager.DoWork()` processes network events
8. If game started: `pMinecraft->run_middle()` runs game tick + render
9. `ui.tick()` + `ui.render()` processes console UI overlay
10. `RenderManager.Present()` submits frame to GPU
11. `app.HandleXuiActions()` processes deferred UI actions

**Game Tick (inside `Minecraft::run_middle()`):**

1. Timer calculates number of ticks to process this frame
2. For each tick: `Minecraft::tick()` updates game state
3. Tick calls `level->tick()` to advance world simulation
4. Server level ticks entities, tile events, weather, sleeping
5. Client level resets tile changes, animates particles
6. `GameRenderer::render()` performs actual 3D rendering
7. Entity rendering dispatched through `EntityRenderDispatcher`
8. Level chunks rebuilt on dirty flag via `LevelRenderer::updateDirtyChunks()`

**Network Packet Flow (Local Multiplayer):**

1. Server creates `ServerPlayer` + `PlayerConnection` per connected client
2. `PlayerConnection::tick()` reads incoming packets from `Connection`
3. Packet handlers (e.g., `handleMovePlayer`, `handlePlayerAction`) update server state
4. Server sends response packets (e.g., `ChunkVisibilityPacket`, `TileUpdatePacket`)
5. `ClientConnection::tick()` processes incoming server packets
6. Client updates `MultiPlayerLevel` state based on received packets

**State Management:**
- World state lives in `Level` / `ServerLevel` / `MultiPlayerLevel` instances (up to 3 dimensions: overworld, nether, end)
- Per-player state in `ServerPlayer` / `MultiplayerLocalPlayer` with arrays indexed by controller pad (`iPad` parameter, 0-3)
- Game settings stored via `C_4JProfile` / `C4JStorage` profile system
- Save data uses custom binary format via `ConsoleSaveFile` / `ConsoleSaveFileSplit`

## Key Abstractions

**CMinecraftApp (Application Framework):**
- Purpose: Central application singleton managing game lifecycle, settings, UI actions, DLC, save/load
- Base: `Minecraft.Client/Common/Consoles_App.h` (`CMinecraftApp`)
- Platform subclass: `CConsoleMinecraftApp` (per-platform `*_App.h`)
- Pattern: Template Method - virtual methods for platform-specific behavior (`SetRichPresenceContext`, `ExitGame`, `FatalLoadError`)
- Global instance: `extern CConsoleMinecraftApp app;`

**ConsoleUIController (UI System):**
- Purpose: Manages UI scene navigation, tooltips, HUD elements, menu state
- Base: Platform-specific implementations (`Xbox_UIController.h`, `Vulkan_UIController.h`, etc.)
- Pattern: Scene stack with per-player navigation (`NavigateToScene`, `NavigateBack`)
- Global instance: `extern ConsoleUIController ui;`
- Scene types enumerated in `Minecraft.Client/Common/UI/UIEnums.h` (`EUIScene`)

**Level Hierarchy:**
- Purpose: Represents a game world/dimension
- `Level` (`Minecraft.World/Level.h`) - Base class with entities, tiles, chunks, tick logic
- `ServerLevel` (`Minecraft.Client/ServerLevel.h`) - Server-side level with entity tracking, chunk map, save/load
- `MultiPlayerLevel` (`Minecraft.Client/MultiPlayerLevel.h`) - Client-side level with packet-driven updates
- Pattern: Inheritance hierarchy with virtual `tick()`, `addEntity()`, `tickTiles()`

**Entity System:**
- Purpose: All game objects (mobs, items, projectiles, players)
- `Entity` (`Minecraft.World/Entity.cpp`) - Base with position, motion, collision, serialization
- `Mob` → `Monster` / `Animal` → specific mobs (Creeper, Cow, etc.)
- `Player` → `ServerPlayer` (server-side) / `LocalPlayer` → `MultiplayerLocalPlayer` (client-side)
- Pattern: Deep inheritance hierarchy with virtual dispatch, `shared_ptr<Entity>` ownership

**Packet System:**
- Purpose: Network protocol between client and server
- `Packet` base class → specific packets (`MovePlayerPacket`, `TileUpdatePacket`, etc.)
- `PacketListener` interface implemented by `ClientConnection` and `PlayerConnection`
- Header aggregation: `Minecraft.World/net.minecraft.network.packet.h`
- Pattern: Visitor-like dispatch via virtual `handle*` methods on listener

**Tile/Block System:**
- Purpose: World blocks (stone, dirt, chests, etc.)
- `Tile` base class with static array of all tile types
- `TileEntity` for blocks with extra state (chests, furnaces, signs)
- Header aggregation: `Minecraft.World/net.minecraft.world.level.tile.h`

**CPlatformNetworkManager:**
- Purpose: Abstract interface for platform-specific networking
- Location: `Minecraft.Client/Common/Network/PlatformNetworkManagerInterface.h`
- Implementations: `CPlatformNetworkManagerXbox`, `CPlatformNetworkManagerSony`, `CPlatformNetworkManagerDurango`, `CPlatformNetworkManagerStub`
- Pattern: Pure virtual interface with platform-selected implementation via `#ifdef`

## Entry Points

**macOS (Community Edition):**
- Location: `Minecraft.Client/MacOSX/main.cpp`
- Triggers: Direct executable launch
- Responsibilities: GLFW window creation, Vulkan init via `VulkanBootstrapApp`, 4J middleware init (`RenderManager`, `InputManager`, `StorageManager`, `ProfileManager`), thread-local storage setup, calls `Minecraft::main()`, runs main game loop, cleanup

**Windows64:**
- Location: `Minecraft.Client/Windows64/Windows64_Minecraft.cpp` (not directly visible but referenced)
- Triggers: WinMain or equivalent
- Responsibilities: Same flow as macOS but with D3D11 rendering backend

**Original Console Platforms:**
- Location: `Minecraft.Client/{Xbox,PS3,Orbis,PSVita,Durango}/*_Minecraft.cpp`
- Responsibilities: Platform-specific init, game loop integration with platform SDK

**Game Initialization:**
- `Minecraft::main()` - Static entry: registers tiles/items/biomes, calls `Minecraft::start()`
- `Minecraft::start()` - Creates `Minecraft` instance, calls `run()`
- `Minecraft::init()` - Initializes rendering subsystems, textures, font, sound
- `Minecraft::run()` / `run_middle()` / `run_end()` - Split game loop for platform integration

**Server Initialization:**
- `MinecraftServer::main()` - Static entry called from `CGameNetworkManager::RunNetworkGameThreadProc()`
- `MinecraftServer::run()` - Server tick loop (20 ticks/second)
- Runs on dedicated thread via `CGameNetworkManager::ServerThreadProc()`

## Error Handling

**Strategy:** Minimal structured error handling. The codebase relies primarily on null checks, boolean return values, and `assert()` macros. Exception usage is rare (primarily in the macOS bootstrap `main.cpp` with a top-level try/catch).

**Patterns:**
- NULL pointer guards before use (e.g., `if (pMinecraft == nullptr) return 1;`)
- Boolean success/failure return from init functions
- `CRITICAL_SECTION` for thread safety (Windows API pattern, stubbed on non-Windows)
- Signal handlers (`SIGSEGV`, `SIGABRT`) in macOS entry point for crash diagnostics
- `DisconnectPacket::eDisconnectReason` enum for network disconnect categorization
- `C4JStorage::EMessageResult` for save/load error reporting
- Debug `printf` gated by `#ifndef _FINAL_BUILD`

## Cross-Cutting Concerns

**Threading:**
- Main thread: rendering + game tick
- Server thread: `MinecraftServer::run()` via `ServerThreadProc()`
- Network thread: `RunNetworkGameThreadProc()`
- Chunk rebuild threads: `LevelRenderer::rebuildChunkThreadProc()` (up to 3 on large worlds)
- Post-process thread: `MinecraftServer::runPostUpdate()` for chunk post-processing
- Level tick thread (optional): `Minecraft::levelTickUpdateFunc()` (currently `#define DISABLE_LEVELTICK_THREAD`)
- Thread synchronization: `CRITICAL_SECTION`, `C4JThread::Event`, `C4JThread::EventArray`
- Thread-local storage (`TlsAlloc/TlsGetValue/TlsSetValue`): Used by `Tesselator`, `Entity`, `AABB`, `Vec3`, `IntCache`, `Level`, `Tile`

**Platform Branching:**
- Primary defines: `_XBOX`, `_DURANGO`, `__PS3__`, `__ORBIS__`, `__PSVITA__`, `_WINDOWS64`, `__APPLE__`
- Preprocessor `#ifdef` chains in `stdafx.h` files select platform headers, types, and implementations
- Platform stubs provide compatibility types (`CRITICAL_SECTION`, `HRESULT`, `DWORD`, `LPVOID`, `__int64`) on non-Windows
- AppleStubs.h provides Windows API, Miles Sound System, DirectXMath, and XUI stubs for macOS

**Precompiled Header Chain:**
- `Minecraft.Client/stdafx.h` → selects platform types + stubs → includes STL → includes `extraX64.h`
- `Minecraft.World/stdafx.h` → same platform selection → includes 4J libs → includes `Consoles_App.h` → includes `GameNetworkManager.h`
- The World stdafx.h includes Client headers (circular dependency managed by include order)

**Logging/Debug:**
- `CMinecraftApp::DebugPrintf()` with per-developer filtering (`USER_JV`, `USER_PB`, `USER_SR`, etc.)
- `OutputDebugString` on Windows, `fprintf(stderr, ...)` on macOS
- Debug overlay system: `Minecraft.Client/Common/XUI/XUI_DebugOverlay.cpp`
- FPS counter: `Minecraft::fpsString`, `renderFpsMeter()`

**Validation:**
- Entity count limits enforced in `ServerLevel` (e.g., `MAX_ITEM_ENTITIES = 200`, `MAX_PRIMED_TNT = 20`)
- Level size bounds: `MAX_LEVEL_SIZE = 30000000`, `maxBuildHeight = 256`
- Small entity ID allocation with collision detection (`Entity::getSmallId()`)

**Splitscreen:**
- Up to 4 players via `XUSER_MAX_COUNT` constant
- Per-player arrays throughout: `localplayers[XUSER_MAX_COUNT]`, `localgameModes[]`, `localitemInHandRenderers[]`
- `iPad` parameter (0-3) identifies controller/player slot across entire codebase
- Viewport assignment: `Minecraft::updatePlayerViewportAssignments()`
- Per-player render state: `LevelRenderer::chunks[4]`, `LevelRenderer::level[4]`

---

*Architecture analysis: 2026-03-08*
