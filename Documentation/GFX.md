# GFX — Graphics Abstraction Layer

**Location**: `Include/Aquila/GFX/`, `Source/Aquila/GFX/`

The GFX layer is a thin C++ wrapper over [RHI](RHI.md). Its purpose is to provide ownership semantics and convenience without adding logic. Each GFX class holds a `Unique<IRHIType>` and delegates all operations to it.

Game and engine code should use GFX, not RHI directly.

---

## GfxContext

The primary API surface for all GPU work. Created once per window.

```cpp
class GfxContext {
    static Unique<GfxContext> Create(GLFWwindow&);

    // Resource creation — all return Ref<T> for shared ownership
    Ref<GfxBuffer>              CreateBuffer(const RHI::BufferDesc&);
    Ref<GfxTexture>             CreateTexture(const RHI::TextureDesc&);
    Ref<GfxSwapchain>           CreateSwapchain(const RHI::SwapchainDesc&);
    Ref<GfxPipeline>            CreateGraphicsPipeline(const RHI::GraphicsPipelineDesc&);
    Ref<GfxPipeline>            CreateComputePipeline(const RHI::ComputePipelineDesc&);
    Ref<GfxDescriptorSetLayout> CreateDescriptorSetLayout(const RHI::DescriptorSetLayoutDesc&);
    Ref<GfxDescriptorSet>       AllocateDescriptorSet(GfxDescriptorSetLayout&);
    Ref<GfxRenderPass>          CreateRenderPass(const RHI::RenderPassDesc&);
    Ref<GfxCommandList>         CreateCommandList(RHI::CommandListType, std::string name = {});

    // Operations
    void CopyBuffer(GfxBuffer& src, GfxBuffer& dst, uint64 size, uint64 srcOffset = 0, uint64 dstOffset = 0);
    void SubmitFrame(GfxCommandList&, GfxSwapchain* = nullptr, uint32 imageIndex = 0);
    void SubmitAndWait(GfxCommandList&);
    void WaitIdle();
    void ProcessPendingDeletions();

    // Execute a one-shot command list inline
    template<typename Func>
    void ExecuteImmediate(RHI::CommandListType, Func&&); // begin → func → end → submit+wait

    RHI::IRHIDevice& GetDevice(); // escape hatch to underlying RHI
};
```

**Ownership model**: `Create*` methods return `Ref<T>` (shared ownership). The device itself (`IRHIDevice`) is held as `Unique` — there is exactly one per `GfxContext`.

---

## GfxCommandList

Thin wrapper around `IRHICommandList`. All recording calls forward directly. Key extra is the typed push constant helper:

```cpp
class GfxCommandList {
    void Begin();
    void End();
    void Reset();
    bool IsRecording();

    void TransitionTexture(GfxTexture&, RHI::ResourceState old, RHI::ResourceState new);
    void TransitionBuffer(GfxBuffer&, RHI::ResourceState old, RHI::ResourceState new);

    void BindPipeline(GfxPipeline&);
    void SetViewport(float x, float y, float width, float height, float minDepth = 0, float maxDepth = 1);
    void SetScissor(int32 x, int32 y, uint32 width, uint32 height);
    void BindDescriptorSet(uint32 set, GfxDescriptorSet&);

    template<typename T>
    void PushConstants(const T& data, RHI::ShaderStageFlags stages, uint32 offset = 0);

    void BindVertexBuffer(GfxBuffer&, uint32 binding = 0, uint64 offset = 0);
    void BindIndexBuffer(GfxBuffer&, RHI::IndexFormat = UInt32, uint64 offset = 0);

    void Draw(uint32 vertexCount, uint32 instanceCount = 1, uint32 firstVertex = 0, uint32 firstInstance = 0);
    void DrawIndexed(uint32 indexCount, uint32 instanceCount = 1, uint32 firstIndex = 0, int32 vertexOffset = 0, uint32 firstInstance = 0);
    void DrawIndirect(GfxBuffer&, uint64 offset, uint32 drawCount, uint32 stride);
    void DrawIndexedIndirect(GfxBuffer&, uint64 offset, uint32 drawCount, uint32 stride);

    void PushDebugGroup(const char* name);
    void PopDebugGroup();
};
```

---

## GfxTexture

Holds `Unique<IRHITexture>`. Destruction queues GPU resources via the deletion queue (handled inside `VulkanTexture::~VulkanTexture()`).

When reallocating the same slot (e.g., on resize), explicitly call `.reset()` before creating the new texture. This ensures old VRAM is queued for release before new VRAM is consumed, preventing peak usage from doubling.

```cpp
// Correct resize pattern:
m_SceneColor.reset();                 // queues old texture for deferred deletion
m_SceneColor = ctx.CreateTexture({}); // allocates new texture
```

---

## GfxBuffer

Holds `Unique<IRHIBuffer>`. Provides the same write / map / unmap / flush interface as `IRHIBuffer`, plus typed write helpers.

---

## GfxPipeline

Holds `Unique<IRHIPipeline>`. No extra logic — just owns the pipeline handle and queues it to the deletion queue on destruction.

---

## GfxDescriptorSetLayout / GfxDescriptorSet

Thin wrappers. `GfxDescriptorSet` adds `UpdateBinding(uint32 slot, GfxTexture&)` and `UpdateBinding(uint32 slot, GfxBuffer&)` convenience methods that forward to the underlying RHI descriptor write path.
