# External Integrations

**Analysis Date:** 2026-03-08

## APIs & External Services

**Platform Network Services:**
- **Xbox Live (Xbox 360)** - Multiplayer, friends, invites, sessions
  - SDK: XDK (`<xonline.h>`, `<xparty.h>`)
  - Implementation: `Minecraft.Client/Xbox/Network/PlatformNetworkManagerXbox.cpp`
  - Player identity: `XUID` typedef
  - Session discovery: `XSESSION_SEARCHRESULT_HEADER`, QoS via `XNQOS`

- **Xbox Live (Xbox One / Durango)** - Multiplayer, friends, commerce
  - SDK: XDK for Xbox One (`<xdk.h>`, WRL, `ppltasks.h`)
  - Implementation: `Minecraft.Client/Durango/Network/PlatformNetworkManagerDurango.cpp`, `DQRNetworkManager.cpp`
  - Additional files: `DQRNetworkManager_FriendSessions.cpp`, `DQRNetworkManager_SendReceive.cpp`, `DQRNetworkManager_XRNSEvent.cpp`

- **PlayStation Network (PS3/PS4/Vita)** - Multiplayer, commerce, trophies, cloud saves
  - SDK: SCE SDK (`<scebase.h>`, `<kernel.h>`, `<fios2.h>`)
  - Implementation: `Minecraft.Client/Common/Network/Sony/PlatformNetworkManagerSony.cpp`
  - Per-platform: `PS3/Network/SQRNetworkManager_PS3.cpp`, `Orbis/Network/SQRNetworkManager_Orbis.cpp`, `PSVita/Network/SQRNetworkManager_Vita.cpp`
  - PS Plus upsell: `Orbis/Network/PsPlusUpsellWrapper_Orbis.h`
  - Ad-hoc networking (Vita): `PSVita/Network/SQRNetworkManager_AdHoc_Vita.cpp`

- **Windows/macOS Stub** - Placeholder networking
  - Implementation: `Minecraft.Client/Common/Network/PlatformNetworkManagerStub.cpp`
  - Uses QNet interface (`IQNet`, `IQNetPlayer`)

**Networking Framework:**
- **QNet** - Cross-platform networking abstraction
  - Header: `Minecraft.World/x64headers/qnet.h` (stub/empty)
  - Used by: `Minecraft.Client/Common/Network/GameNetworkManager.h` (includes `<qnet.h>`)
  - Provides: `IQNet`, `IQNetPlayer` interfaces
  - Transport: Custom socket layer over TCP in `Minecraft.World/Socket.cpp`, `Minecraft.World/Connection.cpp`

- **Winsock / BSD Sockets** - Low-level networking
  - Windows: `Minecraft.Client/Windows64/Network/WinsockNetLayer.cpp/.h`
  - macOS: BSD sockets via `Apple/AppleStubs.h` (WSAStartup/WSACleanup mapped to no-ops, `closesocket` mapped to `close`)
  - P2P: `Minecraft.Client/Windows64/Network/P2PConnectionManagerWin.cpp`

- **miniupnpc** - UPnP NAT port forwarding
  - Source: `Minecraft.Client/miniupnpc/` (full library bundled)
  - Purpose: Automatic port forwarding for peer-to-peer multiplayer hosting

## Data Storage

**Save System:**
- **Custom binary format** - Proprietary console save file format
  - Implementation: `Minecraft.World/ConsoleSaveFileOriginal.cpp`, `Minecraft.World/ConsoleSaveFileSplit.cpp/.h`
  - Header: `Minecraft.World/FileHeader.h` - 12-byte header (offset, size, version pair)
  - Version tracking: `ESaveVersions` enum (versions 1-8, from pre-launch through compressed chunk storage)
  - Platform tags: `ESavePlatform` - fourCC codes for X360, XB1, PS3, PS4, PSV, WIN
  - Endianness: Big-endian on PS3/Xbox 360, little-endian on all others
  - Storage: Virtual memory backed (`VirtualAlloc`), 64MB reserved, 64KB pages committed on demand
  - Cross-platform cloud saves: Sony cloud storage with compression type auto-detection

- **4J Storage Manager** - Platform storage abstraction
  - Interface: `4J_Storage.h` (per-platform in `{Platform}/4JLibs/inc/`)
  - Provides: `C4JStorage` class with message results, file operations
  - Global instance: `StorageManager`

**File Storage:**
- Local filesystem only (platform-specific paths managed by `C4JStorage`)
- Archive files: `Minecraft.Client/ArchiveFile.cpp/.h` - custom archive format for bundled assets

**Caching:**
- `Minecraft.World/IntCache.cpp` - Thread-local integer buffer cache for world generation
- `Minecraft.World/BiomeCache.h` - Biome data cache
- `Minecraft.Client/MultiPlayerChunkCache.cpp` - Network chunk data cache
- No external caching service

## Authentication & Identity

**Auth Providers:**
- **Xbox Live** (Xbox 360/One) - Gamertag/XUID-based identity
  - Player identity: `typedef XUID PlayerUID` (64-bit)
  - Sign-in change handling: `CGameNetworkManager::HandleSignInChange()`

- **PlayStation Network** (PS3/PS4/Vita) - PSN account-based identity
  - Player identity: Custom `PlayerUID` type per platform
  - PS Plus verification: `PsPlusUpsellWrapper_Orbis.h`
  - Sign-in: `PSNSignInReturned_0/1` callbacks in `GameNetworkManager.h`

- **Windows/macOS** - No authentication
  - Uses stub networking; `FakeLocalPlayerJoined()` for testing

**Implementation:**
- Profile management: `C4JProfile` (via `4J_Profile.h`)
  - Global instance: `ProfileManager`
  - Methods: `GetPrimaryPad()`, `SetDebugFullOverride()`

## Commerce & DLC

**Digital Commerce:**
- **Sony Commerce** (PS3/PS4)
  - Implementation: `Common/Network/Sony/SonyCommerce.cpp/.h`
  - Per-platform: `PS3/Network/SonyCommerce_PS3.cpp`, `Orbis/Network/SonyCommerce_Orbis.cpp`
  - Vita: `PSVita/Network/SonyCommerce_Vita.h`

- **DLC Manager** - Cross-platform downloadable content system
  - Implementation: `Minecraft.Client/Common/DLC/DLCManager.cpp/.h`
  - Pack system: `DLCPack.cpp/.h` - individual content packs
  - Content types: Skins (`DLCSkinFile`), Textures (`DLCTextureFile`), Audio (`DLCAudioFile`), Localization (`DLCLocalisationFile`), Game Rules (`DLCGameRulesFile`), Capes (`DLCCapeFile`), Colour Tables (`DLCColourTableFile`), UI Data (`DLCUIDataFile`)
  - Parameters: Display name, theme name, free flag, credit, cape path, box art, animation, pack ID, particle colours

## Monitoring & Observability

**Telemetry:**
- **Sentient Analytics** - Game telemetry system
  - Base: `Minecraft.Client/Common/Telemetry/TelemetryManager.cpp/.h`
  - Per-platform implementations:
    - Xbox 360: `Xbox/Sentient/SentientManager.cpp`, `Xbox/Sentient/MinecraftTelemetry.h`
    - Xbox One: `Durango/Sentient/DurangoTelemetry.cpp/.h`
    - PS3: `PS3/Sentient/MinecraftTelemetry.h`
    - PS4: `Orbis/Sentient/MinecraftTelemetry.h`
    - Windows/macOS: `Windows64/Sentient/MinecraftTelemetry.h`, `Windows64/Sentient/SentientTelemetryCommon.h`
  - Events tracked: Player session start/exit, heartbeat, level start/exit/resume, pause/unpause, menu shown, achievement unlock, media share, upsell presented/responded, player death, enemy killed, texture pack loaded, skin changed, ban/unban
  - Dynamic configuration: `DynamicConfigurations.cpp` per platform

- **Leaderboard System** - Per-platform leaderboard integration
  - Base: `Minecraft.Client/Common/Leaderboards/LeaderboardManager.cpp`
  - Xbox 360: `Xbox/Leaderboards/XboxLeaderboardManager.cpp/.h`
  - Xbox One: `Durango/Leaderboards/DurangoLeaderboardManager.cpp/.h`, `Durango/Leaderboards/GameProgress.cpp`
  - PS3: `PS3/Leaderboards/PS3LeaderboardManager.cpp/.h`
  - PS4: `Orbis/Leaderboards/OrbisLeaderboardManager.cpp/.h`
  - Vita: `PSVita/Leaderboards/PSVitaLeaderboardManager.cpp/.h`
  - Windows: `Windows64/Leaderboards/WindowsLeaderboardManager.h` (stub)

**Error Tracking:**
- None (no external error tracking service)

**Logs:**
- `OutputDebugString` / `OutputDebugStringA` / `OutputDebugStringW` - Platform debug output
  - macOS stub: writes to `stderr` via `fprintf`
  - Disabled in `_FINAL_BUILD` (mapped to `BREAKTHECOMPILE`)
- `printf` / `wprintf` - Also disabled in `_FINAL_BUILD`
- Custom debug overlay: `Common/XUI/XUI_DebugOverlay.cpp`

## Social Features

**Social Manager:**
- Per-platform social integration
  - Xbox: `Xbox/Social/SocialManager.cpp`
  - Windows/macOS: `Windows64/Social/SocialManager.cpp`
- Social posting: `xsocialpost.h` (stub at `Minecraft.World/x64headers/xsocialpost.h`); excluded on macOS
- Preview images: `XSOCIAL_PREVIEWIMAGE` struct for screenshot sharing

**Remote Storage (Sony):**
- Cloud save synchronization
  - Base: `Common/Network/Sony/SonyRemoteStorage.cpp/.h`
  - PS3: `PS3/Network/SonyRemoteStorage_PS3.h`
  - PS4: `Orbis/Network/SonyRemoteStorage_Orbis.h`
  - Vita: `PSVita/Network/SonyRemoteStorage_Vita.h`

## Audio Integration

**Miles Sound System:**
- Proprietary audio middleware by RAD Game Tools
- Used on: All platforms except Xbox 360 (which uses XACT3)
- Interface: `Minecraft.Client/Common/Audio/SoundEngine.h` - `SoundEngine` class
- Sound bank: `HMSOUNDBANK` via `AIL_add_soundbank()`
- Streaming music: `HSTREAM` via `AIL_open_stream()`
- 3D audio: `AIL_set_sample_3D_position()`, `AIL_set_listener_3D_position()`
- macOS status: All ~50 API functions stubbed as no-ops in `Apple/AppleStubs.h` (silent audio)
- Xbox 360: `Xbox/Audio/SoundEngine.cpp` (XACT3-based)
- PS3: `PS3/Audio/PS3_SoundEngine.cpp`

## Rendering Integration

**C4JRender Abstraction:**
- Central rendering API wrapping platform graphics backends
- Interface: `4J_Render.h` per platform
- Vulkan/macOS: `Minecraft.Client/Vulkan/4JLibs/inc/4J_Render.h` - GLFW-backed stub implementation
- Vulkan bootstrap: `Minecraft.Client/Vulkan/VulkanBootstrapApp.h` - Full Vulkan renderer for testing
- OpenGL-style constants remapped: `GL_BLEND`, `GL_DEPTH_TEST`, `GL_TEXTURE_2D`, etc. redefined as integer enums
- Matrix stack: Model-view, projection, texture stacks managed internally

**RAD Iggy (UI Rendering):**
- Flash/ActionScript-based UI middleware
- Version: 1.2.30 (`iggy.h`)
- All platforms use Iggy for menus except Xbox 360 (uses XUI natively)
- macOS: All Iggy symbols stubbed as link-time no-ops in `Apple/AppleLinkerStubs.cpp`

## CI/CD & Deployment

**Hosting:**
- GitHub (community repository)

**CI Pipeline:**
- None detected; no `.github/workflows/` directory
- Issue templates present: `.github/ISSUE_TEMPLATE/`
- PR template present: `.github/PULL_REQUEST_TEMPLATE.md`

## Data Compression

**Compression System:**
- `Minecraft.World/compression.h/.cpp` - Thread-safe compression with per-thread storage
- Algorithms:
  - RLE (Run-Length Encoding) - custom implementation
  - LZX+RLE - Xbox 360 (`XMEMCOMPRESSION_CONTEXT`)
  - zlib+RLE - PS4, Xbox One, Windows, Vita, macOS (`APPROPRIATE_COMPRESSION_TYPE`)
  - PS3 zlib - PS3-specific
- zlib: Full source bundled at `Minecraft.Client/Common/zlib/` (zlib.h, deflate.c, inflate.c, etc.)

## HTTP Integration

**Sony HTTP:**
- `Common/Network/Sony/SonyHttp.cpp/.h` - HTTP client for Sony platforms
- Used for: Commerce transactions, remote storage, potentially telemetry upload

## Environment Configuration

**Required env vars:**
- None detected; the game uses no environment variable configuration

**Secrets location:**
- Not applicable; all platform credentials are managed by console SDKs at the OS level
- No `.env` files present

## Webhooks & Callbacks

**Incoming:**
- None (no webhook endpoints; this is a game client, not a server application)

**Outgoing:**
- Telemetry events via platform-specific Sentient SDK
- Leaderboard score submissions via platform APIs
- Commerce transactions via Sony Commerce / Xbox Marketplace APIs

---

*Integration audit: 2026-03-08*
