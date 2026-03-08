# Feature Landscape

**Domain:** Vulkan rendering backend for Minecraft Legacy Console Edition (C++ game engine port)
**Researched:** 2026-03-08

## Table Stakes

Features that must work or the rendering proof fails. Without these, the game cannot visually render its world, entities, or UI.

### Tier 1: Foundation (blocks everything else)

| Feature | Why Expected | Complexity | Notes |
|---------|--------------|------------|-------|
| **Texture creation and binding** | Every visible surface in the game is textured. Without textures, the game is just colored geometry (current state). `TextureCreate()`, `TextureData()`, `TextureBind()` are called on every frame. | High | Must manage VkImage + VkImageView + VkSampler per texture. Need descriptor sets and a descriptor pool. The game creates ~120+ textures at startup (see `TEXTURE_NAME` enum with `TN_COUNT` entries). Need texture ID -> VkImage mapping. |
| **Texture-aware shader** | Current shader is position+color only. The engine vertex format includes UV coordinates (float u, float v) that are currently discarded in `convertVertex()`. Without sampling textures in the fragment shader, no textures are visible. | Medium | Need a new vertex format with position+texcoord+color, a sampler uniform, and a fragment shader that samples a texture and multiplies by vertex color. This is the standard Minecraft rendering equation. |
| **Depth buffer** | Without depth testing, geometry draws in submission order, causing massive visual corruption. The game calls `glEnable(GL_DEPTH_TEST)`, `glDepthFunc()`, `glDepthMask()` constantly. | Medium | Requires a VkImage for depth, adding it to the render pass as a depth/stencil attachment, and creating a `VkPipelineDepthStencilStateCreateInfo`. Need to handle `StateSetDepthTestEnable()`, `StateSetDepthFunc()`, `StateSetDepthMask()`. |
| **Alpha blending** | Water, glass, particles, clouds, GUI overlays, item tooltips -- a large fraction of the game's rendering requires blending. `StateSetBlendEnable()` and `StateSetBlendFunc()` are called extensively. | Medium | Vulkan requires pipeline state variants for blend on/off. Need to track blend state and switch pipelines. The game uses ~6 blend function combinations (src_alpha/one_minus_src_alpha, one/one, src_alpha/one, etc.). |
| **Alpha testing** | Foliage, flowers, torches, doors, fences -- anything with transparent cutout pixels uses alpha test. `StateSetAlphaTestEnable()` and `StateSetAlphaFunc()` are called on many blocks. Without this, all transparency appears as solid black or fully opaque. | Medium | Not a native Vulkan feature -- must be implemented in the fragment shader with `discard`. Pass alpha reference value and function via push constants or UBO. |

### Tier 2: World Rendering (core visual proof)

| Feature | Why Expected | Complexity | Notes |
|---------|--------------|------------|-------|
| **Command buffer display lists** | Chunk rendering uses `CBuffCreate()`/`CBuffStart()`/`CBuffEnd()`/`CBuffCall()` to record vertex data into reusable "display lists" (not Vulkan command buffers -- these are vertex data caches). `Chunk::rebuild()` calls `glNewList()` and `glEndList()` which map to these. Without them, the world cannot render terrain. | High | Need a system that records DrawVertices calls during CBuffStart/CBuffEnd, stores the vertex data in GPU buffers, and replays them on CBuffCall. This is essentially a retained-mode vertex buffer manager. The game allocates up to `MAX_COMMANDBUFFER_ALLOCATIONS` (2GB on Windows64) for these buffers. |
| **Texture parameter control** | The game sets per-texture filtering and wrapping via `TextureSetParam()` (min/mag filter, wrap S/T). Terrain textures use NEAREST filtering (the blocky Minecraft look). Without this, textures look blurry or tile incorrectly. | Low | Map GL_NEAREST/GL_LINEAR to VkFilter, GL_CLAMP/GL_REPEAT to VkSamplerAddressMode. Create VkSampler objects per unique parameter combination. |
| **Orthographic projection for GUI** | The GUI (menus, HUD, inventory) uses `glOrtho()` for 2D rendering. Matrix math is already implemented in VulkanRenderManager.cpp. But the MVP needs to reach the shader. Currently, CPU-side matrix transform converts to clip space -- this works but is slow. | Low | Already working via CPU-side transform in `convertVertex()`. For the rendering proof, CPU-side MVP is acceptable. GPU-side UBO/push-constant MVP is a performance optimization for later. |
| **Vertex color modulation** | `StateSetColour()` sets a global color multiplier applied to all subsequent vertices. Used for tinting (night darkness, biome colors, damage flash, fog fade). Currently a no-op. | Low | Track the current color state. Either apply it CPU-side when converting vertices (multiply into vertex color) or pass as a push constant to the shader. |
| **Face culling** | `StateSetFaceCull()` and `StateSetFaceCullCW()` control backface culling. Without this, interior faces of blocks are visible (wasted GPU work and visual artifacts). | Low | Track cull mode state, set `VkPipelineRasterizationStateCreateInfo::cullMode` appropriately. Requires pipeline variants or dynamic state. |
| **Fog** | Distance fog is integral to Minecraft's atmosphere. `StateSetFogEnable/Mode/NearDistance/FarDistance/Density/Colour` are called every frame. Without fog, the world edge is a hard cutoff instead of a gradual fade. | Medium | Implement in fragment shader. Pass fog parameters (mode, start, end, density, color) via UBO or push constants. Calculate fog factor from vertex distance to camera. Three modes: linear, exp, exp2. |

### Tier 3: Entity and Detail Rendering

| Feature | Why Expected | Complexity | Notes |
|---------|--------------|------------|-------|
| **Lighting** | `StateSetLightingEnable()`, `StateSetLightColour()`, `StateSetLightDirection()`, `StateSetLightAmbientColour()` control directional lighting for entity models. Without lighting, mobs are flat-colored. | Medium | Two directional lights (LIGHT0, LIGHT1) plus ambient. Implement in vertex or fragment shader. Pass light data via UBO. The game uses this for entity rendering (not terrain -- terrain uses a lightmap texture). |
| **Light texture (lightmap)** | `GameRenderer::updateLightTexture()` creates a 16x16 texture encoding sky light + block light. This is bound as a second texture and sampled using the `tex2` UV from the vertex. Creates the day/night cycle lighting effect on terrain. | High | Requires multi-texture support (binding two textures simultaneously). The second texture is bound via `TextureBindVertex()`. Need descriptor set with two sampler bindings. The `_tex2` field in vertices encodes the lightmap UV. |
| **Texture sub-region updates** | `TextureDataUpdate()` is used for animated textures (water, lava, fire, portal). The game updates sub-rectangles of the terrain atlas every tick. | Medium | Use `vkCmdCopyBufferToImage` to update sub-regions of existing VkImages. Need a staging buffer for the pixel data. Called frequently (every game tick for animations). |
| **Line rendering** | The game renders selection outlines (block highlight when looking at a block) using `PRIMITIVE_TYPE_LINE_LIST` and `PRIMITIVE_TYPE_LINE_STRIP`. `StateSetLineWidth()` is also called. | Low | Current VulkanBootstrapApp only handles triangle primitives. Need to either support line topology or emulate lines with thin quads. MoltenVK may not support wide lines. |
| **Stencil buffer** | `StateSetStencil()` is called for the End portal rendering effect. | Low | Add stencil attachment to render pass. Not critical for the basic rendering proof but needed for visual completeness. |
| **Color write mask** | `StateSetWriteEnable()` controls per-channel write masking. Used during the End portal rendering and some special effects. | Low | Set `VkPipelineColorBlendAttachmentState::colorWriteMask` dynamically. |
| **Depth bias / polygon offset** | `StateSetDepthSlopeAndBias()` is used for block breaking overlay and selection outline rendering to prevent z-fighting. `glPolygonOffset()` maps to this. | Low | Set `depthBiasEnable`, `depthBiasConstantFactor`, `depthBiasSlopeFactor` in rasterization state. |

## Differentiators

Features that improve quality but are not required for the rendering proof to succeed.

| Feature | Value Proposition | Complexity | Notes |
|---------|-------------------|------------|-------|
| **GPU-side MVP transform** | Current CPU-side MVP in `convertVertex()` works but transforms every vertex on the CPU. Moving MVP to a vertex shader UBO/push constant would dramatically improve performance and is the standard approach. | Medium | Requires changing the shader to accept an MVP uniform, adding a descriptor set or push constant range, and no longer transforming vertices CPU-side. |
| **Multiple frames in flight** | Current implementation has a single fence/semaphore pair. Allowing 2-3 frames in flight would improve GPU utilization and reduce stalls. | Medium | Need per-frame sync objects, per-frame vertex buffers, per-frame command buffers. Standard Vulkan optimization. |
| **Dynamic vertex buffer growth** | Current `kMaxFrameVertices = 65536` is far too small for a full Minecraft world. The Tesselator alone allocates 16MB. Need dynamic growth or a much larger budget. | Medium | Either pre-allocate a large buffer (matching the engine's 16MB budget) or implement a ring buffer / sub-allocation scheme. |
| **Mipmapping** | The game supports mipmapped textures (`m_iMipLevels` calculation in Texture.cpp). Mipmaps reduce aliasing on distant terrain. | Medium | Need to generate mipmap levels (either CPU-side or via `vkCmdBlitImage`), allocate VkImages with multiple mip levels, and set `TextureSetTextureLevels()` properly. |
| **Texture atlas caching** | Pre-stitched texture maps (terrain atlas, items atlas) are loaded once and rarely change. Keeping them in device-local memory with staging upload is standard. | Low | Use `createBuffer` with staging + `vkCmdCopyBufferToImage` for initial texture upload. Already have `copyBuffer` infrastructure. |
| **Shader hot-reload** | During development, being able to reload shaders without restarting the game speeds up iteration. | Low | Watch shader files, recreate pipeline on change. Development-only feature. |
| **Validation layers** | Vulkan validation layers catch API misuse. Essential during development. | Low | Already available via `VK_LAYER_KHRONOS_validation`. Just need to enable in debug builds. |
| **Pipeline caching** | `VkPipelineCache` reduces pipeline creation time on subsequent launches. | Low | Create a pipeline cache, save to disk, load on startup. Small effort, nice quality of life. |

## Anti-Features

Features to explicitly NOT build for this milestone.

| Anti-Feature | Why Avoid | What to Do Instead |
|--------------|-----------|-------------------|
| **RAD Iggy UI replacement** | Iggy is the Flash-based UI system. Replacing it is a massive undertaking (entire UI framework). The game's Screen.cpp hierarchy and Common/UI system work without Iggy for basic rendering. | Keep Iggy stubbed. The non-Iggy Screen/UI path renders menus and HUD through the standard C4JRender path. |
| **Audio implementation** | Miles Sound System has ~50 stubbed functions. Audio is completely orthogonal to rendering. | Keep all Miles functions stubbed as no-ops. Zero audio. |
| **Multiplayer networking** | Networking requires platform network managers, QNet, session discovery -- all unrelated to rendering. | Keep `CPlatformNetworkManagerStub`. Single-player integrated server only. |
| **Splitscreen rendering** | The game supports 4-player split-screen with per-player viewports, render lists, and cameras. Supporting this correctly requires viewport management, per-player chunk allocation, and multiple render passes. | Render for player 0 only. Ignore `iPad` parameter / `XUSER_MAX_COUNT` arrays. |
| **Console-specific rendering paths** | PS3 SPU chunk rebuild, Vita alpha cutout optimization, Xbox 360 compressed vertex format. | These are excluded by platform `#ifdef`. No need to implement. |
| **Shader complexity matching original** | The original game has separate vertex/pixel shaders per vertex type (STANDARD, PROJECTION, FORCELOD, COMPRESSED, LIT, TEXGEN). Replicating all shader permutations is not needed for proof. | Start with a single uber-shader that handles the common case (textured + colored + lit). Add variants only as needed. |
| **VBO mode / hardware vertex buffers** | `Tesselator::USE_VBO` is false by default. The game's VBO path uses `ARBVertexBufferObject` which is an OpenGL extension. | Use immediate-mode vertex submission through the existing DrawVertices path. Optimize with proper VBOs later. |
| **Texture format variants** | The game uses only `TEXTURE_FORMAT_RxGyBzAw` (RGBA). No need to support other formats. | Hardcode RGBA8 format for all textures. |
| **Screen capture / thumbnails** | `CaptureThumbnail()`, `CaptureScreen()`, `DoScreenGrabOnNextPresent()` are for save file thumbnails and social sharing. | Keep as no-ops. |
| **Conditional rendering / occlusion queries** | `BeginConditionalSurvey/Rendering` are for hardware occlusion queries. | Keep as no-ops. The game already has CPU-side frustum culling. |
| **Gamma correction** | `UpdateGamma()` adjusts display gamma. | Keep as no-op. Renders fine without it. |
| **Anaglyph 3D** | `GameRenderer::anaglyph3d` for red/blue 3D glasses mode. | Ignore completely. |

## Feature Dependencies

```
Texture creation/binding --> Texture-aware shader (shader must sample textures)
Texture creation/binding --> Texture parameter control (filtering/wrapping)
Texture creation/binding --> Texture sub-region updates (animated textures)
Texture creation/binding --> Light texture (second texture binding)

Depth buffer --> Face culling (both needed for correct 3D)
Depth buffer --> Depth bias (z-fighting fix requires depth buffer)
Depth buffer --> Stencil buffer (shares depth/stencil attachment)

Alpha blending --> Alpha testing (both control transparency)
Alpha blending --> Fog (fog blends with fragment color)

Command buffer display lists --> Texture creation/binding (lists record textured geometry)
Command buffer display lists --> Depth buffer (lists render 3D geometry)

Lighting --> Light texture (both contribute to final lighting)

Vertex color modulation --> Alpha blending (tinted transparent geometry)

GPU-side MVP (differentiator) --> All rendering (performance improvement)
```

## MVP Recommendation

The minimum set of features to achieve a visual rendering proof (recognizable Minecraft world rendering):

### Phase 1: See Textures (highest priority)
1. **Texture creation and binding** -- textures must load and bind
2. **Texture-aware shader** -- fragment shader must sample textures
3. **Depth buffer** -- 3D geometry must depth-sort correctly
4. **Alpha blending** -- water, glass, and GUI transparency
5. **Alpha testing** -- foliage and cutout blocks

This phase transforms the game from "colored rectangles" to "recognizable textured Minecraft."

### Phase 2: See the World
6. **Command buffer display lists** -- terrain chunks must render
7. **Vertex color modulation** -- biome tinting and darkness
8. **Face culling** -- correct backface elimination
9. **Fog** -- distance fade instead of hard world edge
10. **Texture parameter control** -- NEAREST filtering for blocky look

This phase enables terrain rendering with the characteristic Minecraft visual style.

### Phase 3: See Everything
11. **Lighting** -- entity models look 3D
12. **Light texture (lightmap)** -- day/night cycle on terrain
13. **Texture sub-region updates** -- animated water/lava/fire
14. **Line rendering** -- block selection outline
15. **Depth bias** -- block breaking overlay

This phase completes the visual picture with all standard rendering effects.

### Defer to Later Milestones
- **GPU-side MVP transform**: Works CPU-side for now, optimize later
- **Multiple frames in flight**: Single frame is fine for proof
- **Mipmapping**: Nearest filtering is the Minecraft look anyway
- **Splitscreen**: Player 0 only
- **All anti-features listed above**

## Sources

- Direct codebase analysis of `Minecraft.Client/Vulkan/4JLibs/inc/4J_Render.h` (C4JRender interface)
- Direct codebase analysis of `Minecraft.Client/Vulkan/VulkanRenderManager.cpp` (current stub implementations)
- Direct codebase analysis of `Minecraft.Client/Vulkan/VulkanBootstrapAppRender.cpp` (Vulkan pipeline)
- Direct codebase analysis of `Minecraft.Client/glWrapper.cpp` (OpenGL -> C4JRender mapping)
- Direct codebase analysis of `Minecraft.Client/Tesselator.cpp` (vertex submission path)
- Direct codebase analysis of `Minecraft.Client/Texture.cpp` (texture creation)
- Direct codebase analysis of `Minecraft.Client/Textures.h` (texture management)
- Direct codebase analysis of `Minecraft.Client/Chunk.cpp` (terrain chunk rebuild/render)
- Direct codebase analysis of `Minecraft.Client/GameRenderer.h` (rendering pipeline)
- Direct codebase analysis of `Minecraft.Client/LevelRenderer.h` (world rendering)
- Vulkan specification knowledge for implementation complexity assessment (HIGH confidence -- well-established API)
