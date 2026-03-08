# Domain Pitfalls

**Domain:** D3D/OpenGL fixed-function to Vulkan port on macOS via MoltenVK
**Researched:** 2026-03-08

## Critical Pitfalls

Mistakes that cause rewrites or major issues.

### Pitfall 1: Vulkan Pipeline State Explosion from Immediate-Mode State Changes

**What goes wrong:** The game changes render state (blend, depth, cull, alpha test, fog, lighting, stencil) via ~30 `C4JRender::StateSet*()` calls per frame in arbitrary order. In OpenGL/D3D11 these are cheap state changes. In Vulkan, each unique combination of these states requires a separate `VkGraphicsPipeline` object baked at creation time.

**Why it happens:** The C4JRender API mirrors OpenGL's state machine model. The game changes state freely: enable blending, draw some quads, disable blending, change depth function, draw more quads. Naively creating a VkPipeline for every unique combination leads to thousands of pipelines, extreme creation latency (1-10ms per `vkCreateGraphicsPipelines` call), and memory waste.

**Consequences:** If you create pipelines on-demand during rendering, 200+ state changes per frame means 40-2000ms just for pipeline creation -- unplayable. Alternatively, using a single pipeline and ignoring state changes produces incorrect rendering (no transparency, wrong depth, missing face culling).

**Prevention:**
- Use Vulkan 1.3 dynamic states (`VK_DYNAMIC_STATE_*`) to minimize pipeline variants. MoltenVK 1.3+ supports: viewport, scissor, depth test enable, depth write enable, depth compare op, cull mode, front face, blend enable, color blend equation, stencil test/ops. This is the single most important architectural decision.
- Alpha test has NO Vulkan equivalent -- it must be implemented in the fragment shader via `discard`. This requires at minimum two shader/pipeline variants (with/without alpha test).
- Track render state as a bitmask in the StateSet* methods. Create VkPipeline objects lazily (on first use of a new non-dynamic state combination) and cache them permanently. In practice, the game uses only 15-25 unique state combinations.
- Implement a `VkPipelineCache` to speed up creation of variants needed at runtime.

**Detection:** Frame time spikes on the first few frames after loading, followed by smooth rendering. If spikes continue indefinitely, the state tracking is not deduplicating properly. Incorrect transparency or depth sorting in rendered output.

**Phase relevance:** Architecture must be designed in Phase 2 (pipeline/shader infrastructure). Will affect every subsequent phase.

**Confidence:** HIGH -- fundamental Vulkan architecture, confirmed by [official Vulkan documentation](https://docs.vulkan.org/guide/latest/common_pitfalls.html) and [NVIDIA Vulkan Dos and Don'ts](https://developer.nvidia.com/blog/vulkan-dos-donts/).

---

### Pitfall 2: CPU-Side Vertex Transform Bottleneck

**What goes wrong:** The current `VulkanRenderManager.cpp` transforms every vertex by the MVP matrix on the CPU (`convertVertex()`) before uploading to the GPU. This works for the bootstrap (few vertices) but will collapse under real game workload.

**Why it happens:** The bootstrap approach mimics OpenGL immediate mode where the driver handled transforms. The MCE Tesselator can emit up to 2M floats per frame (16MB buffer, `MAX_MEMORY_USE = 16 * 1024 * 1024`). The triple bottleneck (CPU transform + memcpy + draw) caps framerate far below GPU capability.

**Consequences:** Unplayable frame rates once terrain rendering is active. The 65,536 vertex budget (`kMaxFrameVertices`) in `VulkanBootstrapAppRender.cpp` is far below what a single chunk rebuild produces. The Tesselator's `MAX_FLOATS = 2,097,152` implies the game pushes vastly more geometry per frame.

**Prevention:**
- Move MVP transformation to the vertex shader via push constants or uniform buffers. Push constants are ideal: 128 bytes guaranteed on all Vulkan implementations = exactly 2 mat4 matrices (projection + modelview). The matrices are already tracked in `C4JRender`'s `projectionStack_` and `modelViewStack_`.
- Use a staging buffer + device-local vertex buffer pattern instead of host-visible only.
- Implement per-frame ring buffers (double/triple buffering) sized for the Tesselator's actual output volume.
- Budget at minimum 4MB of vertex data per frame, not 65K vertices.

**Detection:** High CPU usage in `DrawVertices()`, low GPU utilization in profiling tools (Metal System Trace on macOS). Frame rate drops when looking at terrain with many visible chunks.

**Phase relevance:** Must be addressed in Phase 2 (shader/UBO infrastructure) before terrain rendering in Phase 3.

**Confidence:** HIGH -- derived directly from codebase analysis of `VulkanRenderManager.cpp` lines 152-164 and `Tesselator.h` line 14.

---

### Pitfall 3: Texture Pipeline Stubs Silently Discard All Data

**What goes wrong:** Every texture-related `C4JRender` method is stubbed to do nothing: `TextureCreate()` returns 0, `TextureBind()` is empty, `TextureData()` is empty, `LoadTextureData()` returns 0. The game's texture loading pipeline (`TextureManager` -> `Texture` -> `BufferedImage` -> `glWrapper` -> `C4JRender`) calls these methods extensively, and all data is silently discarded.

**Why it happens:** The stub approach got the build compiling, but implementing the texture pipeline requires a full Vulkan texture management system: `VkImage` creation, `VkImageView`, `VkSampler`, staging buffer uploads, image layout transitions, and descriptor set binding. This is ~15 C4JRender methods that need coordinated implementation.

**Consequences:** Without textures, the game renders colored geometry only (current state). Partial implementation causes harder bugs: if `TextureCreate()` returns valid IDs but `TextureBind()` doesn't bind them, the game logic proceeds as if textures are bound, leading to incorrect shader sampling or crashes from null descriptor sets.

**Prevention:**
- Implement texture methods as a cohesive unit, not individually. Minimum viable set: `TextureCreate()`, `TextureData()`, `TextureBind()`, `TextureFree()`, `TextureSetParam()`, and `LoadTextureData()`.
- Track Vulkan texture objects (VkImage, VkImageView, VkDeviceMemory, VkSampler) in a map keyed by the integer IDs from `TextureCreate()`.
- Implement image layout transitions correctly: `VK_IMAGE_LAYOUT_UNDEFINED` -> `VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL` -> `VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL`.
- Batch all init-time texture uploads into a single command buffer submission (the game loads ~120+ textures during `Minecraft::init()`). Synchronous per-texture upload with `vkQueueWaitIdle` would stall for 5-15 seconds total.
- Test with the terrain atlas first -- it is the most critical single texture.

**Detection:** Black or missing textures on geometry. Validation layer errors about unbound descriptor sets or image layout mismatches. Long stalls during init if uploads are synchronous.

**Phase relevance:** This IS Phase 2 -- the texture pipeline is the next major milestone.

**Confidence:** HIGH -- directly observed in `VulkanRenderManager.cpp` stubs (lines 376-392) and `Texture.cpp` / `glWrapper.cpp` call sites (25 total call sites across 4 files).

---

### Pitfall 4: Vulkan NDC Y-Axis and Depth Range Differ from OpenGL/D3D

**What goes wrong:** Vulkan's normalized device coordinates have Y pointing downward (top-left origin) and depth range [0, 1], while OpenGL uses Y upward (bottom-left origin) and depth [-1, 1]. D3D11 uses Y downward but depth [0, 1]. The MCE codebase was ported from Java Minecraft (OpenGL) to D3D (console), so coordinate conventions are inconsistent.

**Why it happens:** The current `VulkanRenderManager.cpp` already flips Y in `convertVertex()` (`result.position[1] = -transformed[1]`), but this CPU-side hack must move to the shader when MVP moves to the GPU. The perspective matrix uses OpenGL conventions (symmetric Z range), producing incorrect depth values in Vulkan's [0, 1] range.

**Consequences:** Depth testing produces incorrect results -- objects at wrong depth get culled or render in front of other objects. UI elements render upside down. Camera view produces a mirrored world.

**Prevention:**
- For Y flip, use negative viewport height (`VK_KHR_maintenance1`, supported by MoltenVK) or negate projection matrix `[1][1]`. Do NOT flip Y per-vertex on the CPU or add `gl_Position.y = -gl_Position.y` in the shader (both work but are fragile).
- For depth range, either: (a) use `VK_EXT_depth_clip_control` to set `negativeOneToOne = VK_TRUE` to keep OpenGL-style depth, or (b) modify `MatrixPerspective()` to output Vulkan [0, 1] range: `result[10] = zFar / (zNear - zFar)` and `result[14] = (zFar * zNear) / (zNear - zFar)`.
- Pick ONE approach and apply consistently. Mixing CPU flips with shader flips produces double-flipped output that looks correct in simple cases but breaks with mirrors/portals/reflections.

**Detection:** Upside-down rendering. Depth fighting on geometry that should be clearly separated. Objects appearing behind the camera. Skybox rendering inside-out.

**Phase relevance:** Must be decided in Phase 2 (shader infrastructure) and enforced through all subsequent phases.

**Confidence:** HIGH -- well-documented, directly observable in current codebase.

**Sources:**
- [How transformations differ from OpenGL to Vulkan (KDAB)](https://www.kdab.com/projection-matrices-with-vulkan-part-1/)
- [Why OpenGL projection matrices fail in Vulkan](https://johannesugb.github.io/gpu-programming/why-do-opengl-proj-matrices-fail-in-vulkan/)

---

### Pitfall 5: Triangle Fan Topology Not Supported on MoltenVK

**What goes wrong:** `VK_PRIMITIVE_TOPOLOGY_TRIANGLE_FAN` is not supported by Metal and is reported as unsupported via `VkPhysicalDevicePortabilitySubsetFeaturesKHR::triangleFans = VK_FALSE`. Any draw call using triangle fan topology fails validation or produces no output.

**Why it happens:** Metal has no native triangle fan primitive. The MCE codebase uses `PRIMITIVE_TYPE_TRIANGLE_FAN` (mapped to `GL_TRIANGLE_FAN`) in `Tesselator::end()`. The current `VulkanRenderManager.cpp` already converts triangle fans to triangle lists in `DrawVertices()`, but any new draw path bypassing this conversion will silently fail.

**Consequences:** Geometry rendered as triangle fans produces nothing on screen, with no error unless validation layers are active. This works on desktop Vulkan (NVIDIA/AMD) but fails ONLY on MoltenVK.

**Prevention:**
- Always convert triangle fans to triangle lists at the `C4JRender::DrawVertices()` level (already done, must be maintained).
- Add a startup check: query `VkPhysicalDevicePortabilitySubsetFeaturesKHR` and assert/log if `triangleFans` is false.
- Enable `VK_KHR_portability_subset` at instance creation with `VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR` (already defined in `VulkanBootstrapApp.h` line 12).

**Detection:** Enable validation layers. Visually: missing geometry that works on other Vulkan platforms.

**Phase relevance:** Already partially addressed. Must be maintained through all phases.

**Confidence:** HIGH -- verified via [MoltenVK source code](https://github.com/KhronosGroup/MoltenVK/issues/1419) and issue tracker.

---

### Pitfall 6: Misunderstanding the CBuff System as Vulkan Command Buffers

**What goes wrong:** The CBuff API (`CBuffCreate`/`CBuffStart`/`CBuffEnd`/`CBuffCall`) looks like Vulkan command buffers but is NOT. It is a display-list system that records VERTEX DATA, not GPU commands. Implementing it as Vulkan secondary command buffers leads to render pass compatibility issues and incorrect state application.

**Why it happens:** "Command buffer" in 4J Studios' terminology means "a reusable batch of pre-recorded geometry." In Vulkan, command buffers are GPU command streams with strict render pass and pipeline state requirements.

**Consequences:** If implemented as Vulkan secondary command buffers, every `CBuffCall` would need to inherit the current render pass and pipeline state. But the game calls `CBuffCall` with different pipeline states (layer 0 = opaque, layer 1 = translucent). This mismatch causes validation errors or visual artifacts.

**Prevention:** Implement CBuff as stored vertex arrays. `CBuffStart` begins recording to a temporary vertex buffer. `CBuffEnd` finalizes it. `CBuffCall` copies the stored vertices into the current frame's draw stream and issues a draw call with the CURRENT pipeline state. This matches OpenGL display list semantics.

**Detection:** Vulkan validation errors about secondary command buffer render pass incompatibility. Terrain rendering with wrong blend state.

**Phase relevance:** Phase 3 (terrain rendering, when chunk display lists are used).

**Confidence:** HIGH -- confirmed from codebase analysis of CBuff API and `Chunk.cpp` usage patterns.

---

### Pitfall 7: Descriptor Set Management Overhead

**What goes wrong:** The game binds textures via `RenderManager.TextureBind(id)` immediately before draw calls -- OpenGL-style "bind and draw." Naively creating a new `VkDescriptorSet` per `TextureBind()` call exhausts the descriptor pool and creates massive allocation overhead.

**Why it happens:** OpenGL texture binding is a cheap state change. Vulkan descriptor sets must be allocated from pools, written with resource handles, and bound. The game calls `TextureBind()` potentially hundreds of times per frame.

**Consequences:** VkDescriptorPool runs out mid-frame causing crashes, or allocation overhead makes rendering CPU-bound.

**Prevention:** Use ONE of these approaches (simplest to most complex):
1. **Single descriptor set, update on bind:** Allocate one descriptor set at init. On `TextureBind()`, call `vkUpdateDescriptorSets` to point at the new texture. Works because the game renders synchronously within a frame.
2. **Push descriptors (`VK_KHR_push_descriptor`):** Use `vkCmdPushDescriptorSetKHR` inline during command buffer recording. Avoids pre-allocated sets entirely. Supported on MoltenVK.
3. **Bindless (texture array):** Create a large descriptor set with all textures. Pass texture index as push constant. Overkill for this project's scale, and MoltenVK's descriptor indexing is limited to Metal Tier 1 (96-128 textures) on older macOS.

**Detection:** Validation errors about descriptor pool exhaustion. Crashes from `vkAllocateDescriptorSets` returning `VK_ERROR_OUT_OF_POOL_MEMORY`.

**Phase relevance:** Must be designed in Phase 2 (texture pipeline).

**Confidence:** HIGH -- bind pattern confirmed from `glWrapper.cpp` (5 TextureBind call sites) and `GameRenderer.cpp`.

---

### Pitfall 8: Single Frame-in-Flight Performance Bottleneck

**What goes wrong:** The bootstrap renderer uses a single fence, two semaphores. CPU blocks (`vkWaitForFences`) until the GPU finishes the previous frame before recording the next. CPU and GPU never overlap.

**Why it happens:** Simpler to implement, sufficient for a bootstrap. But Minecraft's rendering involves significant CPU work (chunk rebuilds, entity updates, tesselation) that should overlap with GPU rendering.

**Consequences:** Framerate limited to `1 / (CPU_time + GPU_time)` instead of `1 / max(CPU_time, GPU_time)`. A frame taking 8ms CPU + 8ms GPU gives 62 FPS with single-buffering vs 125 FPS with double-buffering.

**Prevention:**
- Implement 2-3 frames in flight with per-frame synchronization sets.
- Duplicate per-frame resources: command buffers, uniform buffers, vertex staging buffers.
- The single mapped vertex buffer (`vertexBufferMapped_`) will cause data races with multiple frames in flight -- must be duplicated or use a ring buffer.

**Detection:** GPU utilization below 50%. CPU time dominated by `vkWaitForFences`.

**Phase relevance:** Phase 2-3, before terrain rendering makes the bottleneck acute.

**Confidence:** HIGH -- directly observed in `VulkanBootstrapAppRender.cpp` lines 432-448.

## Moderate Pitfalls

### Pitfall 9: Swapchain Recreation Fence Deadlock

**What goes wrong:** When the window is resized, if `vkAcquireNextImageKHR` returns `VK_ERROR_OUT_OF_DATE_KHR` after the fence was waited on and reset but before work is submitted, the fence is never signaled. Next frame's `vkWaitForFences` hangs forever.

**Why it happens:** The current code in `drawFrame()` waits on the fence, resets it, then acquires the image. If acquisition fails, it recreates the swapchain and returns -- but the fence was already reset and no work was submitted.

**Consequences:** Application hangs on window resize, minimize, or display change. Especially common on macOS where Retina scaling changes trigger swapchain invalidation.

**Prevention:**
- Move fence reset to AFTER successful image acquisition.
- Alternatively: only reset immediately before `vkQueueSubmit()`.
- Test by rapidly resizing the window during rendering.

**Detection:** Application freeze on resize/minimize/restore.

**Phase relevance:** Should be fixed in Phase 1 before relying on the renderer for development.

**Confidence:** HIGH -- well-documented pattern, current code has the bug.

**Sources:**
- [Vulkan swapchain recreation tutorial](https://vulkan-tutorial.com/Drawing_a_triangle/Swap_chain_recreation)

---

### Pitfall 10: Missing Depth Buffer in Render Pass

**What goes wrong:** The render pass has only a color attachment. `pDepthStencilState` is `nullptr` in `createGraphicsPipeline()`. Without depth testing, all geometry renders in submission order.

**Why it happens:** Bootstrap only needed colored triangles. Minecraft's 3D world requires depth for correct occlusion.

**Consequences:** 3D terrain is a mess of overlapping blocks with no occlusion. Sky renders over everything.

**Prevention:**
- Create depth image (`VK_FORMAT_D32_SFLOAT` preferred -- most broadly supported on MoltenVK/Metal). Query format support with `vkGetPhysicalDeviceFormatProperties()` before creation.
- Add depth attachment to render pass.
- Add `VkPipelineDepthStencilStateCreateInfo` with `depthTestEnable = VK_TRUE`.
- Recreate depth image when swapchain is recreated.
- Avoid `VK_FORMAT_D16_UNORM` (limited on Apple GPUs).

**Detection:** Overlapping geometry in wrong order. `StateSetDepthTestEnable()` calls have no effect.

**Phase relevance:** Must be added in Phase 2, before 3D scene rendering.

**Confidence:** HIGH -- directly observed in `VulkanBootstrapAppRender.cpp` lines 360-362.

---

### Pitfall 11: EngineVertex Color Packing Format Mismatch

**What goes wrong:** The `EngineVertex` color field is a packed uint32. The current `convertVertex()` unpacks it as `(color >> 24) & 0xff` = R, `(color >> 16) & 0xff` = G, etc. But `Tesselator::color(int r, int g, int b, int a)` packs as `(a << 24) | (r << 16) | (g << 8) | b` -- format is ARGB8. The current unpacking treats A as R, producing wrong colors.

**Why it happens:** The bootstrap was developed without examining the Tesselator's packing format.

**Prevention:** Correct the unpacking in the vertex shader (or conversion function):
- Alpha = `(color >> 24) & 0xFF`
- Red = `(color >> 16) & 0xFF`
- Green = `(color >> 8) & 0xFF`
- Blue = `color & 0xFF`

OR use `VK_FORMAT_B8G8R8A8_UNORM` in the vertex attribute description and let hardware do the swizzle.

**Detection:** Colors appear wrong -- red/blue channels swapped, or alpha and color channels mixed.

**Phase relevance:** Phase 2, when vertex format is finalized for shader-based rendering.

**Confidence:** HIGH -- confirmed from `Tesselator.cpp` color packing code.

---

### Pitfall 12: Thread-Local Tesselator vs Vulkan Thread Safety

**What goes wrong:** The Tesselator uses thread-local storage (`TlsAlloc`/`TlsGetValue`). Background chunk rebuild threads each have their own Tesselator. If multiple threads submit vertices to the Vulkan backend, the `frameVertices_` vector in `VulkanBootstrapApp` has no synchronization.

**Why it happens:** The original engine had background threads produce vertex data consumed by the main thread's draw calls. In the current Vulkan path, `DrawVertices` -> `submitVertices` -> `frameVertices_.push_back()` is unsynchronized.

**Prevention:**
- Ensure all `submitVertices()` calls happen on the main thread only.
- Background threads should only accumulate vertex data in per-thread buffers (the CBuff system), with the main thread consuming the results.
- If multithreaded command recording is desired later, use per-thread command pools.

**Detection:** Sporadic crashes in `submitVertices()`. Corrupted vertex data. Race conditions detectable with ThreadSanitizer.

**Phase relevance:** Phase 3 (when multithreaded chunk rebuilding becomes relevant).

**Confidence:** MEDIUM -- threading confirmed from code, but macOS thread stubs may prevent background threading currently.

---

### Pitfall 13: Texture Format RGBA vs BGRA Confusion

**What goes wrong:** The game's texture format `TEXTURE_FORMAT_RxGyBzAw` suggests RGBA ordering. The swapchain uses `VK_FORMAT_B8G8R8A8_UNORM` (BGRA). If texture VkImages use BGRA format but data is RGBA (or vice versa), red and blue channels swap.

**Prevention:**
- Use `VK_FORMAT_R8G8B8A8_UNORM` for texture VkImages (matching the game's RGBA data).
- Swapchain format (BGRA) is separate from texture format -- they do not need to match.
- Be aware of sRGB vs linear: `_SRGB` formats apply gamma correction. Test with both and compare against original game screenshots.
- Verify actual byte order in `BufferedImage::loadData()` at runtime.

**Detection:** Grass appears blue, water appears red/orange. Washed-out or too-dark appearance (sRGB mismatch).

**Phase relevance:** Phase 2 (texture pipeline).

**Confidence:** MEDIUM -- byte ordering from code analysis, exact in-memory format needs runtime verification.

---

### Pitfall 14: Render Pass Load/Store Operations on Tile-Based GPUs

**What goes wrong:** Using `VK_ATTACHMENT_LOAD_OP_LOAD` when `VK_ATTACHMENT_LOAD_OP_CLEAR` is sufficient wastes bandwidth on tile-based GPUs, which is what Apple Silicon uses via Metal.

**Prevention:** Always use `VK_ATTACHMENT_LOAD_OP_CLEAR` for both color and depth at frame start. Only use `LOAD` if you genuinely need to preserve content from a previous render pass. Use `VK_ATTACHMENT_STORE_OP_DONT_CARE` for depth if it is not read after the render pass.

**Detection:** Lower-than-expected frame rates on Apple Silicon. Metal GPU profiler shows unnecessary tile memory loads.

**Phase relevance:** Phase 2-3, when render pass structure is finalized.

**Confidence:** HIGH -- Apple Silicon GPU architecture is well-documented.

## Minor Pitfalls

### Pitfall 15: Shader Compilation Pipeline Missing

**What goes wrong:** The bootstrap loads pre-compiled SPIR-V from hardcoded paths. No infrastructure exists for compiling GLSL to SPIR-V at build time for multiple shader variants.

**Prevention:**
- Add glslangValidator or glslc to CMake to compile `.vert`/`.frag` to `.spv` at build time.
- Design shader variant system early: minimum variants needed are color-only, textured, textured+alpha-test, textured+fog.
- Use specialization constants to reduce variant count -- one shader with specialization constants for fog/alpha-test/lighting.
- Bundle compiled SPIR-V into the macOS .app bundle for relocatability.

**Phase relevance:** Phase 2 (shader infrastructure).

**Confidence:** HIGH.

---

### Pitfall 16: Texture ID Zero Collision

**What goes wrong:** `TextureCreate()` returns 0 (hardcoded stub). All textures get the same ID. The game uses `glId == -1` as "no texture" so ID 0 is valid.

**Prevention:** Implement `TextureCreate()` as an incrementing counter (matching the original engine pattern). Map each ID to its Vulkan texture struct.

**Phase relevance:** Phase 2 (immediate, before any other texture work).

**Confidence:** HIGH -- directly observed.

---

### Pitfall 17: MoltenVK Geometry Shader Unavailability

**What goes wrong:** Metal has no geometry shader stage. `VkPhysicalDeviceFeatures::geometryShader = VK_FALSE` on MoltenVK. Tessellation is partially supported but `tessellationIsolines` and `tessellationPointMode` are unsupported.

**Prevention:** Do not plan features requiring geometry shaders. Use compute shaders or vertex instancing instead. The MCE codebase does not use geometry shaders, so this only matters for new features.

**Phase relevance:** Not blocking current phases. Flag for future planning.

**Confidence:** HIGH -- confirmed by [MoltenVK geometry shader issue #1524](https://github.com/KhronosGroup/MoltenVK/issues/1524).

---

### Pitfall 18: Missing Animated Texture Sub-Region Updates

**What goes wrong:** The animated texture system calls `glTexSubImage2D` (maps to `RenderManager.TextureDataUpdate()`) to update regions of the terrain atlas. Currently a no-op. Without it, water, lava, fire, and portal textures are static.

**Prevention:** Implement `TextureDataUpdate()` using `vkCmdCopyBufferToImage` with a sub-region `VkBufferImageCopy`. Requires image transition to `VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL` before copy, then back to `VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL`. Perform in the main command buffer before the render pass -- no separate submission.

**Phase relevance:** Phase 3-4, after basic textures work.

**Confidence:** HIGH.

---

### Pitfall 19: Validation Layers Not Enabled by Default

**What goes wrong:** Development without validation layers means API usage errors, sync hazards, and resource lifetime bugs go undetected until they manifest as mysterious artifacts or crashes.

**Prevention:**
- Enable `VK_LAYER_KHRONOS_validation` in debug builds.
- Enable synchronization validation for hazard detection.
- Set `MVK_CONFIG_DEBUG = 1` for Metal-level validation.
- Treat validation errors as build-breaking.

**Phase relevance:** Phase 1 -- enable immediately.

**Confidence:** HIGH.

## Phase-Specific Warnings

| Phase Topic | Likely Pitfall | Mitigation |
|-------------|---------------|------------|
| Phase 1: Bootstrap stabilization | Swapchain fence deadlock on resize (Pitfall 9) | Fix fence reset ordering |
| Phase 1: Bootstrap stabilization | No validation layers (Pitfall 19) | Enable VK_LAYER_KHRONOS_validation |
| Phase 2: Shader/UBO infrastructure | CPU-side MVP must move to shaders (Pitfall 2) | Push constants for matrices |
| Phase 2: Shader/UBO infrastructure | NDC Y-flip and depth range (Pitfall 4) | Choose one approach, apply everywhere |
| Phase 2: Shader/UBO infrastructure | Pipeline state explosion (Pitfall 1) | Use Vulkan 1.3 dynamic state + specialization constants |
| Phase 2: Shader/UBO infrastructure | Shader compilation pipeline (Pitfall 15) | Add glslangValidator to CMake |
| Phase 2: Shader/UBO infrastructure | Missing depth buffer (Pitfall 10) | Add depth attachment, query format support |
| Phase 2: Texture pipeline | Texture stubs discard all data (Pitfall 3) | Implement as cohesive unit, batch uploads |
| Phase 2: Texture pipeline | Format mismatch RGBA vs BGRA (Pitfall 13) | Use VK_FORMAT_R8G8B8A8 for textures |
| Phase 2: Texture pipeline | Descriptor set management (Pitfall 7) | Push descriptors or single set with update-on-bind |
| Phase 2: Texture pipeline | Texture ID zero collision (Pitfall 16) | Incrementing counter |
| Phase 2: Texture pipeline | Vertex color unpacking (Pitfall 11) | Fix ARGB8 unpack order |
| Phase 3: Terrain rendering | Vertex budget overflow (Pitfall 2) | Ring buffer, 4MB+ per frame |
| Phase 3: Terrain rendering | CBuff misunderstood as command buffers (Pitfall 6) | Implement as stored vertex arrays |
| Phase 3: Terrain rendering | Thread safety of vertex submission (Pitfall 12) | Main-thread-only draw calls |
| Phase 3: Terrain rendering | Single frame in flight (Pitfall 8) | 2-3 frames in flight with per-frame resources |
| Phase 3: Terrain rendering | Triangle fan topology (Pitfall 5) | Maintain conversion in DrawVertices |
| Phase 3: Terrain rendering | Tile-based GPU efficiency (Pitfall 14) | Use CLEAR load op, DONT_CARE store for depth |
| Phase 4: Entity/UI rendering | Alpha test in shader (Pitfall 1) | Fragment shader `discard`, specialization constant |
| Phase 4: Entity/UI rendering | Animated texture updates (Pitfall 18) | Sub-region vkCmdCopyBufferToImage |
| Future features | No geometry shaders (Pitfall 17) | Use compute or vertex instancing |

## Sources

- Direct codebase analysis of `VulkanRenderManager.cpp`, `VulkanBootstrapAppRender.cpp`, `Tesselator.cpp`, `Texture.cpp`, `glWrapper.cpp`, `4J_Render.h`
- [MoltenVK GitHub repository](https://github.com/KhronosGroup/MoltenVK)
- [MoltenVK triangle fan issue #1419](https://github.com/KhronosGroup/MoltenVK/issues/1419)
- [MoltenVK geometry shader issue #1524](https://github.com/KhronosGroup/MoltenVK/issues/1524)
- [Vulkan common pitfalls (official documentation)](https://docs.vulkan.org/guide/latest/common_pitfalls.html)
- [Vulkan NDC coordinate differences (KDAB)](https://www.kdab.com/projection-matrices-with-vulkan-part-1/)
- [Why OpenGL projection matrices fail in Vulkan](https://johannesugb.github.io/gpu-programming/why-do-opengl-proj-matrices-fail-in-vulkan/)
- [Vulkan frames in flight tutorial](https://docs.vulkan.org/tutorial/latest/03_Drawing_a_triangle/03_Drawing/03_Frames_in_flight.html)
- [Vulkan swapchain recreation](https://vulkan-tutorial.com/Drawing_a_triangle/Swap_chain_recreation)
- [D3D12 and Vulkan: Lessons Learned (AMD GPUOpen)](https://gpuopen.com/download/d3d12_vulkan_lessons_learned.pdf)
- [VK_KHR_portability_subset extension](https://registry.khronos.org/vulkan/specs/latest/man/html/VK_KHR_portability_subset.html)
- [The State of Vulkan on Apple (LunarG, Jan 2026)](https://www.lunarg.com/the-state-of-vulkan-on-apple-jan-2026/)
- [NVIDIA Vulkan Dos and Don'ts](https://developer.nvidia.com/blog/vulkan-dos-donts/)
- [Vulkan staging buffer patterns](https://docs.vulkan.org/tutorial/latest/04_Vertex_buffers/02_Staging_buffer.html)

---

*Pitfalls audit: 2026-03-08*
