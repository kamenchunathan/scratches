# General design goals

The renderer for this game is intended to be a 2d deferred renderer with a CPU rasterizer and outputing colors to the terminal (or web) as pixels represented by utf8 half-block characters using forground and background colors which allows doubling the vertical resolution. Other characters for effects and UI are also supported. Using graphics libraries was considered but requiring linking graphics libraries for a terminal application seemed unnecessary, so I went the tedious route of essentially mimmicking graphics APIs provided by GPUs. This was primarily because of farmiliarity with their design

The renderer is independent of the ECS and should be usable separately. The application runs systems that can call the renderer to issue render commands

The early steps of the renderer are similar to modern graphics pipelines, modelled after webgpu. The novel part of this is that "Shader programs" render to two buffers, a pixel buffer and a character buffer which is half the height of the pixel buffer. This is because pixels are rendered using half-blocks and foregrnound and background colors inspired by this [Jelle Pelgrims post](https://jellepelgrims.com/posts/devlog/1-terminal-as-canvas) which doubles the vertical resolution of the canvas. This allows rendering text for UI and other rendering techniques explored earlier e.g. using braille characters or rendering certian objects as characters specifically for styllistic reasons. A mask is used to mask off the characters from the character buffer from being rendered. Allowing for ordering between these layers

The presentation step which outputs to the terminal and in future to a DOM, then combines and diffs the front and back buffer to generate ansi terminal commands


---

## 1. Core Infrastructure
- [ ] **Render Graph System**
  - [x] Implement graph node abstraction (inputs, outputs, dependencies)
  - [x] Implement graph scheduler (topological sort for execution order)
  - [x] Support multi-pass execution
  - [x] Allow dynamic registration of passes at runtime
- [ ] **Framebuffer Management**
  - [ ] Define `Framebuffer` abstraction (attachments, formats, size)
  - [ ] Implement creation of framebuffers for different passes (G-buffer, lighting, presentation)
  - [ ] Add resize support (e.g. when terminal resolution changes)
- [ ] **Resource Binding**
  - [ ] Define binding model for:
    - [ ] Textures
    - [ ] Buffers (uniform, vertex, index)
    - [ ] Samplers
  - [ ] Implement per-pass binding logic (inputs/outputs)
- [ ] **Shader Program Abstraction**

  - [ ] Define shader stages (vertex-like, fragment-like, compute-like)
  - [ ] Support compile-time type-checking of shader interfaces
  - [ ] Implement a runtime registry for shader programs
  - [ ] Add system for hot-reloading shaders (optional, debugging feature)

## 2. Rasterization & Primitive Handling

- [ ] **Rasterizer Core**
  - [ ] Implement support for **points**
  - [ ] Implement support for **lines**
  - [x] Implement support for **triangles**
  - [x] Implement fragment interpolation (barycentric coordinates, etc.)
- [ ] **Coverage Functions**
  - [ ] Implement `covers()` for point-in-primitive tests
  - [ ] Implement `interpolate()` for per-fragment attributes
- [ ] **Pipeline Stages**
  - [x] Vertex processing stage (transform vertices → screen space)
  - [x] Primitive assembly (points, lines, triangles) - only for triangles
  - [x] Rasterization (convert primitives → fragments)
  - [x] Fragment shading (run bound shader program)

## 3. Deferred Shading Pipeline

- [ ] **G-Buffer Pass**
  - [ ] Define framebuffer attachments: albedo, normals, depth
  - [ ] Implement sprite rasterization into G-buffer
  - [ ] Add alpha masking (discard transparent pixels)
- [ ] **Lighting Pass**

  - [ ] Implement additive blending of multiple lights
  - [ ] Implement light intensity functions (falloff, angle, etc.)
  - [ ] Use G-buffer normals for per-fragment lighting
  - [ ] Add volumetric light factor
- [ ] **(TODO: Shadows)**

  - [ ] Add stencil buffer support
  - [ ] Implement shadow caster meshes
  - [ ] Implement stencil-based light masking
- [ ] **(TODO: Transparency)**

  - [ ] Investigate per-pixel linked lists (order-independent transparency)
  - [ ] Provide fallback (sprite inherits underlying normals)

## 4. Parallelization & SIMD Optimization

- [ ] **Task Parallelism**

  - [ ] Split render graph passes across threads
  - [ ] Ensure synchronization at pass boundaries
- [ ] **Data Parallelism**

  - [ ] Parallelize rasterization loops across scanlines/fragments
  - [ ] Parallelize lighting accumulation across pixels
- [ ] **SIMD Optimization**

  - [ ] Implement SIMD versions of common math ops (dot, normalize, etc.)
  - [ ] Use SIMD for coverage tests (batch 4–8 pixels at once)
  - [ ] Expose optional SIMD intrinsics to advanced users

## 5. Integration & ECS Independence

- [ ] **Renderer Independence**
  - [ ] Ensure renderer takes only raw buffers, not ECS entities
  - [ ] Define `Sprite`/`Mesh` components that know how to submit themselves
- [ ] **Data Extraction**
  - [ ] Implement system that packs ECS data (positions, textures, lights) into optimized buffers
  - [ ] Pass references of packed buffers to renderer

### 5.1. Presentation Layer

- [x] Terminal Presenter Initialization/Deinitialization
  - [x] Terminal state hanlding, changing and resetting
  - [x] Set terminal to raw mode for input handling
- [x] **Basic Frame Presentation (Cursor-based)**
  - [x] Clear screen and set cursor to home position
  - [x] Iterate through frame buffer, applying ANSI colors and printing characters
  - [x] Use cursor positioning commands instead of newlines
- [ ] **Redisplay Algorithm (Diffing)**
  - [ ] Implement double-buffering (current vs. previous frame)
  - [ ] Implement line-by-line diffing (e.g., Myers diff)
  - [ ] Implement character-by-character diffing within changed lines
  - [ ] Generate optimized ANSI escape sequences for updates (e.g., cursor movement, color changes, character writes)
  - [ ] Map pixel colors → terminal glyphs (presentation phase only)
- [ ] **Web Presenter (Future)**
  - [ ] Implement DOM manipulation for web output
  - [ ] Optimize updates for web (e.g., virtual DOM, minimal re-renders)

## 6. Testing & Debugging Infrastructure

- [ ] **Unit Tests**
  - [ ] Test coverage functions (point-in-triangle, etc.)
  - [ ] Test barycentric interpolation
- [ ] **Render Graph Tests**
  - [ ] Validate correct dependency resolution
  - [ ] Validate framebuffer attachment consistency
- [ ] **Performance Tools**
  - [ ] Add debug overlay (timings per pass)
  - [ ] Integrate cache-miss inspection tooling (perf, VTune, etc.)
  - [ ] Add optional debug views of intermediate buffers (albedo, normals, etc.)

## 7. Stretch Goals

- [ ] **Advanced Effects**
  - [ ] Depth of field
  - [ ] Light shafts / god rays
  - [ ] Post-processing pipeline
- [ ] **Extensibility**
  - [ ] Plugin system for new primitives (circles, polygons, text meshes)
  - [ ] Compute-like shaders for particle effects
