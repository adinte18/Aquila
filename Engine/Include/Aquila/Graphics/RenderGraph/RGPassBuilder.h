#pragma once
#include "Aquila/Foundation/PrimitiveTypes.h"
#include "Aquila/Graphics/RenderGraph/RGTypes.h"
#include "Aquila/Graphics/RenderGraph/RGRegistry.h"

namespace Aquila::GFX {
class GfxCommandList;
}

namespace Aquila::Graphics::RG {

struct RGColorAttachment {
	RGTextureHandle handle;
	AttachmentLoadOp loadOp = AttachmentLoadOp::Clear;
	AttachmentStoreOp storeOp = AttachmentStoreOp::Store;
	ClearColor clear = {};
};

struct RGDepthAttachment {
	RGTextureHandle handle;
	AttachmentLoadOp depthLoadOp = AttachmentLoadOp::Clear;
	AttachmentStoreOp depthStoreOp = AttachmentStoreOp::Store;
	AttachmentLoadOp stencilLoadOp = AttachmentLoadOp::DontCare;
	AttachmentStoreOp stencilStoreOp = AttachmentStoreOp::DontCare;
	bool readOnly = false; // depth-read / stencil-read layouts
	ClearDepth clear = {};
};

// Per-resource access records (used by the executor for barrier emission)
struct RGTextureAccess {
	RGTextureHandle handle;
	ResourceState state;
};

struct RGBufferAccess {
	RGBufferHandle handle;
	ResourceState state;
};
struct RGPassData {
	std::string name;

	// Fine-grained resource accesses (for barrier / hazard tracking).
	std::vector<RGTextureAccess> textureReads;
	std::vector<RGTextureAccess> textureWrites;
	std::vector<RGBufferAccess> bufferReads;
	std::vector<RGBufferAccess> bufferWrites;

	// Renderpass attachment descriptions (empty = compute / copy pass).
	std::vector<RGColorAttachment> colorAttachments;
	RGDepthAttachment depthAttachment = {};
	bool hasDepthAttachment = false;

	// The execute lambda called by the executor with a resolved command list.
	Delegate<void(GFX::GfxCommandList &, RGRegistry &)> RenderPassExecute;

	// When true the culling step keeps this pass alive even if it has no
	// graph-tracked outputs (e.g. a swapchain blit that writes to an external image).
	bool hasSideEffect = false;
};

class RGPassBuilder {
  public:
	// Not user-constructible; the RenderGraph creates one per AddPass call.
	explicit RGPassBuilder(std::string_view passName, RGRegistry &registry);

	/// Declare a sampled  read.
	/// Returns the same handle (reads don't version).
	RGTextureHandle ReadTexture(RGTextureHandle handle, ResourceState state = ResourceState::ShaderRead);

	/// Declare a texture write and returns the NEW versioned handle.
	/// The caller MUST replace their local handle with the returned one.
	[[nodiscard]] RGTextureHandle WriteTexture(RGTextureHandle handle,
											   ResourceState state = ResourceState::ColorAttachment);

	/// Declare a buffer  read.
	RGBufferHandle ReadBuffer(RGBufferHandle handle, ResourceState state = ResourceState::ShaderRead);

	/// Declare a buffer write.
	/// Returns the NEW versioned handle; caller must replace their local copy.
	[[nodiscard]] RGBufferHandle WriteBuffer(RGBufferHandle handle,
											 ResourceState state = ResourceState::UnorderedAccess);

	/// Declare a depth-stencil attachment.
	/// readOnly = true -> DepthRead layout, still usable as SRV in the same pass.
	/// Returns handle (no version bump for read-only; bumps for read-write).
	RGTextureHandle SetDepthAttachment(RGTextureHandle handle, AttachmentLoadOp depthLoad = AttachmentLoadOp::Clear,
									   AttachmentStoreOp depthStore = AttachmentStoreOp::Store,
									   AttachmentLoadOp stencilLoad = AttachmentLoadOp::DontCare,
									   AttachmentStoreOp stencilStore = AttachmentStoreOp::DontCare,
									   bool readOnly = false, ClearDepth clear = {});

	/// Declare a color attachment at a given slot index.
	/// Internally calls WriteTexture — returns the new versioned handle.
	[[nodiscard]] RGTextureHandle SetColorAttachment(uint32 slot, RGTextureHandle handle,
													 AttachmentLoadOp loadOp = AttachmentLoadOp::Clear,
													 AttachmentStoreOp storeOp = AttachmentStoreOp::Store,
													 ClearColor clear = {});

	// Mark this pass as having an external side effect (e.g. swapchain present).
	// Prevents the culling step from removing it even when it has no tracked outputs.
	void MarkAsSideEffect() { m_Data.hasSideEffect = true; }

	// Called by RenderGraph after the setup lambda returns.
	RGPassData &&TakeData() { return std::move(m_Data); }

  private:
	RGRegistry &m_Registry;
	RGPassData m_Data;

	// Ensure a handle isn't double-declared with conflicting states.
	void AssertNoDuplicateTextureAccess(RGTextureHandle handle, bool isWrite) const;
	void AssertNoDuplicateBufferAccess(RGBufferHandle handle, bool isWrite) const;
};

} // namespace Aquila::Graphics::RG
