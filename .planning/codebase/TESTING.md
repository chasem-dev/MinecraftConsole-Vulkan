# Testing Patterns

**Analysis Date:** 2026-03-08

## Test Framework

**Runner:**
- No test framework is configured or in use.
- No unit test runner, no integration test runner, no CI test pipeline.

**Assertion Library:**
- `assert()` from `<assert.h>` is used for runtime invariant checks within production code, but not as part of a test suite.
- On PS3, a custom assert is defined in `PS3Types.h` because the standard library assert aborts.

**Run Commands:**
```bash
# No test commands exist. The project has no test targets.
# Build verification is the only form of testing:
cmake -S . -B build-macos && cmake --build build-macos --target mce_vulkan_boot
```

## Test File Organization

**Location:**
- No test files exist in the project source tree.
- The only test-related files found are:
  - `Minecraft.Client/miniupnpc/java/JavaBridgeTest.java` -- a third-party library test, not project code
  - `Minecraft.Client/PS3/PS3Extras/boost_1_53_0/boost/test/` -- Boost.Test headers bundled with PS3 SDK, not used by the project

**Naming:**
- Not applicable. No test naming convention exists.

**Structure:**
- Not applicable. No test directory structure exists.

## Test Structure

**Suite Organization:**
- Not applicable. No test suites exist.

**Patterns:**
- The project relies on manual testing and runtime assertions rather than automated tests.
- `assert()` calls are scattered throughout production code for invariant checking.
- `__debugbreak()` is used for critical assertion failures (e.g., `Minecraft.World/Entity.cpp` line 95).
- Final builds disable all debug output by redefining `printf`/`wprintf` to `BREAKTHECOMPILE`.

## Mocking

**Framework:** None

**Patterns:**
- No mocking framework is in use.
- Platform abstraction is achieved through stub files rather than mock objects:
  - `Minecraft.Client/stubs.h` -- Stubs OpenGL, Keyboard, Mouse, Display, ZipFile, ImageIO, and other Java API wrappers with no-op implementations
  - `Minecraft.Client/Apple/AppleStubs.h` -- Stubs Windows API types, HRESULT, CRITICAL_SECTION, TLS, threading, DirectXMath, Miles Sound System for macOS
  - `Minecraft.Client/Apple/AppleLinkerStubs.cpp` -- Stubs external symbols (C_4JProfile, C4JStorage, Iggy UI, WinsockNetLayer, Item/Tile IDs, ShutdownManager) to satisfy the linker on macOS
  - `Minecraft.Client/Apple/AppleInputStubs.cpp` -- Stubs input system functions for macOS

**Stub Pattern Example (from `Minecraft.Client/stubs.h`):**
```cpp
class Keyboard
{
public:
    static void create() {}
    static void destroy() {}
    static bool isKeyDown(int) { return false; }
    static wstring getKeyName(int) { return L"KEYNAME"; }
    static void enableRepeatEvents(bool) {}
    // ...
};

class Mouse
{
public:
    static void create() {}
    static void destroy() {}
    static int getX() { return 0; }
    static int getY() { return 0; }
    static bool isButtonDown(int) { return false; }
};
```

**What to Mock (if tests were added):**
- Platform-specific subsystems (rendering, audio, networking, storage)
- The `CConsoleMinecraftApp` global (`app`) and its `DebugPrintf()`
- File I/O (`C4JStorage`, `ConsoleSaveFile`)
- Networking (`Connection`, `ClientConnection`, `GameNetworkManager`)

**What NOT to Mock:**
- Core game logic classes (`Entity`, `Level`, `LevelChunk`, `Player`) -- these should be tested with real instances
- Math utilities (`Mth`, `Random`) -- deterministic and side-effect free

## Fixtures and Factories

**Test Data:**
- Not applicable. No test fixtures or factories exist.

**Location:**
- Not applicable.

## Coverage

**Requirements:** None enforced. No coverage tooling is configured.

**View Coverage:**
```bash
# No coverage tooling exists.
```

## Test Types

**Unit Tests:**
- Do not exist. No unit test infrastructure.

**Integration Tests:**
- Do not exist. No integration test infrastructure.

**E2E Tests:**
- Do not exist. Testing is performed manually by running the game.

## Runtime Assertions as Pseudo-Tests

The codebase uses several assertion-like mechanisms that serve as runtime verification:

**`assert()` usage:**
- Used in `Minecraft.World/ArrayWithLength.h` for bounds checking on resize
- Used throughout game logic for precondition validation
- Disabled in release/final builds

**`__debugbreak()` usage:**
- Used in `Minecraft.World/Entity.cpp` when entity ID pool is exhausted
- Triggers a debugger breakpoint in development builds

**`BREAKTHECOMPILE` macro:**
- Defined in both `Minecraft.Client/stdafx.h` and `Minecraft.World/stdafx.h` under `_FINAL_BUILD`
- Causes compile errors if any debug output accidentally remains in final builds:
```cpp
#ifdef _FINAL_BUILD
#define printf BREAKTHECOMPILE
#define wprintf BREAKTHECOMPILE
#undef OutputDebugString
#define OutputDebugString BREAKTHECOMPILE
#endif
```

## Build Verification

The closest thing to a test suite is build verification across platforms:

**Original platforms (MSVC):**
- Visual Studio 2012 solution: `MinecraftConsoles.sln`
- Project files: `Minecraft.Client.vcxproj`, `Minecraft.World/Minecraft.World.vcxproj`
- Build configurations: Debug, Release, ReleaseForArt, ContentPackage
- Target platforms: Xbox 360, PS3, Durango (Xbox One), ORBIS (PS4), PSVita, x64 (Windows)

**macOS port (CMake):**
- `CMakeLists.txt` at project root
- `Minecraft.Client/MacOSX/CMakeLists.txt` for the macOS bootstrap target
- Build command: `cmake -S . -B build-macos && cmake --build build-macos --target mce_vulkan_boot`
- Compiles and links with 0 errors (as of 2026-03-08)

## CI/CD

**Pipeline:**
- No CI/CD pipeline is configured for automated testing.
- `.github/ISSUE_TEMPLATE/` contains bug report, crash report, and feature request templates.
- `.github/PULL_REQUEST_TEMPLATE.md` exists for PR guidelines.
- No `.github/workflows/` directory exists in the main project (only in the fetched GLFW dependency).

## Recommendations for Adding Tests

If tests were to be introduced, the following approach would fit the codebase:

**Framework choice:**
- Google Test (gtest) or Catch2 would integrate well with the CMake build system.
- Add a `tests/` directory at project root.

**Priority test targets:**
- `Minecraft.World/Mth.h` -- Pure math functions, easy to unit test
- `Minecraft.World/StringHelpers.h` -- String utility functions
- `Minecraft.World/ArrayWithLength.h` -- Container template
- `Minecraft.World/Random.h` -- RNG (verify determinism with seed)
- `Minecraft.World/compression.h` -- Compression/decompression roundtrip
- `Minecraft.World/TilePos.h`, `Minecraft.World/ChunkPos.h` -- Coordinate helpers

**Test CMake target:**
```cmake
# Example addition to CMakeLists.txt
if(MCE_BUILD_TESTS)
  enable_testing()
  add_subdirectory(tests)
endif()
```

---

*Testing analysis: 2026-03-08*
