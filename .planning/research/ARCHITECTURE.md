# Architecture Patterns: Vulkan Rendering Backend for MCE

**Domain:** Vulkan rendering backend integration with existing C4JRender abstraction layer
**Researched:** 2026-03-08

## Recommended Architecture

The MCE Vulkan backend must bridge an OpenGL-style immediate-mode rendering API (C4JRender + glWrapper) to Vulkan's explicit, descriptor-based, pipeline-state-object model. The architecture has **three layers**: the existing game code speaks OpenGL-like calls through `glWrapper.cpp`, those map to `C4JRender` (RenderManager singleton), and `C4JRender` delegates to `VulkanBootstrapApp` which owns all Vulkan objects.

The current implementation has the skeleton right but is missing five critical subsystems: texture management, depth testing, render state tracking, shader programs with uniforms, and command buffer management. Each of these must be added as distinct components within the existing two-class boundary (C4JRender + VulkanBootstrapApp).

### System Diagram

```
Game Code (Minecraft.cpp, GameRenderer, LevelRenderer, TileRenderer, *Renderer, *Screen)
    |
    | calls glBindTexture(), glEnable(), glBlendFunc(), Tesselator::end()
    v
glWrapper.cpp  (OpenGL-to-C4JRender translation -- EXISTING, COMPLETE)
    |
    | calls RenderManager.TextureBind(), StateSetBlendEnable(), DrawVertices()
    v
C4JRender (VulkanRenderManager.cpp -- PARTIALLY IMPLEMENTED)
    |
    | Matrix stacks: EXISTING
    | Texture management: STUBBED (returns 0/null)
    | Render state: STUBBED (empty bodies)
    | DrawVertices: EXISTING (color-only, no textures)
    | Command buffers (CBuffCreate/Call): STUBBED
    |
    | delegates to
    v
VulkanBootstrapApp (VulkanBootstrapAppCore.cpp + VulkanBootstrapAppRender.cpp)
    |
    | Vulkan instance/device/swapchain: EXISTING
    | Render pass: COLOR ONLY (no depth attachment)
    | Pipeline: SINGLE (position+color, no texture coords)
    | Vertex buffer: SINGLE host-coherent mapped buffer
    | Descriptors: NONE
    | Textures: NONE
    |
    v
MoltenVK / Vulkan Driver
```

### Component Boundaries

| Component | Responsibility | Communicates With | Current State |
|-----------|---------------|-------------------|---------------|
| **glWrapper.cpp** | Translates OpenGL calls to C4JRender methods | C4JRender (RenderManager) | Complete -- no changes needed |
| **C4JRender** (4J_Render.h) | Abstraction interface; holds matrix stacks, render state, texture slot mapping | glWrapper.cpp (receives), VulkanBackend (delegates) | Partial -- matrices work, textures/state stubbed |
| **VulkanTextureManager** (new) | Manages VkImage/VkImageView/VkSampler per texture slot; handles upload via staging buffers | C4JRender (TextureCreate/Bind/Data), VulkanBackend (for Vulkan objects) | Does not exist |
| **VulkanPipelineCache** (new) | Creates/caches VkPipeline objects based on render state bitmask | C4JRender (state changes), VulkanBackend (for render passes) | Does not exist |
| **VulkanRenderState** (new) | Tracks current blend, depth, alpha test, fog, lighting state; produces pipeline state key | C4JRender (StateSet* calls), VulkanPipelineCache (pipeline lookup) | Does not exist |
| **VulkanDescriptorManager** (new) | Allocates/updates descriptor sets for texture binding and uniform buffers | VulkanTextureManager, VulkanPipelineCache, VulkanBackend | Does not exist |
| **VulkanShaderManager** (new) | Loads/compiles SPIR-V shaders; manages shader modules and push constant layouts | VulkanPipelineCache, VulkanBackend | Does not exist (two bootstrap shaders exist) |
| **VulkanBootstrapApp** (existing) | Owns Vulkan instance, device, swapchain, command pool, sync objects; executes draw commands | C4JRender (receives batched draws), all Vulkan sub-managers | Exists -- needs extension for depth buffer, multi-pipeline, textures |

### Data Flow

**Texture Loading (currently broken):**

```
1. Game calls: Textures::loadTexture() or Textures::bindTexture()
2. Textures reads image from ArchiveFile -> BufferedImage (int* RGBA pixel data)
3. Texture::_init() calls glGenTextures() -> RenderManager.TextureCreate() -> returns slot ID
4. Texture::_init() calls glBindTexture() -> RenderManager.TextureBind(id)
5. glTexImage2D() -> RenderManager.TextureData(width, height, data, level)
6. [BROKEN HERE] TextureData() is empty stub -- pixel data goes nowhere

NEEDED: TextureData() must:
  a. Create VkImage (VK_FORMAT_R8G8B8A8_UNORM, width x height)
  b. Allocate device-local VkDeviceMemory
  c. Create staging buffer, memcpy pixel data into it
  d. Record vkCmdCopyBufferToImage with layout transitions
  e. Create VkImageView and VkSampler
  f. Store in texture slot map indexed by the integer ID
```

**Texture Binding (currently broken):**

```
1. Game calls: glBindTexture(GL_TEXTURE_2D, textureId) or Textures::bind(id)
2. glWrapper: RenderManager.TextureBind(id)
3. [BROKEN HERE] TextureBind() is empty stub

NEEDED: TextureBind() must:
  a. Look up VkImageView + VkSampler from texture slot map
  b. Update current descriptor set with the bound texture
  c. Mark descriptor as dirty so next draw call binds it
```

**Vertex Submission (partially working):**

```
1. Game code uses Tesselator: begin() -> tex(u,v) -> color(r,g,b) -> vertex(x,y,z) -> end()
2. Tesselator::end() calls RenderManager.DrawVertices(primitiveType, count, data, vertexType, pixelShader)
3. DrawVertices() in VulkanRenderManager.cpp:
   a. Reads EngineVertex structs (position[3], uv[2], color[1], normal[1], extra[1] = 32 bytes each)
   b. Multiplies position by CPU-side MVP matrix (modelview * projection)
   c. Converts to VulkanBootstrapApp::Vertex (position[3] + color[4])
   d. [DISCARDS UV coordinates] -- this is why textures don't render
   e. Submits converted vertices to VulkanBootstrapApp::submitVertices()

NEEDED:
  a. Pass UV coordinates through to the shader (add to vertex format)
  b. Move MVP transform to GPU (push constants or uniform buffer)
  c. Sample bound texture in fragment shader using UVs
  d. Apply render state (blend, depth, alpha test) via pipeline selection
```

**Frame Lifecycle (working but incomplete):**

```
1. main.cpp game loop: RenderManager.StartFrame() -> game tick + render -> RenderManager.Present()
2. StartFrame(): g_vulkanBackend.beginFrame() -- clears frame vertex/batch arrays
3. During frame: multiple DrawVertices() calls accumulate into frameVertices_ / frameBatches_
4. Present(): g_vulkanBackend.tickFrame() -> drawFrame()
5. drawFrame():
   a. vkWaitForFences / vkAcquireNextImageKHR
   b. memcpy all frame vertices to mapped vertex buffer
   c. Record command buffer: begin render pass, bind pipeline, draw batches, end render pass
   d. vkQueueSubmit / vkQueuePresentKHR

ISSUES:
  - Single vertex buffer capped at 65536 vertices (too small for terrain)
  - Single pipeline (no texture sampling, no depth, no blend state variations)
  - No depth buffer attachment in render pass
  - No descriptor sets for textures
  - CPU-side MVP transform is slow (should be GPU push constants)
```

**Command Buffer System (CBuff -- currently stubbed):**

```
The CBuff* API (CBuffCreate, CBuffStart, CBuffEnd, CBuffCall) is how the engine
pre-records render commands for chunk geometry. glNewList()/glCallList() in
glWrapper.cpp maps to these. Currently all stubbed returning 0/false.

This system is used by:
- LevelRenderer: pre-compiles chunk geometry into "display lists"
- Chunk::rebuild(): records tile rendering into a command buffer
- renderChunks(): calls CBuffCall() to replay recorded geometry
- Stars, sky, clouds: pre-built geometry stored as display lists

NEEDED: Implement as secondary command buffers OR as pre-recorded vertex
buffers that can be replayed. The original uses D3D command buffers on Xbox
and GL display lists conceptually. For Vulkan, the simplest approach is
storing vertex data + draw parameters per slot, then replaying them.
```

## Patterns to Follow

### Pattern 1: Render State Bitmask for Pipeline Selection

**What:** Track all render state changes (blend, depth, alpha test, cull, fog) as a single bitmask/struct. Use this as a key into a pipeline cache. Create VkPipeline objects lazily on first use for each unique combination.

**When:** Every frame, on every state change call (StateSetBlendEnable, StateSetDepthTestEnable, etc.)

**Why:** Vulkan requires pre-created pipeline state objects. The game code sets state in an OpenGL immediate-mode style with dozens of StateSet* calls per frame. We cannot create a new VkPipeline per call. Instead, track dirty state and only create/bind a new pipeline when DrawVertices is called AND state has actually changed.

**Example:**
```cpp
struct RenderStateKey {
    uint32_t blendEnable : 1;
    uint32_t blendSrc : 4;
    uint32_t blendDst : 4;
    uint32_t depthTestEnable : 1;
    uint32_t depthWriteEnable : 1;
    uint32_t depthFunc : 3;
    uint32_t alphaTestEnable : 1;
    uint32_t alphaFunc : 3;
    uint32_t faceCullEnable : 1;
    uint32_t faceCullCW : 1;
    uint32_t textureEnabled : 1;
    // ... fog, lighting handled in shader uniforms
};

// In DrawVertices():
RenderStateKey key = currentState_.toKey();
VkPipeline pipeline = pipelineCache_.getOrCreate(key, renderPass_, shaderModules_);
if (pipeline != lastBoundPipeline_) {
    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
    lastBoundPipeline_ = pipeline;
}
```

**Estimated unique pipeline states:** The game uses roughly 15-25 distinct render state combinations across terrain, entities, particles, UI, sky, and weather rendering. This is a manageable number for a pipeline cache.

### Pattern 2: Texture Slot Map with Staging Upload

**What:** Map C4JRender's integer texture IDs (from TextureCreate()) to Vulkan texture objects. Upload pixel data via a staging buffer on TextureData(). Bind via descriptor set update on TextureBind().

**When:** During game initialization (Minecraft::init() loads ~100+ textures) and at runtime for dynamic textures (light map, animated textures).

**Example:**
```cpp
struct VulkanTexture {
    VkImage image;
    VkDeviceMemory memory;
    VkImageView view;
    VkSampler sampler;
    uint32_t width, height;
    uint32_t mipLevels;
};

// TextureCreate() returns incrementing IDs (0, 1, 2, ...)
// Store: std::unordered_map<int, VulkanTexture> textureSlots_;

// TextureData(width, height, data, level):
// 1. Create staging buffer + memcpy pixel data
// 2. Create VkImage if not exists for this slot
// 3. Transition VK_IMAGE_LAYOUT_UNDEFINED -> VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL
// 4. vkCmdCopyBufferToImage
// 5. Transition VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL -> VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
// 6. Create VkImageView + VkSampler if not exists
```

### Pattern 3: Push Constants for MVP Matrix

**What:** Pass the model-view-projection matrix to the vertex shader via Vulkan push constants (128 bytes guaranteed), eliminating CPU-side vertex transformation.

**When:** Replace the current CPU-side `transformPoint()` calls in `convertVertex()` with a GPU-side vertex shader that reads MVP from push constants.

**Why:** The current code transforms EVERY vertex on the CPU before sending to GPU. This is the single biggest performance bottleneck. Push constants are faster than uniform buffers for small, frequently-changing data like MVP matrices. Vulkan guarantees at least 128 bytes of push constant space -- exactly enough for two 4x4 matrices (MVP + texture transform).

**Example:**
```glsl
// Vertex shader:
layout(push_constant) uniform PushConstants {
    mat4 mvp;
} pc;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec2 inTexCoord;
layout(location = 2) in vec4 inColor;

void main() {
    gl_Position = pc.mvp * vec4(inPosition, 1.0);
    gl_Position.y = -gl_Position.y; // Vulkan Y-flip
}
```

### Pattern 4: Deferred Draw Batching

**What:** Buffer all DrawVertices calls during a frame, then sort by pipeline state and texture to minimize state changes when recording the command buffer.

**When:** Between StartFrame() and Present().

**Why:** The game makes hundreds of DrawVertices calls per frame with interleaved state changes. Naive immediate submission would require hundreds of pipeline binds. Batching by state reduces this dramatically.

**Implementation note:** The current code already batches vertices into `frameVertices_` / `frameBatches_`. Extend this to also record the pipeline state key and bound texture per batch, then sort batches by (pipeline, texture) before recording the command buffer.

### Pattern 5: Dynamic Vertex Buffer with Ring Allocation

**What:** Replace the single 65536-vertex host-coherent buffer with a larger ring buffer (or per-frame buffers for double/triple buffering) that can handle full terrain + entity + particle rendering.

**When:** Immediately -- 65536 vertices is far too few. A single chunk rebuild can produce 20,000+ vertices. Full scene rendering needs 500K-2M+ vertices.

**Example:** Use 2-3 frame buffers, each 8-16MB. Map persistently. Write sequentially during frame. Reset offset at frame start. If budget exceeded mid-frame, flush and start new command buffer submission.

## Anti-Patterns to Avoid

### Anti-Pattern 1: Creating VkPipeline Per Draw Call

**What:** Creating a new Vulkan graphics pipeline every time render state changes.
**Why bad:** VkPipeline creation is extremely expensive (can take milliseconds). With 200+ draw calls per frame and frequent state changes, this would make the game unplayable.
**Instead:** Cache pipelines by render state bitmask. Pre-warm common combinations during init. The game uses roughly 15-25 unique state combinations -- cache all of them.

### Anti-Pattern 2: CPU-Side Vertex Transformation

**What:** Multiplying every vertex by the MVP matrix on the CPU (the CURRENT implementation in `convertVertex()`).
**Why bad:** Transforms 100K+ vertices per frame on CPU. Defeats the purpose of having a GPU. Prevents future use of pre-recorded vertex buffers (CBuffCall system).
**Instead:** Pass MVP as push constant, transform in vertex shader. The current approach exists only as a bootstrap expedient and must be replaced.

### Anti-Pattern 3: Synchronous Texture Upload

**What:** Uploading texture data synchronously by blocking the main thread until vkQueueWaitIdle completes.
**Why bad:** Blocks the game loop during texture loading. The game loads 100+ textures during init -- synchronous uploads would cause a multi-second stall on each texture.
**Instead:** Use a dedicated transfer queue (if available) or batch all texture uploads into a single command buffer submission. For init-time textures, submit all uploads, then waitIdle once. For runtime texture updates (animated textures, light map), use double-buffered staging and submit as part of the frame command buffer.

### Anti-Pattern 4: One Descriptor Set Per Texture

**What:** Allocating a new VkDescriptorSet for every texture bind.
**Why bad:** Hundreds of descriptor set allocations per frame. Pool fragmentation. Unnecessarily complex.
**Instead:** Use a small number of descriptor sets (1-2 per frame) and update the texture binding via `vkUpdateDescriptorSets` or push descriptors when the bound texture changes. Alternatively, use a bindless approach with a large texture array and pass the texture index as a push constant. For this codebase's scale (~100 unique textures), simple update-on-bind is sufficient.

### Anti-Pattern 5: Implementing CBuff as Vulkan Secondary Command Buffers

**What:** Trying to use Vulkan secondary command buffers to implement the CBuffCreate/CBuffCall display list system.
**Why bad:** Secondary command buffers in Vulkan have strict compatibility requirements with render passes and pipeline state. The engine's CBuffCall pattern expects display lists that can be replayed in any context with the current state applied. This doesn't map cleanly to Vulkan secondary command buffers.
**Instead:** Implement CBuff as stored vertex data + draw parameters. CBuffStart records to a CPU-side vertex array. CBuffEnd finalizes it. CBuffCall copies that vertex data into the current frame's vertex buffer and issues a draw. This is conceptually simpler and matches how the original code uses them (pre-compiled geometry, not pre-compiled state).

## Build Order (Dependencies Between Components)

The components must be built in this order because each depends on the previous:

```
Phase 1: Depth Buffer
    No dependencies on other new components.
    Required before: anything that renders 3D (terrain, entities).
    Add depth attachment to render pass. Create depth image/view.
    Update pipeline to enable depth test/write.

Phase 2: Shader Infrastructure
    Depends on: Phase 1 (render pass format must match)
    New vertex shader with position + UV + color inputs.
    New fragment shader with texture sampling.
    Push constant layout for MVP matrix.
    Move vertex transform from CPU to GPU.

Phase 3: Texture Management
    Depends on: Phase 2 (shaders must accept texture coordinates)
    Implement TextureCreate/TextureData/TextureBind.
    Staging buffer upload pipeline.
    Descriptor set for texture binding.
    -> At this point: UI and simple textured geometry renders correctly.

Phase 4: Render State + Pipeline Cache
    Depends on: Phase 2 (shader modules), Phase 1 (depth format)
    Implement StateSet* methods to track state.
    Pipeline cache keyed by render state.
    -> At this point: blend, depth, alpha test all work correctly.

Phase 5: Command Buffer System (CBuff)
    Depends on: Phase 2 (vertex format), Phase 3 (textures), Phase 4 (state)
    Implement CBuffCreate/Start/End/Call as stored vertex arrays.
    -> At this point: chunk rendering works (the big unlock for terrain).

Phase 6: Dynamic Vertex Buffer Scaling
    Depends on: Phase 5 (full scene rendering generates large vertex counts)
    Replace 65K cap with ring buffer or per-frame allocations.
    -> At this point: full scenes render without vertex budget overflow.
```

### Dependency Graph

```
[Depth Buffer] -----> [Shader Infrastructure] -----> [Texture Management]
                              |                              |
                              v                              v
                       [Render State + Pipeline Cache] <-----+
                              |
                              v
                       [CBuff System]
                              |
                              v
                       [Dynamic Vertex Scaling]
```

## Scalability Considerations

| Concern | Current (Bootstrap) | After Full Implementation | Notes |
|---------|--------------------|-----------------------------|-------|
| Vertex capacity | 65,536 per frame | 2M+ per frame | Ring buffer or per-frame allocation |
| Texture count | 0 (all stubbed) | ~150 active textures | Integer-keyed slot map |
| Pipeline objects | 1 (hardcoded) | 15-25 cached | Lazy creation, state bitmask key |
| Draw calls per frame | ~200 (colored quads) | ~500-1000 (textured, state-varied) | Batch by state to reduce binds |
| Memory: vertex buffers | 1.8 MB (65K * 28 bytes) | 16-32 MB (ring buffer) | Host-coherent for simplicity |
| Memory: textures | 0 | ~50-100 MB device-local | Terrain atlas + mob textures + UI |
| Frame sync | 1 fence, 1 frame in flight | 2-3 frames in flight | Reduces CPU-GPU stalls |

## MoltenVK-Specific Constraints

| Constraint | Impact | Mitigation |
|-----------|--------|------------|
| No geometry shaders | Cannot use geometry shaders for point/line expansion | Not needed -- game uses triangle lists |
| Push constant limit varies | Metal guarantees 4KB but MoltenVK maps to Metal argument buffers | 128 bytes for MVP is well within limits |
| VK_FORMAT support varies | Not all Vulkan formats available on Metal | Use VK_FORMAT_R8G8B8A8_UNORM (universally supported) |
| Swizzle limitations | Some texture swizzle modes unsupported | Game uses RGBA consistently -- no issue |
| No PVRTC via staging buffer | Compressed texture upload via staging can malform | Game uses uncompressed RGBA bitmaps -- no issue |
| Max descriptor sets | Limited compared to discrete GPUs | Game needs very few descriptors -- no issue |

## Sources

- [Vulkan Tutorial: Depth Buffering](https://vulkan-tutorial.com/Depth_buffering)
- [Vulkan Tutorial: Texture Mapping / Images](https://vulkan-tutorial.com/Texture_mapping/Images)
- [Vulkan Guide: Loading Images](https://vkguide.dev/docs/chapter-5/loading_images/)
- [Vulkan Guide: Descriptor Sets](https://vkguide.dev/docs/chapter-4/descriptors/)
- [Vulkan Guide: Setting up depth buffer](https://vkguide.dev/docs/chapter-3/depth_buffer/)
- [Sokol Vulkan Backend (2025)](https://floooh.github.io/2025/12/01/sokol-vulkan-backend-1.html)
- [Vulkan Docs: Engine Architecture - Rendering Pipeline](https://docs.vulkan.org/tutorial/latest/Building_a_Simple_Engine/Engine_Architecture/05_rendering_pipeline.html)
- [Vulkan Docs: Engine Architecture - Resource Management](https://docs.vulkan.org/tutorial/latest/Building_a_Simple_Engine/Engine_Architecture/04_resource_management.html)
- [NVIDIA Vulkan Dos and Don'ts](https://developer.nvidia.com/blog/vulkan-dos-donts/)
- [Zoo: Vulkan Resource Binding in 2023](https://zoo.dev/blog/vulkan-resource-binding-2023)
- [VMA: Recommended Usage Patterns](https://gpuopen-librariesandsdks.github.io/VulkanMemoryAllocator/html/usage_patterns.html)
- [MoltenVK PVRTC Issue #854](https://github.com/KhronosGroup/MoltenVK/issues/854)

---

*Architecture analysis: 2026-03-08*
