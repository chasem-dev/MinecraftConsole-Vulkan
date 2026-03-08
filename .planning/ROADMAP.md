# Roadmap: MCE Vulkan Port — Rendering Proof

## Overview

This roadmap takes the MCE Vulkan renderer from its current state (colored rectangles on a main menu) to a fully textured, lit, terrain-rendering game on macOS. The progression follows the rendering pipeline's own dependency chain: stabilize the foundation and build shader infrastructure, then load and display textures, then add all the render state and terrain systems needed for a correct-looking world, and finally scale the vertex pipeline to handle full scenes without crashing.

## Phases

**Phase Numbering:**
- Integer phases (1, 2, 3): Planned milestone work
- Decimal phases (2.1, 2.2): Urgent insertions (marked with INSERTED)

Decimal phases appear between their surrounding integers in numeric order.

- [ ] **Phase 1: Renderer Foundation** - Stabilize bootstrap, add depth buffer, build shader infrastructure with GPU-side transforms
- [ ] **Phase 2: Texture Pipeline** - Load textures from archives, bind them in Vulkan, display textured geometry
- [ ] **Phase 3: Visual Correctness** - Render state tracking, pipeline caching, terrain chunk system, lighting, and fog produce a correct-looking world
- [ ] **Phase 4: Performance Scaling** - Dynamic vertex buffers and multi-frame-in-flight handle full scene complexity

## Phase Details

### Phase 1: Renderer Foundation
**Goal**: The renderer is stable, debuggable, and renders correct 3D geometry with depth testing and GPU-side transforms -- ready for textures
**Depends on**: Nothing (first phase)
**Requirements**: BOOT-01, BOOT-02, BOOT-03, SHDR-01, SHDR-02, SHDR-03
**Success Criteria** (what must be TRUE):
  1. Window can be resized and restored without the renderer hanging or deadlocking
  2. Vulkan validation layer output appears in the console identifying API usage errors
  3. 3D geometry renders with correct depth ordering (nearer objects occlude farther ones)
  4. Colored geometry displays accurate colors matching the original game's palette
  5. Multiple shader pipelines exist and can be selected at runtime (color-only, textured, textured+alpha-test, textured+fog)
**Plans:** 2 plans

Plans:
- [ ] 01-01-PLAN.md -- Bootstrap stabilization: fix fence deadlock, enable validation layers, verify color correctness
- [ ] 01-02-PLAN.md -- Depth buffer, GPU-side MVP transforms, shader variant system with 4 pipeline variants

### Phase 2: Texture Pipeline
**Goal**: Textures load from game archives, upload to GPU, bind in shaders, and display correctly -- the game transitions from colored shapes to recognizable Minecraft visuals
**Depends on**: Phase 1
**Requirements**: TEX-01, TEX-02, TEX-03
**Success Criteria** (what must be TRUE):
  1. Main menu renders with its textures visible (background panorama, buttons, logo)
  2. Textures display with blocky NEAREST filtering matching the original Minecraft look
  3. The renderer handles the full init-time texture load (120+ textures) without crashing or exhausting descriptor sets
**Plans**: TBD

Plans:
- [ ] 02-01: TBD

### Phase 3: Visual Correctness
**Goal**: Transparent surfaces, alpha-tested foliage, fog, lighting, terrain chunks, and animated textures all render correctly -- the world looks right
**Depends on**: Phase 2
**Requirements**: RSTATE-01, RSTATE-02, RSTATE-03, RSTATE-04, RSTATE-05, TERR-01, TERR-02, VIS-01, VIS-02, VIS-03, VIS-04
**Success Criteria** (what must be TRUE):
  1. Transparent surfaces (water, glass) render with correct alpha blending and are see-through
  2. Foliage and cutout blocks (leaves, flowers, tall grass) render without black rectangles around them
  3. Terrain chunks load and display with block textures, and the world is navigable
  4. Entity models (mobs, player) render with textures and directional lighting
  5. Distance fog fades terrain into the background, and biome tinting colors grass and leaves correctly
**Plans**: TBD

Plans:
- [ ] 03-01: TBD
- [ ] 03-02: TBD
- [ ] 03-03: TBD

### Phase 4: Performance Scaling
**Goal**: The renderer handles full scene complexity without vertex buffer overflow or frame-rate bottlenecks
**Depends on**: Phase 3
**Requirements**: PERF-01, PERF-02
**Success Criteria** (what must be TRUE):
  1. Full terrain rendering (all visible chunks) does not crash from vertex budget overflow
  2. Frame rendering overlaps with CPU preparation, reducing stutter and improving throughput
**Plans**: TBD

Plans:
- [ ] 04-01: TBD

## Progress

**Execution Order:**
Phases execute in numeric order: 1 → 2 → 3 → 4

| Phase | Plans Complete | Status | Completed |
|-------|----------------|--------|-----------|
| 1. Renderer Foundation | 0/2 | Not started | - |
| 2. Texture Pipeline | 0/1 | Not started | - |
| 3. Visual Correctness | 0/3 | Not started | - |
| 4. Performance Scaling | 0/1 | Not started | - |
