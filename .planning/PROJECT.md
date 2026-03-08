# MCE Vulkan Port — Rendering Proof

## What This Is

A cross-platform port of Minecraft Legacy Console Edition (originally Windows/Xbox/PlayStation) to macOS (primary) and Linux via a Vulkan rendering backend. The game currently boots to the main menu with colored rectangles visible but no textures, running through the existing C4JRender abstraction layer with a new Vulkan/MoltenVK backend.

## Core Value

The world renders correctly with textures on macOS — terrain, entities, and UI all visually match the original game through the Vulkan backend.

## Requirements

### Validated

- ✓ CMake build system compiles and links on macOS — existing
- ✓ Vulkan/MoltenVK swapchain and basic pipeline functional — existing
- ✓ Game boots to main menu with geometry rendering (colored quads) — existing
- ✓ C4JRender abstraction layer serves as rendering backend interface — existing
- ✓ Platform stubs (Iggy UI, Miles Sound, Windows API) allow compilation — existing

### Active

- [ ] Textures load from game archives and bind correctly in Vulkan
- [ ] Main menu renders with correct textures and interactive UI elements
- [ ] World loads and terrain renders with block textures
- [ ] Mob/entity models render with their textures
- [ ] UI hover/interaction state propagates to visual feedback

### Out of Scope

- Multiplayer/networking — rendering proof only, no network play
- Audio implementation — Miles Sound System remains stubbed
- RAD Iggy UI replacement — UI architecture decision deferred; current stubs sufficient for rendering proof
- Linux support — macOS first; Linux follows naturally from Vulkan but not a current target
- Gameplay features beyond rendering — input handling for gameplay, saving, etc. deferred

## Context

- **Origin:** Minecraft Legacy Console Edition codebase targeting Xbox 360, PS3, PS Vita, Xbox One, PS4, and Windows
- **Rendering architecture:** C4JRender is the existing abstraction layer; Vulkan is being implemented as a new backend behind it
- **UI system:** Original uses RAD Iggy (Flash-based), fully stubbed on macOS. The non-Iggy screen/menu system (Screen.cpp hierarchy) is the current rendering path
- **Current state:** Process boots, Vulkan swapchain works, main menu geometry draws as colored rectangles. Textures don't load/bind. UI hover states don't work
- **Texture pipeline:** Textures come from game archives (ArchiveFile), go through TextureManager/PreStitchedTextureMap/StitchedTexture, and need to reach the Vulkan backend
- **Build:** CMake in `Minecraft.Client/MacOSX/CMakeLists.txt`, platform exclusions for console-specific code, stubs in `Apple/AppleStubs.h` and `Apple/AppleLinkerStubs.cpp`

## Constraints

- **Platform:** macOS with MoltenVK (Vulkan-over-Metal), must work within MoltenVK's subset of Vulkan
- **Architecture:** Must work behind C4JRender abstraction — no bypassing the existing rendering interface
- **Assets:** Original game assets/archives must be loadable; no asset conversion pipeline
- **Stubs:** Iggy UI and Miles Sound remain stubbed; don't invest in replacing them for this milestone

## Key Decisions

| Decision | Rationale | Outcome |
|----------|-----------|---------|
| Vulkan behind C4JRender | Preserves existing rendering abstraction, minimizes game code changes | — Pending |
| macOS first, Linux later | Simplifies testing; Vulkan portability makes Linux follow-on straightforward | — Pending |
| Rendering proof before gameplay | Validates the hardest part (graphics pipeline) before investing in gameplay systems | — Pending |
| Iggy UI deferred | Not needed for rendering proof; decision on replacement can wait | — Pending |

---
*Last updated: 2026-03-08 after initialization*
