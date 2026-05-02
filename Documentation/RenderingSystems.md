## Rendering Systems

**Location**: `Include/Aquila/Rendering/`, `Source/Aquila/Rendering/`

### RenderPipeline

The per-frame orchestrator. Owns a `RenderGraph`, a list of renderers, and the primary render targets.

```cpp
class RenderPipeline {
    RenderPipeline(GfxContext&, uint32 width, uint32 height);

    template<typename T, typename... Args>
    T& Add(Args&&...);         // creates T, calls T::OnInit(ctx), stores in m_Renderers

    void Render(GfxCommandList&, Scene&, f32 deltaTime);
    void Resize(uint32 width, uint32 height);

    GfxTexture& GetOutput(); // returns scene color texture

private:
    void RebuildTargets();   // reset + recreate SceneColor (RGBA16F) and Depth (Depth32)
    void BuildFrameContext(Scene&, f32 dt, FrameContext& out);

    GfxContext& m_Ctx;
    RenderGraph m_Graph;
    Ref<GfxTexture> m_SceneColor;   // RGBA16F, ColorAttachment | Sampled
    Ref<GfxTexture> m_DepthTex;     // Depth32, DepthAttachment
    uint32 m_Width, m_Height;
    std::vector<Unique<IRenderer>> m_Renderers;
};
```

`RebuildTargets()` explicitly calls `.reset()` on the old refs before allocating new textures, ensuring old resources are queued for deferred deletion before new VRAM is consumed.

### IRenderer

```cpp
class IRenderer {
    virtual void OnInit(GfxContext&) {}
    virtual void AddPasses(RenderGraph&, const FrameContext&) = 0;
    virtual void AddFinalPasses(RenderGraph&, const FrameContext&) {}
    virtual void OnResize(uint32 w, uint32 h) {}
    virtual void OnShutdown() {}
};
```

`AddPasses` declares the main geometry/lighting/etc. passes. `AddFinalPasses` is called after all renderers have added their main passes — used for blit/post-process operations that must run last (e.g., swapchain blit).

### Renderer (3D)

Owns a collection of `IRenderingSystem` instances. Delegates `AddPasses` to each system in order. Handles the swapchain blit in `AddFinalPasses` using a fullscreen triangle pipeline.

```cpp
class Renderer : public IRenderer {
    template<typename T, typename... Args>
    T& AddSystem(Args&&...);  // creates T, calls T::OnInit(ctx, device)
    void AddPasses(RenderGraph&, const FrameContext&) override;
    void AddFinalPasses(RenderGraph&, const FrameContext&) override;

private:
    std::vector<Unique<IRenderingSystem>> m_Systems;
    Ref<GfxPipeline> m_BlitPipeline;
    Ref<GfxDescriptorSet> m_BlitDescriptorSet;
    Ref<GfxSwapchain>* m_Swapchain;
    uint32 m_SwapchainImageIndex;
};
```

### IRenderingSystem

```cpp
class IRenderingSystem {
    virtual void OnInit(GfxContext&, Device&) = 0;
    virtual void AddPasses(RenderGraph&, const FrameContext&) = 0;
    virtual void OnResize(uint32 w, uint32 h) {}
    virtual void OnShutdown() {}
};
```

### RenderingSystemBase

Extends `IRenderingSystem` with mesh caching and descriptor management:

- Lazy mesh upload: first time a `Mesh*` is seen, uploads vertex/index data to GPU
- Per-scene descriptor set management: `AllocateDescriptorSetsForScene()`, `UpdateDescriptorsForScene()`
- Dirty tracking: `needsUpdate[frameIndex]` prevents redundant descriptor writes
- Helper writers: `BindUniformBuffer()`, `BindTexture()`, `BindTextureArray()`

### Rendering Systems

| System | Responsibility |
|--------|---------------|
| `DepthPrepassSystem` | Early-Z pass; writes depth buffer to reduce overdraw in later passes |
| `GeometrySystem` | Opaque mesh rendering; populates G-buffer or writes directly to scene color |

### Renderer2D

Independent renderer for 2D sprites and quads. Wraps `QuadBatcher`. Adds a single pass that draws all batched 2D geometry.

### FrameContext

Passed to every renderer and system for the current frame:

```cpp
struct FrameContext {
    Scene* scene;
    uint32 width, height;
    f32 deltaTime;
    vec3 cameraPosition;
    mat4 view, projection, viewProjection;
    RGTextureHandle hSceneColor; // RGBA16F render target
    RGTextureHandle hDepth; // Depth32 render target
};
```

### QuadBatcher

Immediate-mode 2D batching system. All quads added within a `Begin()`/`End()` pair are sorted and submitted in minimal draw calls.

```cpp
class QuadBatcher {
    void Begin(GfxCommandList&);
    void DrawRect(const RectSpec&); // solid colored rectangle
    void DrawSprite(const SpriteSpec&); // textured quad
    void Flush(); // submit accumulated quads
    void End();
    QuadBatchStats GetStats(); // quad count, draw call count
};
```

Pipelines are cached per format-combo (color format × depth format × sample count) so `Begin()` with different attachment configs is supported without recreation.

WIP
