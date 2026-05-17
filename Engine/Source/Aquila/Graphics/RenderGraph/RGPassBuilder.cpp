#include "Aquila/Graphics/RenderGraph/RGPassBuilder.h"

namespace Aquila::Graphics::RG {

static uint32 SlotOf(uint32 id) {
	return id & 0x00FFFFFFu;
}

RGPassBuilder::RGPassBuilder(std::string_view passName, RGRegistry &registry) : m_Registry(registry) {
	m_Data.name = passName;
}

RGTextureHandle RGPassBuilder::ReadTexture(RGTextureHandle handle, ResourceState state) {
	if (!handle.IsValid()) {
		m_Data.hasUnsatisfiedDep = true;
		return handle;
	}

	// Same slot declared as a read twice is fine, e.g. depth-read attachment + SRV in the same pass.
	const uint32 slot = SlotOf(handle.id);
	auto slotMatches = [slot](const RGTextureAccess &a) { return SlotOf(a.handle.id) == slot; };
	if (std::ranges::any_of(m_Data.textureReads, slotMatches)) {
		return handle;
	}

	AssertNoDuplicateTextureAccess(handle, /*isWrite=*/false);
	m_Data.textureReads.push_back({ handle, state });
	return handle;
}

RGTextureHandle RGPassBuilder::WriteTexture(RGTextureHandle handle, ResourceState state) {
	AQUILA_ASSERT(handle.IsValid(), "WriteTexture: invalid handle");
	AssertNoDuplicateTextureAccess(handle, /*isWrite=*/true);

	// Bump version in the registry — every write produces a new handle.
	RGTextureHandle newHandle = m_Registry.WriteTexture(handle);
	m_Data.textureWrites.push_back({ newHandle, state });
	return newHandle;
}

RGBufferHandle RGPassBuilder::ReadBuffer(RGBufferHandle handle, ResourceState state) {
	if (!handle.IsValid()) {
		m_Data.hasUnsatisfiedDep = true;
		return handle;
	}

	const uint32 slot = SlotOf(handle.id);
	auto slotMatches = [slot](const RGBufferAccess &a) { return SlotOf(a.handle.id) == slot; };
	if (std::ranges::any_of(m_Data.bufferReads, slotMatches)) {
		return handle;
	}

	AssertNoDuplicateBufferAccess(handle, /*isWrite=*/false);
	m_Data.bufferReads.push_back({ handle, state });
	return handle;
}

RGBufferHandle RGPassBuilder::WriteBuffer(RGBufferHandle handle, ResourceState state) {
	AQUILA_ASSERT(handle.IsValid(), "WriteBuffer: invalid handle");
	AssertNoDuplicateBufferAccess(handle, /*isWrite=*/true);

	RGBufferHandle newHandle = m_Registry.WriteBuffer(handle);
	m_Data.bufferWrites.push_back({ newHandle, state });
	return newHandle;
}

RGTextureHandle RGPassBuilder::SetDepthAttachment(RGTextureHandle handle, AttachmentLoadOp depthLoad,
												  AttachmentStoreOp depthStore, AttachmentLoadOp stencilLoad,
												  AttachmentStoreOp stencilStore, bool readOnly, ClearDepth clear) {
	AQUILA_ASSERT(handle.IsValid(), "SetDepthAttachment: invalid handle");
	AQUILA_ASSERT(!m_Data.hasDepthAttachment, "A pass can only have one depth attachment");

	RGTextureHandle resolvedHandle = handle;

	if (readOnly) {
		// Read-only depth: register as a texture read with DepthRead state.
		// No version bump the resource isn't modified.
		resolvedHandle = ReadTexture(handle, ResourceState::DepthRead);
	} else {
		// Read-write depth: counts as a write, bumps the version.
		resolvedHandle = WriteTexture(handle, ResourceState::DepthWrite);
	}

	m_Data.depthAttachment = RGDepthAttachment{
		.handle = resolvedHandle,
		.depthLoadOp = depthLoad,
		.depthStoreOp = depthStore,
		.stencilLoadOp = stencilLoad,
		.stencilStoreOp = stencilStore,
		.readOnly = readOnly,
		.clear = clear,
	};
	m_Data.hasDepthAttachment = true;

	return resolvedHandle;
}

RGTextureHandle RGPassBuilder::SetColorAttachment(uint32 slot, RGTextureHandle handle, AttachmentLoadOp loadOp,
												  AttachmentStoreOp storeOp, ClearColor clear) {
	AQUILA_ASSERT(handle.IsValid(), "SetColorAttachment: invalid handle");

	// Color attachments are always writes, bump the version.
	RGTextureHandle newHandle = WriteTexture(handle, ResourceState::ColorAttachment);

	// Grow the slot array to fit.
	if (slot >= m_Data.colorAttachments.size()) {
		m_Data.colorAttachments.resize(slot + 1, RGColorAttachment{ .handle = RGTextureHandle{},
																	.loadOp = AttachmentLoadOp::DontCare,
																	.storeOp = AttachmentStoreOp::DontCare,
																	.clear = {} });
	}

	m_Data.colorAttachments[slot] = RGColorAttachment{
		.handle = newHandle,
		.loadOp = loadOp,
		.storeOp = storeOp,
		.clear = clear,
	};

	return newHandle;
}

// Duplicate access guards
//
// Catching these at setup time is cheaper than debugging a mis-ordered barrier
// at runtime.  We match on the slot index (lower 24 bits) so that a stale
// handle and a current handle to the same slot are still caught.

void RGPassBuilder::AssertNoDuplicateTextureAccess(RGTextureHandle handle, bool isWrite) const {
	const uint32 slot = SlotOf(handle.id);

	auto slotMatches = [slot](const RGTextureAccess &a) { return SlotOf(a.handle.id) == slot; };

	const bool inReads = std::ranges::any_of(m_Data.textureReads, slotMatches);
	const bool inWrites = std::ranges::any_of(m_Data.textureWrites, slotMatches);

	if (isWrite) {
		AQUILA_ASSERT(!inReads, "Texture slot is already declared as a read in this pass — cannot also write it");
		AQUILA_ASSERT(!inWrites, "Texture slot is already declared as a write in this pass — double-write detected");
	} else {
		// Duplicate reads are caught upstream in ReadTexture before reaching here.
		AQUILA_ASSERT(!inReads, "Texture slot is already declared as a read in this pass");
		AQUILA_ASSERT(!inWrites, "Texture slot is already declared as a write in this pass — cannot also read it");
	}
}

void RGPassBuilder::AssertNoDuplicateBufferAccess(RGBufferHandle handle, bool isWrite) const {
	const uint32 slot = SlotOf(handle.id);

	auto slotMatches = [slot](const RGBufferAccess &a) { return SlotOf(a.handle.id) == slot; };

	const bool inReads = std::ranges::any_of(m_Data.bufferReads, slotMatches);
	const bool inWrites = std::ranges::any_of(m_Data.bufferWrites, slotMatches);

	if (isWrite) {
		AQUILA_ASSERT(!inReads, "Buffer slot is already declared as a read in this pass — cannot also write it");
		AQUILA_ASSERT(!inWrites, "Buffer slot is already declared as a write in this pass — double-write detected");
	} else {
		AQUILA_ASSERT(!inReads, "Buffer slot is already declared as a read in this pass");
		AQUILA_ASSERT(!inWrites, "Buffer slot is already declared as a write in this pass — cannot also read it");
	}
}

} // namespace Aquila::Graphics::RG
