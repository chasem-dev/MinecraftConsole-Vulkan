# Technology Stack

**Analysis Date:** 2026-03-08

## Languages

**Primary:**
- C++ (C++17) - All game logic, rendering, networking, UI; ~660 `.cpp` + ~1050 `.h` files in `Minecraft.Client/`, ~715 `.cpp` + ~850 `.h` files in `Minecraft.World/`
- C (C99/C11) - Bundled zlib and miniupnpc libraries

**Secondary:**
- GLSL (Vulkan SPIR-V) - GPU shaders for the macOS/Vulkan bootstrap renderer (`Minecraft.Client/MacOSX/Shaders/`)
- CMake - macOS build system (`CMakeLists.txt`, `Minecraft.Client/MacOSX/CMakeLists.txt`)

## Runtime

**Environment:**
- Native C++ (no managed runtime)
- Targets: Xbox 360, Xbox One (Durango), PS3, PS4 (Orbis), PS Vita, Windows 64-bit, macOS (community port)
- macOS port uses MoltenVK (Vulkan-to-Metal translation layer)

**Package Manager:**
- None (all dependencies are vendored/bundled in-tree or resolved via system SDKs)
- CMake `FetchContent` for GLFW on macOS when not found via `find_package`

## Frameworks

**Core:**
- **4J Studios Framework** - Proprietary engine abstraction layer providing `C4JRender`, `C4JInput`, `C4JProfile`, `C4JStorage`, `C4JThread`; per-platform implementations in `{Platform}/4JLibs/inc/`
- **QNet** - Networking framework (header stub at `Minecraft.World/x64headers/qnet.h`); drives `CGameNetworkManager` and all multiplayer

**Rendering:**
- **Vulkan/MoltenVK** - macOS bootstrap renderer; `Minecraft.Client/Vulkan/VulkanBootstrapApp.h`, `VulkanBootstrapAppRender.cpp`
- **GLFW 3.4** - Window management / input on macOS; fetched via CMake if not system-available
- **C4JRender** - Engine rendering abstraction; Vulkan implementation at `Minecraft.Client/Vulkan/4JLibs/inc/4J_Render.h`, stubs all D3D11/OpenGL calls
- **Original platforms**: Direct3D 11 (Xbox One/Windows), GCM (PS3), GNM (PS4), libGXM (PS Vita)

**UI:**
- **RAD Iggy** (v1.2.30) - Flash-based UI rendering; headers at `Minecraft.Client/Windows64/Iggy/include/iggy.h`; fully stubbed on macOS via `Apple/AppleLinkerStubs.cpp`
- **XUI (Xbox UI)** - Xbox 360 UI framework; `Common/XUI/` directory (~80 files); excluded from macOS build
- **Common/UI/** - Platform-independent UI layer with `UIScene`, `UIControl`, `UIController` classes

**Audio:**
- **Miles Sound System (MSS)** - RAD proprietary audio; fully stubbed on macOS in `Apple/AppleStubs.h` (lines 920-1172); uses `HDIGDRIVER`, `HSAMPLE`, `HSTREAM` handles
- **XACT3** - Xbox 360 audio

**Testing:**
- Not detected; no test framework, test runner, or test files present

**Build/Dev:**
- **Visual Studio 2012** (Express for Windows Desktop) - Solution file `MinecraftConsoles.sln`; `.vcxproj` files for both projects
- **CMake 3.24+** - macOS build; `CMakeLists.txt` at root and `Minecraft.Client/MacOSX/CMakeLists.txt`
- **glslangValidator** - SPIR-V shader compilation for Vulkan

## Key Dependencies

**Critical (Bundled In-Tree):**
- **zlib** (full source) - Data compression; `Minecraft.Client/Common/zlib/` (26 files: `deflate.c`, `inflate.c`, `crc32.c`, etc.)
- **miniupnpc** (full source) - UPnP NAT traversal for multiplayer; `Minecraft.Client/miniupnpc/`
- **Boost 1.53.0** (partial, PS3 only) - Spirit/Wave libraries; `Minecraft.Client/PS3/PS3Extras/boost_1_53_0/`

**Infrastructure (Proprietary, Headers Only):**
- **4J Studios SDK** - `4J_Render.h`, `4J_Input.h`, `4J_Profile.h`, `4J_Storage.h` across all platform directories
- **RAD Iggy** - `iggy.h`, `gdraw.h`, `rrCore.h`; link-time stubs on macOS
- **Miles Sound System** - Types and API fully stubbed in `Apple/AppleStubs.h`
- **QNet** - Networking; stub header at `Minecraft.World/x64headers/qnet.h`

**System (macOS build):**
- Vulkan SDK (with MoltenVK)
- GLFW 3.x
- Apple frameworks: Cocoa, IOKit, CoreVideo, QuartzCore, Metal
- POSIX: pthreads, BSD sockets

## Configuration

**Build Configurations (from `.sln`):**
- Debug, Release, ContentPackage, ContentPackage_NO_TU, CONTENTPACKAGE_SYMBOLS
- Per-platform: Xbox 360, Durango (Xbox One), PS3, ORBIS (PS4), PSVita, Windows64

**Platform Detection (preprocessor defines):**
- `_XBOX` - Xbox 360
- `_DURANGO` / `_XBOX_ONE` - Xbox One
- `__PS3__` - PlayStation 3
- `__ORBIS__` - PlayStation 4
- `__PSVITA__` - PlayStation Vita
- `_WINDOWS64` - Windows 64-bit
- `__APPLE__` - macOS (community port)
- `_FINAL_BUILD` - Production build (disables printf/OutputDebugString)

**Precompiled Headers:**
- `Minecraft.Client/stdafx.h` - Client PCH; includes platform stubs, STL, 4J framework, shared engine headers
- `Minecraft.World/stdafx.h` - World PCH; includes platform stubs, STL, 4J framework, game data headers
- On macOS: `stdafx.h` → `Apple/AppleStubs.h` → `Orbis/OrbisExtras/OrbisTypes.h` (type definitions chain)

**Build (macOS):**
- `CMakeLists.txt` - Root; delegates to `Minecraft.Client/MacOSX/CMakeLists.txt`
- `Minecraft.Client/MacOSX/CMakeLists.txt` - Collects sources from `.vcxproj`, excludes platform-specific dirs, links Vulkan + GLFW + Apple frameworks
- C++17 standard (`cxx_std_17`)
- Extensive warning suppression (`-Wno-*` flags) for legacy code compatibility
- No error limit (`-ferror-limit=0`)

## Platform Requirements

**Development (macOS port):**
- macOS with Vulkan SDK (MoltenVK) installed
- CMake 3.24+
- GLFW 3.x (auto-fetched if missing)
- glslangValidator for shader compilation
- Build command: `cmake -S . -B build-macos && cmake --build build-macos --target mce_vulkan_boot`
- Executable: `build-macos/Minecraft.Client/MacOSX/mce_vulkan_boot.app/Contents/MacOS/mce_vulkan_boot`

**Development (Original Windows):**
- Visual Studio 2012+ (Express for Windows Desktop)
- DirectX 11 SDK
- XInput
- Platform SDKs for console targets (Xbox XDK, PlayStation SDKs - proprietary)

**Production:**
- No deployment target; community project in early development
- macOS build compiles and links but all game logic runs through stubs (audio silent, UI non-functional, networking placeholder)

---

*Stack analysis: 2026-03-08*
