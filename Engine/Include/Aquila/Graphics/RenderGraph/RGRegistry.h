#pragma once
#include "Aquila/Foundation/Defines.h"
#include "Aquila/Foundation/Macros.h"
#include "Aquila/Foundation/PrimitiveTypes.h"

#include "Aquila/Graphics/RenderGraph/RGTypes.h"

#include "Aquila/GFX/GfxTexture.h"
#include "Aquila/GFX/GfxBuffer.h"

namespace Aquila::Graphics::RG {

// Internal entries stored per handle slot.

struct RGTextureEntry {
	RGTextureDesc desc;

	// Version counter: bumped every time a pass declares a write to this slot.
	// Consumers record the version at declaration time; a mismatch at execute
	// time means a use-after-write ordering bug.
	uint32 version = 0;

	// True when the resource was Imported (swapchain image, persistent GBuffer …).
	// The registry holds a non-owning view; lifetime is managed externally.
	bool imported = false;

	// Frame-start state. Only relevant for imported resources; transients always start at Undefined.
	ResourceState initialState = ResourceState::Undefined;

	// Physical backing — null until Execute() allocates / imports it.
	GFX::GfxTexture *physical = nullptr;

	// For imported resources the registry stores the raw pointer it was given.
	// For transient resources this stays null; physical is set by the allocator.
	GFX::GfxTexture *importedPtr = nullptr;
};

struct RGBufferEntry {
	RGBufferDesc desc;
	uint32 version = 0;
	bool imported = false;
	ResourceState initialState = ResourceState::Undefined;
	GFX::GfxBuffer *physical = nullptr;
	GFX::GfxBuffer *importedPtr = nullptr;
};

class RGRegistry {
  public:
	RGRegistry() = default;
	~RGRegistry() = default;

	AQUILA_NONCOPYABLE(RGRegistry);

	RGRegistry(RGRegistry &&) = default;
	RGRegistry &operator=(RGRegistry &&) = default;

	[[nodiscard]] RGTextureHandle DeclareTexture(const RGTextureDesc &desc);

	[[nodiscard]] RGBufferHandle DeclareBuffer(const RGBufferDesc &desc);

	[[nodiscard]] RGTextureHandle ImportTexture(GFX::GfxTexture *texture, std::string_view debugName = {},
											    ResourceState initialState = ResourceState::Undefined);

	[[nodiscard]] RGBufferHandle ImportBuffer(GFX::GfxBuffer *buffer, std::string_view debugName = {},
											  ResourceState initialState = ResourceState::Undefined);

	/// Signal that a pass will write to this texture.
	/// Returns a NEW handle whose id encodes the incremented version.
	/// The old handle remains valid as a read-only reference to the previous
	/// version; the new handle must be used for any subsequent reads.
	[[nodiscard]] RGTextureHandle WriteTexture(RGTextureHandle handle);

	[[nodiscard]] RGBufferHandle WriteBuffer(RGBufferHandle handle);

	void ResolveTexture(RGTextureHandle handle, GFX::GfxTexture *physical);

	void ResolveBuffer(RGBufferHandle handle, GFX::GfxBuffer *physical);

	/// Descriptor lookup (valid after Declare / Import, before Execute).
	[[nodiscard]] const RGTextureDesc &GetTextureDesc(RGTextureHandle handle) const;
	[[nodiscard]] const RGBufferDesc &GetBufferDesc(RGBufferHandle handle) const;

	/// Physical resource lookup (valid only after ResolveTexture / ResolveBuffer).
	[[nodiscard]] GFX::GfxTexture &GetTexture(RGTextureHandle handle) const;
	[[nodiscard]] GFX::GfxBuffer &GetBuffer(RGBufferHandle handle) const;

	/// True if the handle refers to an imported (externally-owned) resource.
	[[nodiscard]] bool IsImportedTexture(RGTextureHandle handle) const;
	[[nodiscard]] bool IsImportedBuffer(RGBufferHandle handle) const;

	/// State at import time, used to seed barrier tracking for persistent resources.
	[[nodiscard]] ResourceState GetTextureInitialState(RGTextureHandle handle) const;
	[[nodiscard]] ResourceState GetBufferInitialState(RGBufferHandle handle) const;

	/// Current write-version for a slot (0 = never written, 1 after first write).
	[[nodiscard]] uint32 GetTextureVersion(RGTextureHandle handle) const;
	[[nodiscard]] uint32 GetBufferVersion(RGBufferHandle handle) const;

	/// Total number of registered texture / buffer slots.
	[[nodiscard]] uint32 TextureCount() const { return static_cast<uint32>(m_Textures.size()); }
	[[nodiscard]] uint32 BufferCount() const { return static_cast<uint32>(m_Buffers.size()); }

	/// Reset all state — called at the start of each frame before graph build.
	void Reset();

  private:
	// NOTE : Handles use the top 8 bits for the version and the lower 24 bits for the
	// slot index. This keeps the "new handle per write" promise without
	// allocating a new entry, so both old and new handles resolve to the same slot
	// so the descriptor and physical pointer are shared and only the version tag
	// differs.
	//
	// So handle (ver=1, slot=5) and handle (ver=2, slot=5) both point to the same
	// physical texture in memory, but they're different handles.
	// That's how the graph knows "this is a different write than before" without wasting memory.
	static constexpr uint32 kVersionShift = 24u;
	static constexpr uint32 kIndexMask =
		(1u << kVersionShift) - 1u;						// bottom 24 bits: 000000 11111111 11111111 11111111
	static constexpr uint32 kVersionMask = ~kIndexMask; // top    8  bits: 111111 00000000 00000000 00000000

	static uint32 EncodeHandle(uint32 index, uint32 version) {
		return (version << kVersionShift) | (index & kIndexMask);
	}

	// strips top 8 bits
	static uint32 SlotOf(uint32 id) { return id & kIndexMask; }

	// strips bottom 24 bits
	static uint32 VersionOf(uint32 id) { return (id & kVersionMask) >> kVersionShift; }

	void ValidateTextureHandle(RGTextureHandle handle) const;
	void ValidateBufferHandle(RGBufferHandle handle) const;

	std::vector<RGTextureEntry> m_Textures;
	std::vector<RGBufferEntry> m_Buffers;
};

} // namespace Aquila::Graphics::RG
