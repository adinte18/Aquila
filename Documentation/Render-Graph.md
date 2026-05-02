# Render Graph

**Location**: `Include/Aquila/Graphics/RenderGraph/`, `Source/Aquila/Graphics/RenderGraph/`

The render graph separates pass *declaration* from pass *execution*. Systems describe what resources they read and write; the graph works out barriers, resource lifetimes, and memory aliasing automatically.

For code snippets and usage patterns, see [RenderGraph Usage Guide](RenderGraphSnippets).

---

## Concepts

| Term | Meaning |
|------|---------|
| **Transient resource** | Created and managed by the graph; freed after `Execute()` |
| **Imported resource** | External — e.g., swapchain image or `RenderPipeline` scene targets |
| **Handle** | Opaque typed `uint32` — `RGTextureHandle` or `RGBufferHandle` |
| **Version** | Sub-index encoded in the handle's top 8 bits; incremented on each write |
| **Side-effect pass** | Kept alive by the culling step even with no tracked outputs (e.g., swapchain blit) |

---

## API

### RenderGraph (`RGGraph.h`)

```cpp
class RenderGraph {
    // Resource declaration
    RGTextureHandle DeclareTexture(const RGTextureDesc&);
    RGBufferHandle  DeclareBuffer(const RGBufferDesc&);
    RGTextureHandle ImportTexture(GfxTexture*, std::string name);
    RGBufferHandle  ImportBuffer(GfxBuffer*, std::string name);

    // Pass registration
    // setupFn:   called immediately (sync) — receives RGPassBuilder&
    // executeFn: captured, called during Execute()
    void AddPass(std::string name,
                 Delegate<void(RGPassBuilder&)> setupFn,
                 Delegate<void(GfxCommandList&, RGRegistry&)> executeFn);

    void Compile(GfxContext&);     // build execution schedule
    void Execute(GfxCommandList&); // replay compiled schedule
    void Reset();                  // release transient resources
};
```

### RGPassBuilder (`RGPassBuilder.h`)

Called inside `setupFn`. Declares resource accesses for a single pass.

```cpp
class RGPassBuilder {
    RGTextureHandle ReadTexture(RGTextureHandle, ResourceState);
    RGTextureHandle WriteTexture(RGTextureHandle, ResourceState); // returns new versioned handle

    RGBufferHandle ReadBuffer(RGBufferHandle, ResourceState);
    RGBufferHandle WriteBuffer(RGBufferHandle, ResourceState);    // returns new versioned handle

    RGTextureHandle SetColorAttachment(uint32 slot, RGTextureHandle,
                                       AttachmentLoadOp, AttachmentStoreOp,
                                       ClearColor clear = {});
    RGTextureHandle SetDepthAttachment(RGTextureHandle,
                                       AttachmentLoadOp depthLoad,
                                       AttachmentStoreOp depthStore,
                                       AttachmentLoadOp stencilLoad,
                                       AttachmentStoreOp stencilStore,
                                       bool readOnly = false,
                                       ClearDepth clear = {});

    void MarkAsSideEffect(); // prevent culling even with no live tracked outputs
};
```

Write calls and `SetColorAttachment` return a **new versioned handle**. The caller must store and use the returned handle for any downstream pass — this is how data hazards are tracked.

### RGRegistry (`RGRegistry.h`)

Passed to `executeFn` at execution time. Resolves handles to actual GPU resources:

```cpp
class RGRegistry {
    GfxTexture& GetTexture(RGTextureHandle);
    GfxBuffer&  GetBuffer(RGBufferHandle);
};
```

Physical resources are only valid inside an execute lambda — never cache them across frames.

---

## Handle Versioning

Handles encode both a **slot index** (bottom 24 bits) and a **version** (top 8 bits). Every write produces a new version on the same slot, so both the old and new handle point at the same physical memory, but the graph sees them as distinct nodes in the dependency DAG.

```
hColor_v0 = graph.ImportTexture(sceneColor)          // imported, version 0
hColor_v1 = builder.SetColorAttachment(0, hColor_v0) // geometry writes, version 1
hColor_v2 = builder.SetColorAttachment(0, hColor_v1) // lighting writes, version 2
hColor_v3 = builder.WriteTexture(hColor_v2, ...)     // composite writes, version 3
```

Passing a stale handle (e.g., `hColor_v1` to a pass that needs `hColor_v2`) creates a missing dependency edge — the compiler will either cull the pass or emit barriers in the wrong order.

---

## Compilation Pipeline

`RGCompiler::Compile(passes, registry, ctx)` produces an `RGCompiledGraph` in seven steps:

1. **Dependency Graph** will build adjacency list from versioned handle producer/consumer pairs
2. **Topological Sort** is DFS-based, detects and reports cycles
3. **Culling** will remove passes with no live outputs (side-effect passes survive)
4. **Lifetime Analysis** will analyse each resource, find first-use and last-use pass indices
5. **Resource Aliasing** will reuse transient allocations with non-overlapping lifetimes (helps reduce peak VRAM)
6. **Barrier Inference** will emit `ResourceState` transitions between each pair of consecutive accesses
7. **RenderPass Creation** will create renderpasses from color + depth attachment sequences

### Compiled Output (`RGCompiledGraph`)

```cpp
struct RGCompiledGraph {
    bool valid;
    std::vector<uint32> passOrder; // culled, sorted pass indices
    std::vector<RGTexBarrier> texBarriers; // flat SoA
    std::vector<uint32> passTexBarStart; // per-pass offset into texBarriers
    std::vector<RGBufBarrier> bufBarriers;
    std::vector<uint32> passBufBarStart;
    std::vector<Ref<GfxRenderPass>> passRenderPasses; // null for compute/copy passes
    std::vector<Ref<GfxTexture>> transientTextures;  // kept alive through Execute()
    std::vector<Ref<GfxBuffer>> transientBuffers;
};
```

---

## Execution

`RenderGraph::Execute(cmd)` replays `passOrder`:

1. Emit texture barriers from `texBarriers[passTexBarStart[i] .. passTexBarStart[i+1]]`
2. Emit buffer barriers from `bufBarriers[passBufBarStart[i] .. passBufBarStart[i+1]]`
3. If `passRenderPasses[i]` is non-null: begin renderpass
4. Call `pass.executeFn(cmd, registry)`
5. End renderpass if active

---

## Resource Aliasing

Transient textures with non-overlapping lifetimes share the same underlying image allocation. The compiler builds a pool of compatible allocations and assigns them by first-fit. Two descriptors are compatible if they have the same format, usage flags, sample count, and dimensions (within tolerance).

This means a texture N, alive only during passes 2–4 and a texture M alive only during passes 6–8 can share VRAM, even though they're different logical resources.
