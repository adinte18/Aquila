# RHI — Render Hardware Interface

**Location**: `Include/Aquila/RHI/Backend/` (interfaces), `Include/Aquila/RHI/Vulkan/` + `Source/Aquila/RHI/Vulkan/` (Vulkan backend)

The RHI layer provides a backend-agnostic GPU programming model. All interfaces are pure virtual. The Vulkan backend is the only current implementation, but the interface is designed for future DirectX 12 / Metal ports.

---

## IRHIDevice

The central factory and submission interface. Everything GPU-related flows through here.

```cpp
class IRHIDevice {
    // Factory
    Unique<IRHIBuffer> CreateBuffer(const BufferDesc&);
    Unique<IRHITexture> CreateTexture(const TextureDesc&);
    Unique<IRHICommandList> CreateCommandList(CommandListType, name);
    Unique<IRHISwapchain> CreateSwapchain(const SwapchainDesc&);
    Unique<IRHIRenderPass> CreateRenderPass(const RenderPassDesc&);
    Unique<IRHIPipeline> CreateGraphicsPipeline(const GraphicsPipelineDesc&);
    Unique<IRHIPipeline> CreateComputePipeline(const ComputePipelineDesc&);
    Unique<IRHIDescriptorSetLayout> CreateDescriptorSetLayout(const DescriptorSetLayoutDesc&);
    Unique<IRHIDescriptorSet> AllocateDescriptorSet(IRHIDescriptorSetLayout&);

    // Submission
    void SubmitFrame(IRHICommandList&, IRHISwapchain*, uint32 imageIndex);
    void SubmitAndWait(IRHICommandList&);

    // Synchronization
    void WaitIdle();
    void ProcessPendingDeletions();

    // Convenience
    template<typename Func>
    void ExecuteImmediate(CommandListType, Func&&);
};
```

All factory methods return `Unique<T>` — the caller takes exclusive ownership.

---

## IRHIBuffer

```cpp
class IRHIBuffer {
    void  Write(const void* data, uint64 size, uint64 offset = 0);
    void* Map();
    void  Unmap();
    void  Flush(uint64 size, uint64 offset);
    uint64 GetSize();
    uint32 GetInstanceCount();
    bool   IsMapped();
};
```

Supports instanced UBO patterns via `GetInstanceCount()` — the buffer stores N instances of a struct, and callers index into them with `WriteToIndex()` / `DescriptorInfoForIndex()` at the Vulkan level.

---

## IRHITexture

```cpp
class IRHITexture {
    uint32 GetWidth();
    uint32 GetHeight();
    uint32 GetMipLevels();
    uint32 GetArrayLayers();
    TextureFormat GetFormat();
    SampleCount GetSampleCount();
    const TextureDesc& GetDesc();
};
```

---

## IRHICommandList

Full GPU command recording API:

```cpp
class IRHICommandList {
    void Begin();
    void End();
    void Reset();
    bool IsRecording();

    // Barriers
    void TransitionTexture(IRHITexture&, ResourceState old, ResourceState new);
    void TransitionBuffer(IRHIBuffer&, ResourceState old, ResourceState new);

    // State
    void BindPipeline(IRHIPipeline&);
    void SetViewport(float x, float y, float w, float h, float minDepth, float maxDepth);
    void SetScissor(int32 x, int32 y, uint32 w, uint32 h);
    void BindDescriptorSet(IRHIDescriptorSet&, uint32 set, IRHIPipeline&);
    void PushConstants(const void* data, uint32 size, ShaderStage stages, uint32 offset);

    // Geometry
    void BindVertexBuffer(IRHIBuffer&, uint32 binding, uint64 offset);
    void BindIndexBuffer(IRHIBuffer&, uint64 offset, IndexType);
    void Draw(uint32 vertexCount, uint32 instanceCount, uint32 firstVertex, uint32 firstInstance);
    void DrawIndexed(uint32 indexCount, uint32 instanceCount, uint32 firstIndex, int32 vertexOffset, uint32 firstInstance);
    void DrawIndirect(IRHIBuffer& args, uint64 offset, uint32 drawCount, uint32 stride);
    void DrawIndexedIndirect(IRHIBuffer& args, uint64 offset, uint32 drawCount, uint32 stride);

    // Debug markers (visible in RenderDoc / Nsight)
    void PushDebugGroup(const char* name, vec4 color);
    void PopDebugGroup();
};
```

---

## IRHISwapchain

```cpp
class IRHISwapchain {
    bool AcquireNextImage(uint32& outImageIndex);
    void Present(uint32 imageIndex);
    bool NeedsResize();
    void Resize(uint32 width, uint32 height);
    uint32 GetWidth();
    uint32 GetHeight();
    TextureFormat GetFormat();
    uint32 GetImageCount();
};
```

`AcquireNextImage` returns false if the swapchain is out-of-date. The caller should defer to the next resize handling tick.

---

## RHI Types (`RHITypes.h`)

### MemoryDomain

Controls VMA allocation strategy:

| Domain | Usage |
|--------|-------|
| `GPU_ONLY` | Device-local, no CPU access. Fastest for rendering |
| `CPU_TO_GPU` | Host-visible + sequentially writable. Upload staging and dynamic UBOs |
| `GPU_TO_CPU` | Host-cached + random-accessible. Readback |
| `CPU_ONLY` | Host memory, no GPU optimization |

### TextureFormat

Full set: `RGBA8`, `RGBA8_SRGB`, `RGBA16F`, `RGBA32F`, `RGB8/16F/32F`, `RG8/16F/32F`, `R8/16F/32F`, `R32UI`, `BGRA8`, `BGRA8_SRGB`, `Depth16`, `Depth32`, `Depth24Stencil8`, `Depth32Stencil8`.

### TextureUsage (bitmask)

`ColorAttachment | DepthAttachment | Sampled | Storage | TransferSrc | TransferDst | InputAttachment`

### BufferUsage (bitmask)

`VertexBuffer | IndexBuffer | UniformBuffer | StorageBuffer | TransferSrc | TransferDst | IndirectBuffer`

### ResourceState (bitmask)

Used for explicit barrier tracking:

`Undefined`, `ColorAttachmentRead/Write`, `DepthStencilRead/Write`, `ShaderRead`, `StorageRead/Write`, `UniformRead`, `TransferSrc/Dst`, `IndirectArgument`, `IndexBuffer`, `VertexBuffer`, `Present`

### Key Descriptor Structs

**TextureDesc**:
```cpp
struct TextureDesc {
    uint32 width, height;
    TextureFormat format;
    TextureUsage  usage;
    uint32 mipLevels   = 1;
    uint32 arrayLayers = 1;
    SampleCount samples     = SampleCount::x1;
    SamplerDesc sampler;
    std::string debugName;
};
```

**GraphicsPipelineDesc**:
```cpp
struct GraphicsPipelineDesc {
    ShaderDesc vertexShader, fragmentShader;
    PrimitiveTopology topology;
    RasterizationState rasterization;
    DepthStencilState depthStencil;
    BlendState blend;
    std::vector<TextureFormat> colorFormats;
    TextureFormat depthFormat;
    SampleCount samples;
    std::vector<IRHIDescriptorSetLayout*> setLayouts;
    PushConstantRange pushConstants;
    std::string debugName;
};
```

**SamplerDesc** named presets:
- `SamplerDesc::Texture2D()` — linear filter, repeat wrap
- `SamplerDesc::RenderTarget()` — linear filter, clamp wrap
- `SamplerDesc::ShadowMap()` — depth comparison, clamp to border

---

## Vulkan Backend

### VulkanDevice

Extends `IRHIDevice`. Creates and owns:

- `VkInstance`, `VkPhysicalDevice`, `VkDevice`
- Four queues: graphics / present / compute / transfer
- `VmaAllocator`
- Descriptor pools, command pools (one per thread for graphics, shared for compute/transfer)

Key extras over the base interface:
- Templated allocation helpers: `CreateBuffer<MemoryDomain>()`, `CreateImage<MemoryDomain>()` — bypass `IRHIBuffer` and allocate raw handles for internal use
- `GetOrCreateThreadLocalGraphicsPool()` — returns a per-thread `VkCommandPool`, lock-free in steady state
- `SetObjectDebugName()` — Vulkan debug labels visible in RenderDoc / Nsight
- `SubmitFrame()` path: end-of-frame barriers → submit with semaphores → wait on per-frame fence → `ProcessDeletions()` → present

### VulkanSwapchain

Manages `VkSwapchainKHR`, pre-created image views, and per-image semaphore pairs (imageAvailable + renderFinished). Present detects `VK_SUBOPTIMAL_KHR` and sets the resize flag.

### VulkanBuffer

Wraps `VkBuffer` + `VmaAllocation`. Supports persistent CPU mapping for `CPU_TO_GPU` buffers, indexed writes for instanced UBOs, and `VkDescriptorBufferInfo` generation.

### VulkanTexture

Wraps `VkImage`, `VkImageView`, `VmaAllocation`. Sampler is retrieved from `VulkanDevice`'s sampler cache (deduplicated by `SamplerDesc`). Destructor calls `QueueDeletion()`.

### VulkanCommandList

Wraps `VkCommandBuffer` + its owning pool. Tracks recording state. Per-thread pool caching means allocation is lock-free in steady state.

### VulkanPipeline

Stores `VkPipeline` + `VkPipelineLayout`. Destructor queues both to the deletion queue.

---

## DeletionQueue

**Files**: `DeletionQueue.h`, `VulkanDeletionQueue.cpp`

Defers Vulkan resource destruction until the GPU is done with them. See [Key Patterns](Key-Patterns.md) for the full three-phase pattern.

```cpp
class DeletionQueue {
    void QueueDeletion(const Deletion::ResourceVariant&); // enqueue
    void ProcessDeletions(); // free all queued (no GPU wait)
    void Flush(); // WaitIdle() then ProcessDeletions()
};
```

`Deletion::ResourceVariant` covers: `VkPipeline`, `VkShaderModule`, `VkPipelineLayout`, `VkImageView`, `VkSampler`, `VkDescriptorSetLayout`, `VkRenderPass`, `VkFramebuffer`, `VkSemaphore`, `VmaImageDeletion`, `VmaBufferDeletion`, and more.

`ProcessDeletions()` is called automatically by `VulkanDevice::SubmitFrame()` after the per-frame fence wait — resources queued during the frame are freed before the next frame begins, without an extra `vkDeviceWaitIdle`.
