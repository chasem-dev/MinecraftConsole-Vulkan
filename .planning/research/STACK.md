# Technology Stack

**Project:** MCE Vulkan Port -- Rendering Proof
**Researched:** 2026-03-08

## Recommended Stack

### Core Rendering

| Technology | Version | Purpose | Why | Confidence |
|------------|---------|---------|-----|------------|
| Vulkan API | 1.2 (target) | Graphics API | Already in use via VulkanBootstrapApp. Target 1.2 not 1.4 -- the codebase needs basic features (textures, depth buffers, blend states), and 1.2 is universally supported by both MoltenVK and KosmicKrisp. No 1.3/1.4 features are needed for this fixed-function emulation renderer. | HIGH |
| MoltenVK | 1.4.1 | Vulkan-to-Metal translation | Already a dependency. Bundled with Vulkan SDK 1.4.341.1. Supports Vulkan 1.4 (nearly conformant). Works on both Apple Silicon and Intel Macs. Mature, proven in production (DXVK/CrossOver, game ports). KosmicKrisp is an emerging alternative but Apple-Silicon-only and not yet Vulkan 1.4 conformant -- stick with MoltenVK for now. | HIGH |
| Vulkan SDK | 1.4.341.1 | SDK, validation layers, tools | Latest LunarG SDK for macOS (Feb 2026). Includes MoltenVK, validation layers, glslangValidator. Already required by the build system. | HIGH |

### Window/Input

| Technology | Version | Purpose | Why | Confidence |
|------------|---------|---------|-----|------------|
| GLFW | 3.4 | Window management, input, Vulkan surface | Already in use. Latest stable release (Feb 2025). Native Vulkan surface creation via `glfwCreateWindowSurface`. Auto-fetched by CMake if not installed. No reason to change. | HIGH |

### Memory Management

| Technology | Version | Purpose | Why | Confidence |
|------------|---------|---------|-----|------------|
| VMA (Vulkan Memory Allocator) | 3.3.0 | GPU memory allocation | The current implementation manually calls `vkAllocateMemory` per-resource -- this does not scale for textures, vertex buffers, and per-chunk geometry. VMA is the industry standard single-header library (MIT license) that handles memory pooling, sub-allocation, defragmentation, and alignment. Works with MoltenVK on macOS. Used by virtually every non-trivial Vulkan renderer (DXVK, VulkanMod, RBDOOM-3-BFG). No alternative comes close. | HIGH |

### Texture Loading

| Technology | Version | Purpose | Why | Confidence |
|------------|---------|---------|-----|------------|
| stb_image | 2.30 | PNG/JPG image decoding | Single-header, public domain, zero dependencies. The engine's `LoadTextureData` needs to decode PNG/JPG from memory buffers (extracted from game archives via `ArchiveFile`). stb_image's `stbi_load_from_memory()` handles exactly this case. The game does not need KTX2 or GPU-compressed formats -- original textures are simple RGBA bitmaps. Already the de facto standard for this use case in Vulkan tutorials and engines. | HIGH |
| stb_image_write | 1.16 | PNG/JPG encoding (screenshots) | Needed for `CaptureThumbnail`/`CaptureScreen` in C4JRender. Same single-header pattern as stb_image. | MEDIUM |

### Math

| Technology | Version | Purpose | Why | Confidence |
|------------|---------|---------|-----|------------|
| Hand-rolled matrix math | N/A | CPU-side transforms | The codebase already has working matrix math in `VulkanRenderManager.cpp` (perspective, orthographic, rotation, scale, translation, multiply). This code is compact (~150 lines), correct, and matches the engine's column-major OpenGL-style conventions. Adding GLM would require adapting to its conventions and creates a mismatch with the existing `C4JRender::MatrixGet()` interface which returns raw `float*` arrays. **Keep the existing math code.** | HIGH |
| GLM | 1.0.3 | Math library (NOT recommended) | While GLM is the standard Vulkan math library, introducing it here creates friction: the C4JRender interface traffics in raw `float[16]` arrays, and GLM's `glm::mat4` type would require constant casting. The existing hand-rolled math already works and matches the engine's assumptions. GLM would be appropriate for a new engine; it is overhead for wrapping an existing one. | HIGH |

### Shader Compilation

| Technology | Version | Purpose | Why | Confidence |
|------------|---------|---------|-----|------------|
| glslangValidator | (Vulkan SDK) | GLSL to SPIR-V offline compilation | Already in use via CMake custom commands. Shaders are pre-compiled to `.spv` at build time. This is the right approach -- no runtime compilation needed. The game has a fixed set of shader permutations (textured, lit, fog, alpha-test), not user-authored shaders. | HIGH |

### Build System

| Technology | Version | Purpose | Why | Confidence |
|------------|---------|---------|-----|------------|
| CMake | 3.24+ | Build system | Already in use. The `FetchContent` for GLFW, `find_package(Vulkan)`, custom shader compilation commands, and object library pattern (`mce_client_legacy`, `mce_world_legacy`) are all working. Extend this for new dependencies (VMA, stb) rather than changing build systems. | HIGH |

### Debugging/Validation

| Technology | Version | Purpose | Why | Confidence |
|------------|---------|---------|-----|------------|
| Vulkan Validation Layers | (Vulkan SDK) | Runtime API usage validation | Bundled with the Vulkan SDK. Essential during development -- catches incorrect resource states, missing barriers, invalid formats. Enable via `VK_INSTANCE_CREATE_INFO` with `VK_LAYER_KHRONOS_validation`. Zero cost when disabled in release builds. | HIGH |
| RenderDoc | 1.35+ | GPU frame capture and debugging | Free, open-source graphics debugger. Works with MoltenVK on macOS (via Metal capture underneath). Invaluable for debugging texture binding, shader input mismatches, and draw call inspection. Install separately, not a build dependency. | MEDIUM |

## Dependencies NOT Recommended

| Technology | Why Not |
|------------|---------|
| GLM 1.0.3 | Existing hand-rolled math works and matches engine conventions. Adding GLM creates type-conversion overhead at every C4JRender boundary. See Math section above. |
| KosmicKrisp | New LunarG Vulkan-to-Metal driver (Mesa-based). Fully Vulkan 1.3 conformant but Apple Silicon only -- does not support Intel Macs. Still in beta (first SDK inclusion: 1.4.341). MoltenVK is more mature and broader. Worth revisiting in 6-12 months. |
| Dear ImGui | Useful for debug UI overlays, but the engine already has its own UI system (Common/UI + Screen hierarchy). Adding ImGui creates a parallel rendering path and Vulkan pipeline complexity. Not needed for rendering proof. |
| SDL2/SDL3 | Alternative to GLFW. GLFW is already integrated and working. SDL would add window management, input, and audio in one package, but audio is out of scope and the extra surface area is not justified when GLFW is functional. |
| Assimp | 3D model loading library. Not needed -- the engine has its own model format (HumanoidModel, ChickenModel, etc. with inline vertex data). No external model files to parse. |
| libpng/libjpeg | Dedicated image decoders. stb_image covers both formats in a single header with simpler integration. No advantage over stb_image for this use case. |
| volk | Vulkan function pointer loader. Useful for projects that need to load Vulkan dynamically at runtime. This project links directly against `Vulkan::Vulkan` via CMake -- volk adds complexity without benefit. |
| shaderc | Google's shader compilation library (wraps glslang). Provides runtime GLSL-to-SPIR-V compilation. Not needed -- offline compilation with glslangValidator is simpler and the shader set is fixed. |

## Vulkan Feature Requirements

The C4JRender abstraction maps to these specific Vulkan features. All are available in Vulkan 1.2 and supported by MoltenVK:

| Engine Feature | Vulkan Requirement | Notes |
|----------------|-------------------|-------|
| Matrix stacks (ModelView, Projection) | Push constants or UBO for MVP | Push constants for per-draw MVP (128 bytes minimum guaranteed, MVP = 64 bytes). Use UBO for shared view/projection if push constant budget is tight. |
| `DrawVertices` (immediate mode) | Dynamic vertex buffer uploads | VMA staging buffer with persistent mapping. Batch per-frame into single large buffer. |
| `TextureCreate/Bind/Data` | `VkImage` + `VkImageView` + `VkSampler`, descriptor sets | One descriptor set per texture bind point. Use `VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER`. |
| `StateSetBlendEnable/Func` | Pipeline variants or dynamic state | Use `VK_EXT_extended_dynamic_state` (core in 1.3, extension in 1.2 on MoltenVK) or pre-bake a small set of pipeline variants (opaque, alpha-blend, additive). |
| `StateSetDepthTestEnable/Func/Mask` | Pipeline variants or dynamic state | Same as blend -- either dynamic state or pipeline cache with hash of (blend, depth, cull) state. |
| `StateSetFogEnable/Colour/Distance` | Uniform buffer + fragment shader | Fog computed in fragment shader using uniforms. No hardware fog in Vulkan. |
| `StateSetLighting/LightColour/Direction` | Uniform buffer + vertex/fragment shader | Fixed-function lighting emulated in shaders. Max 2 lights (GL_LIGHT0, GL_LIGHT1) based on C4JRender constants. |
| `StateSetAlphaTestEnable/Func` | Fragment shader discard | `if (alpha < threshold) discard;` controlled by push constant or UBO flag. |
| `CBuffCreate/Start/End/Call` (command buffers) | Secondary command buffers or draw call recording | C4JRender command buffers are display-list-style batches. Can be implemented as recorded vertex data + state snapshots replayed at CBuffCall time. |
| Depth buffer | `VkImage` with depth format | Use `VK_FORMAT_D32_SFLOAT` on MoltenVK (most broadly supported). Query format support at init. |
| Splitscreen viewports | `vkCmdSetViewport` / `vkCmdSetScissor` | Dynamic viewport/scissor (always dynamic in modern Vulkan usage). |

## Shader Architecture

The engine needs a small set of GLSL shader permutations, not a single uber-shader:

| Shader | Vertex Inputs | Uniforms | Fragment Logic |
|--------|--------------|----------|----------------|
| **Colored** (current) | pos3 + color4 | MVP (push constant) | Pass-through color |
| **Textured** | pos3 + uv2 + color4 + normal4 | MVP, texture sampler | `texture(sampler, uv) * color` |
| **Textured+Fog** | Same as textured | MVP, fog params | Textured + linear/exp fog blend |
| **Textured+Lit** | Same as textured | MVP, light dir/color/ambient | Textured + N.L directional lighting |
| **Textured+Lit+Fog** | Same as textured | MVP + fog + light params | Full fixed-function emulation |

Use `#define` preprocessor flags in GLSL and compile permutations offline with glslangValidator. This matches how the original D3D11 backend used pixel shader types (`ePixelShaderType`).

## Integration Pattern

New dependencies should be vendored as single files or fetched via CMake:

```cmake
# VMA -- single header, add to Vulkan/ directory
# Download vk_mem_alloc.h from GitHub, include in one .cpp with:
#   #define VMA_IMPLEMENTATION
#   #include "vk_mem_alloc.h"

# stb_image -- single header, add to Vulkan/ or a shared vendor/ directory
# Download stb_image.h from GitHub, include in one .cpp with:
#   #define STB_IMAGE_IMPLEMENTATION
#   #include "stb_image.h"
```

No CMake `FetchContent` needed for these -- they are single-header files that should be committed to the repository for build reproducibility.

## Installation / Setup

```bash
# Prerequisites (macOS)
# 1. Install Vulkan SDK from https://vulkan.lunarg.com/sdk/home
# 2. GLFW auto-fetched by CMake if not installed via Homebrew

# Build (existing, no changes needed for stack additions)
cmake -S . -B build-macos
cmake --build build-macos --target mce_vulkan_boot

# New dependencies (manual download, one-time)
curl -o Minecraft.Client/Vulkan/vendor/vk_mem_alloc.h \
  https://raw.githubusercontent.com/GPUOpen-LibrariesAndSDKs/VulkanMemoryAllocator/master/include/vk_mem_alloc.h

curl -o Minecraft.Client/Vulkan/vendor/stb_image.h \
  https://raw.githubusercontent.com/nothings/stb/master/stb_image.h

curl -o Minecraft.Client/Vulkan/vendor/stb_image_write.h \
  https://raw.githubusercontent.com/nothings/stb/master/stb_image_write.h
```

## Sources

- [MoltenVK GitHub](https://github.com/KhronosGroup/MoltenVK) - v1.4.1, Vulkan 1.4 support (HIGH confidence)
- [MoltenVK Releases](https://github.com/KhronosGroup/MoltenVK/releases) - v1.4.1 released Nov 30, 2024 (HIGH confidence)
- [VMA GitHub](https://github.com/GPUOpen-LibrariesAndSDKs/VulkanMemoryAllocator) - v3.3.0 (HIGH confidence)
- [stb GitHub](https://github.com/nothings/stb) - stb_image v2.30 (HIGH confidence)
- [GLFW GitHub](https://github.com/glfw/glfw) - v3.4 released Feb 23, 2025 (HIGH confidence)
- [GLM GitHub](https://github.com/g-truc/glm) - v1.0.3 released Dec 31, 2025 (HIGH confidence, but NOT recommended for this project)
- [LunarG Vulkan SDK](https://vulkan.lunarg.com/sdk/home) - v1.4.341.1, Feb 2026 (HIGH confidence)
- [LunarG: State of Vulkan on Apple Jan 2026](https://www.lunarg.com/the-state-of-vulkan-on-apple-jan-2026/) - KosmicKrisp comparison (HIGH confidence)
- [Vulkan Push Constants docs](https://docs.vulkan.org/guide/latest/push_constants.html) - 128-byte minimum (HIGH confidence)
- [VulkanMod GitHub](https://github.com/xCollateral/VulkanMod) - Minecraft Vulkan renderer architecture reference (MEDIUM confidence)
- [Vulkan depth buffering tutorial](https://vulkan-tutorial.com/Depth_buffering) - Format selection guidance (HIGH confidence)
- [MoltenVK depth format issue #888](https://github.com/KhronosGroup/MoltenVK/issues/888) - D32_SFLOAT preferred on MoltenVK (HIGH confidence)
- [VK_KHR_dynamic_rendering docs](https://docs.vulkan.org/samples/latest/samples/extensions/dynamic_rendering/README.html) - Supported in MoltenVK 1.4 (HIGH confidence)

---

*Stack research: 2026-03-08*
