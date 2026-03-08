# Project Research Summary

**Project:** MCE Vulkan Port -- Rendering Proof
**Domain:** Vulkan rendering backend for C++ game engine (Minecraft Legacy Console Edition macOS port)
**Researched:** 2026-03-08
**Confidence:** HIGH

## Executive Summary

This project is porting a fixed-function OpenGL/D3D11 rendering engine to Vulkan on macOS via MoltenVK. The codebase is a decompiled version of Minecraft Legacy Console Edition (4J Studios) with a three-layer rendering architecture: game code speaks OpenGL-style calls through `glWrapper.cpp`, which delegates to a `C4JRender` abstraction layer, which must delegate to a Vulkan backend. The bootstrap renderer already compiles, links, and opens a window -- it renders colored geometry but has no texture support, no depth buffer, no render state management, and no command buffer (display list) system. The stack is well-established and fully decided: Vulkan 1.2 via MoltenVK, GLFW for windowing, VMA for memory allocation, stb_image for texture decoding, and hand-rolled matrix math that already matches the engine's conventions.

The recommended approach is a six-phase build-up from the existing bootstrap: stabilize the renderer foundation, build shader and depth infrastructure, implement the texture pipeline, add render state tracking with pipeline caching, implement the CBuff display-list system for chunk rendering, and finally scale vertex buffers for full scene loads. This ordering follows strict dependency chains -- textures require shaders, terrain requires command buffers, and everything requires depth testing. The architecture research confirms that only 5-6 new internal components need to be created within the existing two-class boundary (C4JRender + VulkanBootstrapApp), and the game exercises only 15-25 unique render state combinations, making pipeline caching tractable.

The top risks are: pipeline state explosion from the immediate-mode state change pattern (mitigate with Vulkan 1.3 dynamic states), CPU-side vertex transformation bottleneck (mitigate by moving MVP to push constants), silent texture data discard from all-stubbed texture methods (mitigate by implementing texture methods as a cohesive unit), and NDC coordinate differences between OpenGL and Vulkan (mitigate by choosing one Y-flip/depth-range approach and enforcing it everywhere). All of these are well-understood Vulkan porting challenges with documented solutions -- none are novel risks.

## Key Findings

### Recommended Stack

The stack is minimal and high-confidence. Every technology is either already integrated or is a single-header library that vendors directly into the repository. No package manager dependencies, no framework debates.

**Core technologies:**
- **Vulkan 1.2 + MoltenVK 1.4.1**: Graphics API and Metal translation layer -- already in use, mature, supports all needed features. Target 1.2 not 1.4 since only basic features are needed.
- **GLFW 3.4**: Window management and Vulkan surface creation -- already integrated via CMake FetchContent.
- **VMA 3.3.0**: GPU memory allocation -- replaces the current per-resource `vkAllocateMemory` calls that will not scale. Single-header, MIT license, industry standard.
- **stb_image 2.30**: PNG/JPG decoding for texture loading from game archives -- single-header, zero dependencies, public domain.
- **glslangValidator (Vulkan SDK)**: Offline GLSL-to-SPIR-V compilation -- already available, shaders are pre-compiled at build time via CMake custom commands.
- **Hand-rolled matrix math**: Keep the existing ~150 lines of column-major matrix code. Adding GLM would create type-conversion friction at every C4JRender boundary that passes raw `float[16]`.

**Explicitly not recommended:** GLM (type mismatch with engine), KosmicKrisp (Apple Silicon only, not mature enough), Dear ImGui (parallel rendering path), SDL (GLFW already works), volk (direct linking is simpler), shaderc (offline compilation is sufficient).

### Expected Features

**Must have (table stakes) -- rendering proof fails without these:**
- Texture creation, binding, and sampling in fragment shader
- Depth buffer with depth test/write/function control
- Alpha blending (water, glass, particles, GUI)
- Alpha testing via fragment shader discard (foliage, cutout blocks)
- Command buffer display lists (CBuff system for chunk terrain rendering)
- Vertex color modulation (biome tinting, darkness)
- Face culling (backface elimination)
- Fog (distance fade, three modes: linear, exp, exp2)
- Texture parameter control (NEAREST filtering for the blocky Minecraft look)

**Should have (visual completeness):**
- Directional lighting for entity models (2 lights + ambient)
- Light texture / lightmap (16x16 multi-texture for day/night cycle on terrain)
- Animated texture sub-region updates (water, lava, fire)
- Line rendering (block selection highlight)
- Depth bias (block breaking overlay z-fighting fix)

**Defer to later milestones:**
- GPU-side MVP transform optimization (CPU-side works for proof initially)
- Multiple frames in flight (single frame is acceptable for proof)
- Mipmapping, splitscreen, audio, networking, Iggy UI replacement, screen capture
- All console-specific rendering paths (PS3 SPU, Vita, Xbox 360)

### Architecture Approach

The architecture preserves the existing two-class boundary (C4JRender as the abstraction interface, VulkanBootstrapApp as the Vulkan implementation) and adds 5 internal subsystems: VulkanTextureManager (texture slot map with staging upload), VulkanPipelineCache (lazily-created pipelines keyed by render state bitmask), VulkanRenderState (tracks ~20 state flags, produces pipeline cache keys), VulkanDescriptorManager (descriptor set allocation and texture binding), and VulkanShaderManager (SPIR-V loading and push constant layouts). The game code and glWrapper.cpp require zero changes.

**Major components:**
1. **glWrapper.cpp** -- OpenGL-to-C4JRender translation (complete, no changes needed)
2. **C4JRender / VulkanRenderManager** -- Abstraction layer with matrix stacks, state tracking, texture slot mapping (partially implemented, needs texture/state methods)
3. **VulkanBootstrapApp** -- Owns all Vulkan objects, executes draws (exists, needs depth buffer, multi-pipeline, descriptors, textures)
4. **VulkanTextureManager** (new) -- VkImage/VkImageView/VkSampler per texture slot, staging buffer uploads
5. **VulkanPipelineCache** (new) -- Creates/caches VkPipeline objects by render state bitmask (~15-25 unique combinations)
6. **VulkanRenderState** (new) -- Tracks blend/depth/alpha/cull/fog/lighting state, produces pipeline key

**Key architectural patterns:**
- Render state bitmask for pipeline selection (lazy creation, permanent cache)
- Texture slot map with integer ID keys matching C4JRender's TextureCreate() counter
- Push constants for MVP matrix (128 bytes guaranteed, MVP = 64 bytes)
- Deferred draw batching sorted by (pipeline state, texture) to minimize state changes
- CBuff implemented as stored vertex arrays (NOT Vulkan secondary command buffers)
- Dynamic ring buffer for per-frame vertex data (replace 65K cap with 4-16MB)

### Critical Pitfalls

1. **Pipeline state explosion** -- The game makes ~30 StateSet* calls per frame in OpenGL immediate-mode style. Each unique combination needs a VkPipeline. Mitigate with Vulkan 1.3 dynamic states (viewport, scissor, depth, cull, blend, stencil all dynamic on MoltenVK 1.3+) plus lazy pipeline creation with permanent caching. Only ~15-25 unique combinations exist in practice.

2. **CPU-side vertex transform bottleneck** -- Current code transforms every vertex on the CPU via `convertVertex()`. The Tesselator can emit 2M+ floats per frame (16MB buffer). Move MVP to GPU push constants when building shader infrastructure.

3. **Texture pipeline is entirely stubbed** -- All 15 texture-related C4JRender methods return 0 or are empty. Must be implemented as a cohesive unit. Batch init-time uploads (120+ textures) into one command buffer submission -- synchronous per-texture upload with vkQueueWaitIdle would stall 5-15 seconds.

4. **NDC Y-axis and depth range differ from OpenGL** -- Vulkan Y points down, depth is [0,1] vs OpenGL's [-1,1]. Use negative viewport height (VK_KHR_maintenance1) for Y-flip and modify perspective matrix for [0,1] depth range. Choose ONE approach and enforce everywhere.

5. **CBuff system is NOT Vulkan command buffers** -- "Command buffers" in 4J's terminology are display lists storing vertex data, not GPU command streams. Implementing them as Vulkan secondary command buffers causes render pass compatibility failures. Implement as stored vertex arrays replayed with current pipeline state.

## Implications for Roadmap

Based on research, suggested phase structure:

### Phase 1: Bootstrap Stabilization

**Rationale:** Fix known bugs in the existing renderer foundation before building on top of it. Two bugs (swapchain fence deadlock, missing validation layers) will cause frustrating development issues if not fixed first.
**Delivers:** A stable, debuggable renderer foundation that does not hang on window resize.
**Addresses:** Swapchain fence deadlock (Pitfall 9), validation layer enablement (Pitfall 19), vertex color unpacking fix for ARGB8 ordering (Pitfall 11).
**Avoids:** Building new features on a broken foundation that deadlocks during testing.
**Complexity:** Low. Three targeted fixes.

### Phase 2: Depth Buffer and Shader Infrastructure

**Rationale:** Everything in the renderer depends on depth testing and proper shaders. The depth buffer is a prerequisite for any correct 3D rendering. The shader infrastructure (new vertex format with UVs, push constants for MVP, fragment shader with texture sampling) is required before textures can display.
**Delivers:** Correct 3D geometry rendering with GPU-side transforms, depth testing, and the shader pipeline needed for texture support.
**Uses:** glslangValidator (shader compilation), existing matrix math for push constants.
**Implements:** VulkanShaderManager, depth attachment in render pass, push constant MVP, shader variant system (color-only, textured, textured+alpha-test, textured+fog).
**Avoids:** CPU-side vertex transform bottleneck (Pitfall 2), NDC mismatch (Pitfall 4), missing depth buffer (Pitfall 10), shader compilation pipeline gap (Pitfall 15).
**Complexity:** Medium-High. Architecturally the most important phase.

### Phase 3: Texture Pipeline

**Rationale:** With shaders accepting UV coordinates and sampling textures, the texture management system can be built. This phase transforms the game from "colored shapes" to "recognizable Minecraft." Depends on Phase 2 shaders being able to sample textures.
**Delivers:** Textured rendering -- terrain atlas, mob textures, UI elements all display correctly.
**Uses:** VMA (GPU memory allocation for texture images), stb_image (PNG/JPG decoding from archive files).
**Implements:** VulkanTextureManager, VulkanDescriptorManager, TextureCreate/Data/Bind/Free/SetParam methods (~15 coordinated method implementations).
**Avoids:** Texture stubs silently discarding data (Pitfall 3), descriptor set exhaustion (Pitfall 7), RGBA/BGRA format confusion (Pitfall 13), texture ID zero collision (Pitfall 16).
**Complexity:** High.

### Phase 4: Render State and Pipeline Cache

**Rationale:** With textures visible, the next visual problems are incorrect blending (water is opaque), missing alpha test (foliage has black pixels), wrong face culling, and no fog. These all require a pipeline state tracking and caching system.
**Delivers:** Correct transparency, alpha cutout, face culling, and fog. The world looks visually correct at a static viewpoint.
**Implements:** VulkanRenderState, VulkanPipelineCache, all StateSet* methods, fog/lighting in fragment shaders.
**Avoids:** Pipeline state explosion (Pitfall 1).
**Complexity:** Medium. Well-defined problem with ~15-25 pipeline variants.

### Phase 5: Command Buffer System (Terrain Rendering)

**Rationale:** Chunk rendering depends on the CBuff display-list system. Without it, the LevelRenderer cannot pre-compile chunk geometry. This phase unlocks terrain rendering -- the single biggest visual milestone.
**Delivers:** Full terrain rendering. The Minecraft world is visible and navigable.
**Implements:** CBuffCreate/CBuffStart/CBuffEnd/CBuffCall as stored vertex arrays, chunk rebuild integration.
**Avoids:** Misimplementing CBuff as Vulkan secondary command buffers (Pitfall 6), thread safety issues with vertex submission (Pitfall 12).
**Complexity:** Medium-High.

### Phase 6: Vertex Buffer Scaling and Performance

**Rationale:** Full terrain rendering will immediately hit the 65K vertex budget. This phase replaces the fixed vertex buffer with a dynamic ring buffer and optionally adds multi-frame-in-flight support.
**Delivers:** Stable frame rates with full scene complexity. No vertex budget overflow crashes.
**Implements:** Ring buffer or per-frame vertex buffer allocation (4-16MB), optionally 2-3 frames in flight with per-frame sync objects.
**Avoids:** Vertex budget overflow (Pitfall 2), single frame-in-flight bottleneck (Pitfall 8), tile-based GPU inefficiency (Pitfall 14).
**Complexity:** Medium.

### Phase Ordering Rationale

- **Depth before textures:** Textures on depth-broken geometry produce worse visual confusion than colored geometry with correct depth. Depth is also simpler to implement and verify.
- **Shaders before textures:** The texture-aware fragment shader must exist before texture data has anywhere to go. Push constant MVP also belongs here since it changes the vertex format that texture UVs depend on.
- **Textures before state:** Without textures, you cannot visually verify that blend/alpha-test/fog are working correctly. State bugs on colored geometry are nearly invisible.
- **State before CBuff:** Chunk display lists record textured, state-dependent geometry. If state tracking is broken, every chunk renders incorrectly and debugging is impossible.
- **CBuff before scaling:** You cannot know how much vertex budget you need until terrain is actually rendering. Premature optimization here wastes effort.

### Research Flags

Phases likely needing deeper research during planning:
- **Phase 2 (Shader Infrastructure):** The NDC coordinate convention decision (Y-flip method, depth range mapping) needs careful validation against the engine's existing projection matrices. Test with both the `MatrixPerspective()` and `MatrixOrtho()` paths. The interaction between CPU-side matrix code and GPU-side push constants requires precise alignment verification.
- **Phase 3 (Texture Pipeline):** The exact byte ordering in `BufferedImage::loadData()` needs runtime verification -- codebase analysis suggests RGBA but the original game data may have platform-specific ordering. The descriptor management strategy (push descriptors vs update-on-bind) should be prototyped before committing.
- **Phase 5 (CBuff System):** The interaction between background chunk rebuild threads and main-thread vertex submission needs thread-safety analysis. The Tesselator's TLS usage may complicate the data flow. Exact CBuff lifecycle semantics (persistence across frames, update frequency, destruction patterns) need investigation.

Phases with standard patterns (skip research-phase):
- **Phase 1 (Bootstrap Stabilization):** Well-documented fixes -- swapchain fence ordering and validation layer enablement are straight from the Vulkan tutorial.
- **Phase 4 (Render State):** Pipeline state caching is thoroughly documented in Vulkan guides, and the game's state combinations are enumerable from the codebase.
- **Phase 6 (Vertex Scaling):** Ring buffer / per-frame allocation is a standard Vulkan pattern with many reference implementations.

## Confidence Assessment

| Area | Confidence | Notes |
|------|------------|-------|
| Stack | HIGH | All technologies already integrated or single-header vendored libraries. Versions verified against official sources (March 2026). No decisions outstanding. |
| Features | HIGH | Feature list derived from direct C4JRender API analysis (~70 methods enumerated). Every feature maps to specific stubbed methods with clear Vulkan implementation paths. |
| Architecture | HIGH | Three-layer architecture (glWrapper -> C4JRender -> VulkanBackend) already implemented and verified. New components fit within existing boundaries. Patterns well-established in Vulkan ecosystem. |
| Pitfalls | HIGH | All 8 critical pitfalls confirmed from codebase analysis with specific line references. Prevention strategies sourced from official Vulkan documentation and MoltenVK issue tracker. |

**Overall confidence:** HIGH

### Gaps to Address

- **BufferedImage byte ordering:** Analysis suggests RGBA, but actual in-memory format needs runtime verification when texture loading is first implemented. Log the first texture's raw bytes and compare against a known reference.
- **Tesselator thread-safety model:** The TLS-based Tesselator in background chunk rebuild threads may or may not be active on macOS (threading stubs may prevent it). Needs investigation when CBuff system is built.
- **MoltenVK dynamic state support:** The plan assumes MoltenVK 1.3+ supports `VK_EXT_extended_dynamic_state` for blend/depth/cull. This needs to be queried at runtime during Phase 2 with a fallback to pre-baked pipeline variants if not available.
- **Vertex budget sizing:** The 4-16MB estimate for per-frame vertex data is based on Tesselator constants, but actual peak usage during full terrain rendering needs measurement. Instrument vertex submission in Phase 5 before sizing the ring buffer in Phase 6.
- **sRGB vs linear texture format:** Whether to use `VK_FORMAT_R8G8B8A8_SRGB` or `VK_FORMAT_R8G8B8A8_UNORM` for texture images needs visual comparison against original game screenshots during Phase 3.
- **ArchiveFile texture extraction on macOS:** The game's texture loading goes through ArchiveFile which reads platform-specific archive formats. Whether these archives are available and readable on macOS needs Phase 3 investigation.
- **CBuff lifecycle semantics:** How often CBuffs are created vs destroyed, whether they persist across frames, and whether they are updated in-place affects the implementation strategy (persistent GPU buffers vs per-frame copies).

## Sources

### Primary (HIGH confidence)
- Direct codebase analysis of `4J_Render.h`, `VulkanRenderManager.cpp`, `VulkanBootstrapAppRender.cpp`, `VulkanBootstrapAppCore.cpp`, `glWrapper.cpp`, `Tesselator.cpp`, `Texture.cpp`, `Textures.h`, `Chunk.cpp`, `GameRenderer.h`, `LevelRenderer.h`
- [Vulkan 1.2 Specification](https://registry.khronos.org/vulkan/specs/1.2/html/)
- [MoltenVK GitHub v1.4.1](https://github.com/KhronosGroup/MoltenVK) -- release notes, issue tracker
- [VMA GitHub v3.3.0](https://github.com/GPUOpen-LibrariesAndSDKs/VulkanMemoryAllocator)
- [LunarG Vulkan SDK v1.4.341.1](https://vulkan.lunarg.com/sdk/home) -- Feb 2026
- [LunarG: State of Vulkan on Apple Jan 2026](https://www.lunarg.com/the-state-of-vulkan-on-apple-jan-2026/) -- KosmicKrisp comparison
- [Vulkan Tutorial](https://vulkan-tutorial.com/) -- depth buffering, swapchain recreation, staging buffers, frames in flight
- [NVIDIA Vulkan Dos and Don'ts](https://developer.nvidia.com/blog/vulkan-dos-donts/)
- [Vulkan common pitfalls (official)](https://docs.vulkan.org/guide/latest/common_pitfalls.html)
- [KDAB: Projection matrices with Vulkan](https://www.kdab.com/projection-matrices-with-vulkan-part-1/) -- NDC coordinate differences

### Secondary (MEDIUM confidence)
- [VulkanMod GitHub](https://github.com/xCollateral/VulkanMod) -- Minecraft Vulkan renderer architecture reference
- [Sokol Vulkan Backend (2025)](https://floooh.github.io/2025/12/01/sokol-vulkan-backend-1.html) -- fixed-function emulation patterns
- [vkguide.dev](https://vkguide.dev/) -- descriptor sets, image loading, depth buffer patterns
- [MoltenVK depth format issue #888](https://github.com/KhronosGroup/MoltenVK/issues/888) -- D32_SFLOAT preferred
- [MoltenVK triangle fan issue #1419](https://github.com/KhronosGroup/MoltenVK/issues/1419) -- topology limitation
- [MoltenVK geometry shader issue #1524](https://github.com/KhronosGroup/MoltenVK/issues/1524) -- Metal limitation
- [Vulkan staging buffer patterns](https://docs.vulkan.org/tutorial/latest/04_Vertex_buffers/02_Staging_buffer.html)
- [VMA: Recommended Usage Patterns](https://gpuopen-librariesandsdks.github.io/VulkanMemoryAllocator/html/usage_patterns.html)

### Tertiary (LOW confidence)
- [D3D12 and Vulkan: Lessons Learned (AMD GPUOpen)](https://gpuopen.com/download/d3d12_vulkan_lessons_learned.pdf) -- general porting guidance, not specific to this codebase

---
*Research completed: 2026-03-08*
*Ready for roadmap: yes*
