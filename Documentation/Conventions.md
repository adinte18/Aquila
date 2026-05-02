# Coding Conventions

This document describes the conventions used throughout the Aquila Engine codebase.
All contributions are expected to follow these rules consistently.

---

## Table of Contents

1. [Naming](#naming)
2. [Types & Aliases](#types--aliases)
3. [File Structure](#file-structure)
4. [Classes & Interfaces](#classes--interfaces)
5. [Error Handling](#error-handling)
6. [GPU Resource Management](#gpu-resource-management)
7. [Vulkan-Specific Rules](#vulkan-specific-rules)
8. [Comments](#comments)
9. [Miscellaneous Patterns](#miscellaneous-patterns)

---

## Naming

### General rules

| Construct | Style | Example |
|-----------|-------|---------|
| Classes / structs / enums | `PascalCase` | `VulkanDevice`, `TextureDesc` |
| Interfaces (pure virtual base) | `I` prefix + `PascalCase` | `IRHIDevice`, `IRHITexture` |
| Member variables | `m_PascalCase` | `m_Device`, `m_DeletionQueue` |
| Methods | `PascalCase` | `CreateBuffer()`, `SubmitFrame()` |
| Parameters | `camelCase` | `submitInfo`, `imageIndex` |
| Local variables | `camelCase` | `cmdBuf`, `fence`, `poolInfo` |
| Template type parameters | Single uppercase letter | `T`, `Func`, `Args` |

### Specific conventions

**Interfaces** are always prefixed with `I`. If a class has a concrete Vulkan implementation alongside an abstract base, the base gets the prefix and the implementation gets the backend name:
```cpp
class IRHIBuffer { ... }; // abstract
class VulkanBuffer : IRHIBuffer { ... }; // concrete
```

**Boolean methods** are phrased as questions:
```cpp
bool IsMapped();
bool IsRecording();
bool HasActiveCamera();
```

**`Get` vs no prefix**: use `Get` when the method returns a stored value or handle. Omit it for computed properties that return new objects.
```cpp
VkDevice GetDevice(); // returns stored handle — Get prefix
VkFence  CreateFence(bool);   // creates something new — no Get
```

**Enum members** follow two sub-rules depending on usage:
- Scoped enums used as bitmasks: `PascalCase` members (`ColorAttachment`, `ShaderRead`, `TransferSrc`)
- Scoped enums representing distinct modes: `PascalCase` members (`Compute`, `Transfer`, `Graphics`)
- Constants that map to external API values (e.g. memory domains): `SCREAMING_SNAKE_CASE` (`GPU_ONLY`, `CPU_TO_GPU`)

---

## Types & Aliases

Always use the engine's primitive type aliases. **Never** use raw C++ arithmetic types directly.

```cpp
// Correct
uint32 width;
f32 deltaTime;
uint64 offset;

// Wrong
unsigned int width;
float deltaTime;
size_t offset;
```

Note : You might find wrongly used types in the codebase, this is fine (when I am on autopilot I forget about engine primitive types), and if you find some, change them! 

The full set of aliases is defined in `Foundation/PrimitiveTypes.h`:

```
uint8, uint16, uint32, uint64
int8,  int16,  int32,  int64
f32, f64
usize, isize, uptr, iptr
```

### Smart pointers

Always use the engine aliases. Never spell out `std::unique_ptr` or `std::shared_ptr` directly.

| Need | Use | Never write |
|------|-----|-------------|
| Exclusive ownership | `Unique<T>` | `std::unique_ptr<T>` |
| Shared ownership | `Ref<T>` | `std::shared_ptr<T>` |
| Non-owning observation | `WeakRef<T>` | `std::weak_ptr<T>` |

Similarly, always use the factory helpers:

```cpp
// Correct
return CreateUnique<VulkanBuffer>(*this, desc);
return CreateRef<GfxTexture>(std::move(rhi));

// Wrong
return std::make_unique<VulkanBuffer>(*this, desc);
return std::make_shared<GfxTexture>(std::move(rhi));
```

### When to use `Unique` vs `Ref`

- `Unique<T>` — one clear owner. Use for subsystems, RHI objects inside GFX wrappers, renderers inside `RenderPipeline`.
- `Ref<T>` — shared across systems. Use for GPU resources that multiple systems hold onto (meshes, textures, pipelines, descriptor sets).
- Raw pointer / reference — non-owning borrow only. The caller must guarantee the lifetime outlives the borrow. Document this at the call site if it is not obvious.

---

## File Structure

### Header guards

Use `#ifndef` guards. Do not use `#pragma once`.

```cpp
#ifndef AQUILA_RHI_VULKAN_TEXTURE_H
#define AQUILA_RHI_VULKAN_TEXTURE_H

// ...

#endif
```

The guard name follows `AQUILA_<SUBSYSTEM>_<FILENAME>_H` in `SCREAMING_SNAKE_CASE`. (you might find some files that use ```pragma once```, this is because I am lazy, but I strongly encourage you to use the screaming snake case).

### Include order in `.cpp` files

1. The file's own header (first, always)
2. Other Aquila headers
3. Third-party headers
4. Standard library headers

```cpp
#include "Aquila/RHI/Vulkan/VulkanTexture.h" // own header first

#include "Aquila/RHI/Vulkan/VulkanDeletionQueue.h" // other Aquila
#include "Aquila/RHI/Vulkan/VulkanDevice.h"
#include "Aquila/RHI/Vulkan/VulkanFormatUtils.h"

#include <vulkan/vulkan.h> // third-party
#include <vma/vk_mem_alloc.h>

#include <vector> // stdlib
#include <stdexcept>
```

### Namespaces

All engine code lives under the `Aquila::` namespace. Sub-namespaces follow the layer:

```cpp
namespace Aquila::RHI { ... }   // RHI interfaces and Vulkan backend
namespace Aquila::GFX { ... }   // GFX wrappers
```

Always close with a comment:
```cpp
} // namespace Aquila::RHI
```

Do not use `using namespace` at file scope in headers. In `.cpp` files it is acceptable only for the file's own namespace.

---

## Classes & Interfaces

### Non-copyable / non-moveable

Use the engine macros. Do not manually delete constructors.

```cpp
class GfxContext {
public:
    AQUILA_NONCOPYABLE(GfxContext);
    AQUILA_NONMOVEABLE(GfxContext);
    // ...
};
```

Most GPU-owning objects should be both non-copyable and non-moveable.

### Abstract base classes

- Default the destructor, mark it `virtual`.
- Keep the default constructor `protected` to prevent direct instantiation.
- Mark all pure virtual overrides with `override` in derived classes.

```cpp
class IRHIDevice {
public:
    virtual ~IRHIDevice() = default;
    IRHIDevice(const IRHIDevice&) = delete;
    IRHIDevice& operator=(const IRHIDevice&) = delete;
    IRHIDevice(IRHIDevice&&) = delete;
    IRHIDevice& operator=(IRHIDevice&&) = delete;

    virtual Unique<IRHIBuffer> CreateBuffer(const BufferDesc&) = 0;

protected:
    IRHIDevice() = default;
};
```

### `[[nodiscard]]`

Apply `[[nodiscard]]` to any method whose return value the caller must not silently discard:

```cpp
[[nodiscard]] virtual Unique<IRHIBuffer> CreateBuffer(const BufferDesc&) = 0;
[[nodiscard]] VkDevice GetDevice() const;
[[nodiscard]] bool IsMapped() const;
```

This covers all factory methods, all getters that return handles, and all boolean query methods.

### Downcasting

Always use `static_cast` when downcasting from an RHI interface to a concrete type. Never use C-style casts.

```cpp
// Correct
auto& vkCmd = static_cast<VulkanCommandList&>(cmd);

// Wrong
auto& vkCmd = (VulkanCommandList&)(cmd);
```

---

## Error Handling

### Vulkan calls

Every fallible Vulkan call must be wrapped in `AQUILA_VULKAN_CHECK`. Never call Vulkan functions bare when they return `VkResult`.

```cpp
// Correct
AQUILA_VULKAN_CHECK(vkCreateCommandPool(m_Device, &poolInfo, nullptr, &m_GraphicsCommandPool));

// Wrong
vkCreateCommandPool(m_Device, &poolInfo, nullptr, &m_GraphicsCommandPool);
```

`AQUILA_VULKAN_CHECK` asserts on failure in debug builds and logs the error. Do not swallow the result.

### Unrecoverable errors

For situations the engine cannot proceed from (no Vulkan-capable GPU, required extension missing), log at critical level then `abort()`:

```cpp
if (deviceCount == 0) {
    AQUILA_LOG_CRITICAL("Failed to find GPUs with Vulkan support!");
    abort();
}
```

Do not throw exceptions for GPU initialization failures.

### Recoverable errors

For runtime failures that can propagate (pool exhausted, surface creation failed), throw `std::runtime_error`:

```cpp
if (!m_GlobalPool->AllocateDescriptor(layout, set)) {
    throw std::runtime_error("Global descriptor pool exhausted");
}
```

### No error-code threading

Do not return `bool` or `VkResult` from internal functions to signal failure. Assert, throw, or abort — pick the appropriate level and handle it at the boundary.

---

## GPU Resource Management

### Never destroy resources inline

GPU resources must never be destroyed while the GPU may still be referencing them. Always route through the deletion queue:

```cpp
// Correct — inside a destructor
auto& queue = m_Device.GetDeletionQueue();
if (m_ImageView != VK_NULL_HANDLE) {
    queue.QueueDeletion(m_ImageView);
}
if (m_ImageAllocation.image != VK_NULL_HANDLE) {
    queue.QueueDeletion(Deletion::VmaImageDeletion{
        .image = m_ImageAllocation.image,
        .allocation = m_ImageAllocation.allocation
    });
}

// Wrong
vkDestroyImageView(m_Device.GetDevice(), m_ImageView, nullptr);
vmaDestroyImage(m_Device.GetAllocator(), m_Image, m_Allocation);
```

### Zero handles after destruction

After queueing or immediately destroying a handle, set it to the null value. This prevents double-destruction if the destructor runs again (e.g. after `DestroyImmediate`):

```cpp
m_ImageView = VK_NULL_HANDLE;
m_ImageAllocation.image = VK_NULL_HANDLE;
m_ImageAllocation.allocation = VK_NULL_HANDLE;
```

### Immediate destruction

If you need to destroy a resource synchronously (outside the normal frame loop), use `DestroyTextureImmediate()` / `DestroyImmediate()`. The caller **must** ensure the GPU is idle before calling:

```cpp
m_Ctx->WaitIdle(); // caller's responsibility
m_Ctx->DestroyTextureImmediate(texture->GetRHI());
```

Document the GPU-idle contract at every call site.

### Always reset old refs before allocating replacements

When recreating a resource (e.g. on resize), explicitly reset the old `Ref` before calling `CreateTexture`. This queues the old resource for deletion before new VRAM is consumed:

```cpp
m_SceneColor.reset();   // queues old texture for deletion
m_DepthTex.reset();
m_SceneColor = m_Ctx->CreateTexture(colorDesc);
m_DepthTex = m_Ctx->CreateTexture(depthDesc);
```

---

## Vulkan-Specific Rules

### Struct initialization

Always zero-initialize Vulkan structs and set `sType` explicitly:

```cpp
VkCommandPoolCreateInfo poolInfo{};
poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
poolInfo.queueFamilyIndex = indices.m_GraphicsFamily.value();
poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
```

Designated initializers are acceptable for value-only structs (no `sType`):

```cpp
VkClearColorValue vkClear{ .float32 = { r, g, b, a } };
```

### Queue submission

All queue submits must hold the corresponding mutex:

```cpp
void VulkanDevice::SubmitToGraphicsQueue(const VkSubmitInfo* submitInfo, VkFence fence) {
    std::lock_guard<std::mutex> lock(m_GraphicsQueueMutex);
    AQUILA_VULKAN_CHECK(vkQueueSubmit(m_GraphicsQueue, 1, submitInfo, fence));
}
```

Never call `vkQueueSubmit` directly outside of the `SubmitTo*Queue` helpers.

### Debug names

Every GPU resource must be given a debug name via `SetObjectDebugName()`. Use the `debugName` field from the descriptor and append a suffix for sub-objects:

```cpp
m_Device.SetObjectDebugName(VK_OBJECT_TYPE_IMAGE,
    reinterpret_cast<uint64>(m_ImageAllocation.image),
    (desc.debugName + "_Image").c_str());

m_Device.SetObjectDebugName(VK_OBJECT_TYPE_IMAGE_VIEW,
    reinterpret_cast<uint64>(m_ImageView),
    (desc.debugName + "_ImageView").c_str());
```

Always provide a non-empty `debugName` in resource descriptors.

### VMA

All GPU memory allocation must go through VMA. Never call `vkAllocateMemory` directly.
Use the typed helpers on `VulkanDevice`:

```cpp
m_ImageAllocation = m_Device.CreateImage<MemoryDomain::GPU_ONLY>(
    desc.width, desc.height, format, usage, mipLevels, arrayLayers, samples, debugName);
```

---

## Comments

### When to comment

Comment the *why*, not the *what*. If the code clearly expresses what it does, no comment is needed. If it does something non-obvious — a workaround, a GPU sync invariant, an ordering requirement — explain it:

```cpp
Wait(); // before killing device wait for it, be gentle

// Sampler is owned by the device sampler cache — do not destroy it here
```

### Namespace closing braces

Always annotate the closing brace of a namespace:

```cpp
} // namespace Aquila::RHI
```

### No doc-comment blocks

The codebase does not use Doxygen or similar. Do not add `/** */` blocks. Prefer self-documenting names and targeted inline comments.

---

## Miscellaneous Patterns

### Local helper lambdas

For logic that is used only within a single function and would clutter the class interface, use a local lambda:

```cpp
auto makeModule = [&](const ShaderStageDesc& s) {
    return VulkanShader::CreateShaderModule(s.spirv, *this, s.entryPoint);
};

VkShaderModule vertModule = makeModule(desc.vertexShader);
VkShaderModule fragModule = makeModule(desc.fragmentShader);
```

### Template helpers for type safety

Prefer typed template wrappers over `void*` interfaces:

```cpp
template<typename T>
void PushConstants(const T& data, ShaderStage stages, uint32 offset = 0);

template<typename Func>
void ExecuteImmediate(CommandListType type, Func&& func);
```

### Validation-layer gating

Validation-layer-only code must be guarded:

```cpp
if (enableValidationLayers) {
    DestroyDebugMessengerEXT(m_VulkanInstance, m_DebugMessenger, nullptr);
}
```

Never leave validation setup code running unconditionally in release paths.

### Logging macros

Use the structured logging macros consistently. Match the severity to the situation:

| Macro | When |
|-------|------|
| `AQUILA_LOG_DEBUG` | Lifecycle events, object creation/destruction |
| `AQUILA_LOG_INFO` | Device selection, capability reporting |
| `AQUILA_LOG_WARNING` | Degraded paths, non-fatal Vulkan warnings |
| `AQUILA_LOG_ERROR` | Vulkan errors from the debug callback |
| `AQUILA_LOG_CRITICAL` | Unrecoverable failures before `abort()` |