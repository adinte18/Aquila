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
	Uint32 version = 0;

	// True when the resource was Imported (swapchain image, persistent GBuffer …).
	// The registry holds a non-owning view; lifetime is managed externally.
	bool imported = false;

	// Frame-start state. Only relevant for imported resources; transients always start at Undefined.
	ResourceState initial_state = ResourceState::Undefined;

	// Physical backing — null until Execute() allocates / imports it.
	GFX::GfxTexture *physical = nullptr;

	// For imported resources the registry stores the raw pointer it was given.
	// For transient resources this stays null; physical is set by the allocator.
	GFX::GfxTexture *imported_ptr = nullptr;
};

struct RGBufferEntry {
	RGBufferDesc desc;
	Uint32 version = 0;
	bool imported = false;
	ResourceState initial_state = ResourceState::Undefined;
	GFX::GfxBuffer *physical = nullptr;
	GFX::GfxBuffer *imported_ptr = nullptr;
};

class RGRegistry {
  public:
	RGRegistry() = default;
	~RGRegistry() = default;

	AQUILA_NONCOPYABLE(RGRegistry);

	RGRegistry(RGRegistry &&) = default;
	RGRegistry &operator=(RGRegistry &&) = default;

	[[nodiscard]] RGTextureHandle declare_texture(const RGTextureDesc &desc);

	[[nodiscard]] RGBufferHandle declare_buffer(const RGBufferDesc &desc);

	[[nodiscard]] RGTextureHandle import_texture(GFX::GfxTexture *texture, std::string_view debug_name = {},
												ResourceState initial_state = ResourceState::Undefined);

	[[nodiscard]] RGBufferHandle import_buffer(GFX::GfxBuffer *buffer, std::string_view debug_name = {},
											  ResourceState initial_state = ResourceState::Undefined);

	/// Signal that a pass will write to this texture.
	/// Returns a NEW handle whose id encodes the incremented version.
	/// The old handle remains valid as a read-only reference to the previous
	/// version; the new handle must be used for any subsequent reads.
	[[nodiscard]] RGTextureHandle write_texture(RGTextureHandle handle);

	[[nodiscard]] RGBufferHandle write_buffer(RGBufferHandle handle);

	void resolve_texture(RGTextureHandle handle, GFX::GfxTexture *physical);

	void resolve_buffer(RGBufferHandle handle, GFX::GfxBuffer *physical);

	/// Descriptor lookup (valid after Declare / Import, before Execute).
	[[nodiscard]] const RGTextureDesc &get_texture_desc(RGTextureHandle handle) const;
	[[nodiscard]] const RGBufferDesc &get_buffer_desc(RGBufferHandle handle) const;

	/// Physical resource lookup (valid only after ResolveTexture / ResolveBuffer).
	[[nodiscard]] GFX::GfxTexture &get_texture(RGTextureHandle handle) const;
	[[nodiscard]] GFX::GfxBuffer &get_buffer(RGBufferHandle handle) const;

	/// True if the handle refers to an imported (externally-owned) resource.
	[[nodiscard]] bool is_imported_texture(RGTextureHandle handle) const;
	[[nodiscard]] bool is_imported_buffer(RGBufferHandle handle) const;

	/// State at import time, used to seed barrier tracking for persistent resources.
	[[nodiscard]] ResourceState get_texture_initial_state(RGTextureHandle handle) const;
	[[nodiscard]] ResourceState get_buffer_initial_state(RGBufferHandle handle) const;

	/// Current write-version for a slot (0 = never written, 1 after first write).
	[[nodiscard]] Uint32 get_texture_version(RGTextureHandle handle) const;
	[[nodiscard]] Uint32 get_buffer_version(RGBufferHandle handle) const;

	/// Total number of registered texture / buffer slots.
	[[nodiscard]] Uint32 texture_count() const { return static_cast<Uint32>(m_textures.size()); }
	[[nodiscard]] Uint32 buffer_count() const { return static_cast<Uint32>(m_buffers.size()); }

	/// Reset all state — called at the start of each frame before graph build.
	void reset();

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
	static constexpr Uint32 K_VERSION_SHIFT = 24u;
	static constexpr Uint32 K_INDEX_MASK =
		(1u << K_VERSION_SHIFT) - 1u; // bottom 24 bits: 000000 11111111 11111111 11111111
	static constexpr Uint32 K_VERSION_MASK = ~K_INDEX_MASK; // top    8  bits: 111111 00000000 00000000 00000000

	static Uint32 encode_handle(Uint32 index, Uint32 version) {
		return (version << K_VERSION_SHIFT) | (index & K_INDEX_MASK);
	}

	// strips top 8 bits
	static Uint32 slot_of(Uint32 id) { return id & K_INDEX_MASK; }

	// strips bottom 24 bits
	static Uint32 version_of(Uint32 id) { return (id & K_VERSION_MASK) >> K_VERSION_SHIFT; }

	void validate_texture_handle(RGTextureHandle handle) const;
	void validate_buffer_handle(RGBufferHandle handle) const;

	std::vector<RGTextureEntry> m_textures;
	std::vector<RGBufferEntry> m_buffers;
};

} // namespace Aquila::Graphics::RG
