#include "Aquila/Graphics/RenderGraph/RGRegistry.h"

namespace Aquila::Graphics::RG {

RGTextureHandle RGRegistry::declare_texture(const RGTextureDesc &desc) {
	const auto index = static_cast<Uint32>(m_textures.size());
	auto &entry = m_textures.emplace_back();
	entry.desc = desc;
	return RGTextureHandle{ encode_handle(index, 0) };
}

RGBufferHandle RGRegistry::declare_buffer(const RGBufferDesc &desc) {
	const auto index = static_cast<Uint32>(m_buffers.size());
	auto &entry = m_buffers.emplace_back();
	entry.desc = desc;
	return RGBufferHandle{ encode_handle(index, 0) };
}

RGTextureHandle RGRegistry::import_texture(GFX::GfxTexture *texture, std::string_view debug_name,
										  ResourceState initial_state) {
	AQUILA_ASSERT(texture, "Cannot import a null texture");

	const RHI::TextureDesc &rhi_desc = texture->get_desc();

	RGTextureDesc desc{};
	desc.width = rhi_desc.width;
	desc.height = rhi_desc.height;
	desc.mip_levels = rhi_desc.mip_levels;
	desc.array_layers = rhi_desc.array_layers;
	desc.format = rhi_desc.format;
	desc.usage = rhi_desc.usage;
	desc.samples = rhi_desc.samples;
	desc.debug_name = debug_name;

	const auto index = static_cast<Uint32>(m_textures.size());
	auto &entry = m_textures.emplace_back();
	entry.desc = desc;
	entry.imported = true;
	entry.initial_state = initial_state;
	entry.imported_ptr = texture;
	entry.physical = texture; // Already resolved, no allocation needed.

	return RGTextureHandle{ encode_handle(index, 0) };
}

RGBufferHandle RGRegistry::import_buffer(GFX::GfxBuffer *buffer, std::string_view debug_name,
										ResourceState initial_state) {
	AQUILA_ASSERT(buffer, "Cannot import a null buffer");

	RGBufferDesc desc{};
	desc.size = buffer->get_size();
	desc.debug_name = debug_name;

	const auto index = static_cast<Uint32>(m_buffers.size());
	auto &entry = m_buffers.emplace_back();
	entry.desc = desc;
	entry.imported = true;
	entry.initial_state = initial_state;
	entry.imported_ptr = buffer;
	entry.physical = buffer;

	return RGBufferHandle{ encode_handle(index, 0) };
}

RGTextureHandle RGRegistry::write_texture(RGTextureHandle handle) {
	validate_texture_handle(handle);
	const Uint32 index = slot_of(handle.id);
	auto &entry = m_textures[index];
	const Uint32 new_ver = ++entry.version;

	AQUILA_ASSERT(new_ver < (1u << (32u - K_VERSION_SHIFT)),
				  "Texture version counter overflow — too many writes to one slot");

	return RGTextureHandle{ encode_handle(index, new_ver) };
}

RGBufferHandle RGRegistry::write_buffer(RGBufferHandle handle) {
	validate_buffer_handle(handle);
	const Uint32 index = slot_of(handle.id);
	auto &entry = m_buffers[index];
	const Uint32 new_ver = ++entry.version;

	AQUILA_ASSERT(new_ver < (1u << (32u - K_VERSION_SHIFT)),
				  "Buffer version counter overflow — too many writes to one slot");

	return RGBufferHandle{ encode_handle(index, new_ver) };
}

void RGRegistry::resolve_texture(RGTextureHandle handle, GFX::GfxTexture *physical) {
	AQUILA_ASSERT(physical, "Resolving texture with null physical pointer");
	const Uint32 index = slot_of(handle.id);
	AQUILA_ASSERT(index < m_textures.size(), "RGTextureHandle out of range");

	auto &entry = m_textures[index];
	if (entry.imported) {
		AQUILA_ASSERT(physical == entry.imported_ptr,
					  "Imported texture resolved with a different pointer — "
					  "did you pass the wrong GfxTexture?");
	}
	entry.physical = physical;
}

void RGRegistry::resolve_buffer(RGBufferHandle handle, GFX::GfxBuffer *physical) {
	AQUILA_ASSERT(physical, "Resolving buffer with null physical pointer");
	const Uint32 index = slot_of(handle.id);
	AQUILA_ASSERT(index < m_buffers.size(), "RGBufferHandle out of range");

	auto &entry = m_buffers[index];
	if (entry.imported) {
		AQUILA_ASSERT(physical == entry.imported_ptr, "Imported buffer resolved with a different pointer");
	}
	entry.physical = physical;
}

// const RGTextureDesc &RGRegistry::GetTextureDesc(RGTextureHandle handle) const {
// 	ValidateTextureHandle(handle);
// 	return m_Textures[SlotOf(handle.id)].desc;
// }

const RGTextureDesc &RGRegistry::get_texture_desc(RGTextureHandle handle) const {
	const Uint32 index = slot_of(handle.id);
	AQUILA_ASSERT(index < m_textures.size(), "RGTextureHandle index out of range");
	return m_textures[index].desc;
}

const RGBufferDesc &RGRegistry::get_buffer_desc(RGBufferHandle handle) const {
	const Uint32 index = slot_of(handle.id);
	AQUILA_ASSERT(index < m_buffers.size(), "RGBufferHandle index out of range");
	return m_buffers[index].desc;
}

GFX::GfxTexture &RGRegistry::get_texture(RGTextureHandle handle) const {
	// ValidateTextureHandle(handle);
	const auto &entry = m_textures[slot_of(handle.id)];
	AQUILA_ASSERT(entry.physical, "Texture has not been resolved yet — called GetTexture before Execute?");
	return *entry.physical;
}

GFX::GfxBuffer &RGRegistry::get_buffer(RGBufferHandle handle) const {
	// ValidateBufferHandle(handle);
	const auto &entry = m_buffers[slot_of(handle.id)];
	AQUILA_ASSERT(entry.physical, "Buffer has not been resolved yet — called GetBuffer before Execute?");
	return *entry.physical;
}

// bool RGRegistry::IsImportedTexture(RGTextureHandle handle) const {
// 	ValidateTextureHandle(handle);
// 	return m_Textures[SlotOf(handle.id)].imported;
// }

// bool RGRegistry::IsImportedBuffer(RGBufferHandle handle) const {
// 	ValidateBufferHandle(handle);
// 	return m_Buffers[SlotOf(handle.id)].imported;
// }

bool RGRegistry::is_imported_texture(RGTextureHandle handle) const {
	const Uint32 index = slot_of(handle.id);
	AQUILA_ASSERT(index < m_textures.size(), "RGTextureHandle index out of range");
	return m_textures[index].imported;
}

bool RGRegistry::is_imported_buffer(RGBufferHandle handle) const {
	const Uint32 index = slot_of(handle.id);
	AQUILA_ASSERT(index < m_buffers.size(), "RGBufferHandle index out of range");
	return m_buffers[index].imported;
}

Uint32 RGRegistry::get_texture_version(RGTextureHandle handle) const {
	AQUILA_ASSERT(slot_of(handle.id) < m_textures.size(), "RGTextureHandle out of range");
	return m_textures[slot_of(handle.id)].version;
}

Uint32 RGRegistry::get_buffer_version(RGBufferHandle handle) const {
	AQUILA_ASSERT(slot_of(handle.id) < m_buffers.size(), "RGBufferHandle out of range");
	return m_buffers[slot_of(handle.id)].version;
}

ResourceState RGRegistry::get_texture_initial_state(RGTextureHandle handle) const {
	const Uint32 index = slot_of(handle.id);
	AQUILA_ASSERT(index < m_textures.size(), "RGTextureHandle index out of range");
	return m_textures[index].initial_state;
}

ResourceState RGRegistry::get_buffer_initial_state(RGBufferHandle handle) const {
	const Uint32 index = slot_of(handle.id);
	AQUILA_ASSERT(index < m_buffers.size(), "RGBufferHandle index out of range");
	return m_buffers[index].initial_state;
}

void RGRegistry::reset() {
	m_textures.clear();
	m_buffers.clear();
}

void RGRegistry::validate_texture_handle(RGTextureHandle handle) const {
	AQUILA_ASSERT(handle.is_valid(), "Using an invalid RGTextureHandle");
	const Uint32 index = slot_of(handle.id);
	const Uint32 ver = version_of(handle.id);
	AQUILA_ASSERT(index < m_textures.size(), "RGTextureHandle index out of range");

	if (ver != m_textures[index].version) {
		AQUILA_LOG_CRITICAL("Stale handle: slot={} handle_ver={} current_ver={} name={}", index, ver,
							m_textures[index].version, m_textures[index].desc.debug_name);
	}

	AQUILA_ASSERT(ver == m_textures[index].version,
				  "Stale RGTextureHandle: a write pass has produced a newer version. "
				  "Use the handle returned by WriteTexture() instead.");
}

void RGRegistry::validate_buffer_handle(RGBufferHandle handle) const {
	AQUILA_ASSERT(handle.is_valid(), "Using an invalid RGBufferHandle");
	const Uint32 index = slot_of(handle.id);
	const Uint32 ver = version_of(handle.id);
	AQUILA_ASSERT(index < m_buffers.size(), "RGBufferHandle index out of range");
	AQUILA_ASSERT(ver == m_buffers[index].version,
				  "Stale RGBufferHandle: a write pass has produced a newer version. "
				  "Use the handle returned by WriteBuffer() instead.");
}

} // namespace Aquila::Graphics::RG
