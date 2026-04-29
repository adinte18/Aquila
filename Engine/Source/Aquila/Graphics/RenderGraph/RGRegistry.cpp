#include "Aquila/Graphics/RenderGraph/RGRegistry.h"

namespace Aquila::Graphics::RG {

RGTextureHandle RGRegistry::DeclareTexture(const RGTextureDesc &desc) {
	const auto index = static_cast<uint32>(m_Textures.size());
	auto &entry = m_Textures.emplace_back();
	entry.desc = desc;
	return RGTextureHandle{ EncodeHandle(index, 0) };
}

RGBufferHandle RGRegistry::DeclareBuffer(const RGBufferDesc &desc) {
	const auto index = static_cast<uint32>(m_Buffers.size());
	auto &entry = m_Buffers.emplace_back();
	entry.desc = desc;
	return RGBufferHandle{ EncodeHandle(index, 0) };
}

RGTextureHandle RGRegistry::ImportTexture(GFX::GfxTexture *texture, std::string_view debugName) {
	AQUILA_ASSERT(texture, "Cannot import a null texture");

	RGTextureDesc desc{};
	desc.width = texture->GetWidth();
	desc.height = texture->GetHeight();
	desc.mipLevels = texture->GetMipLevels();
	desc.arrayLayers = texture->GetArrayLayers();
	desc.format = texture->GetFormat();
	desc.debugName = debugName;

	const auto index = static_cast<uint32>(m_Textures.size());
	auto &entry = m_Textures.emplace_back();
	entry.desc = desc;
	entry.imported = true;
	entry.importedPtr = texture;
	entry.physical = texture; // Already resolved, no allocation needed.

	return RGTextureHandle{ EncodeHandle(index, 0) };
}

RGBufferHandle RGRegistry::ImportBuffer(GFX::GfxBuffer *buffer, std::string_view debugName) {
	AQUILA_ASSERT(buffer, "Cannot import a null buffer");

	RGBufferDesc desc{};
	desc.size = buffer->GetSize();
	desc.debugName = debugName;

	const auto index = static_cast<uint32>(m_Buffers.size());
	auto &entry = m_Buffers.emplace_back();
	entry.desc = desc;
	entry.imported = true;
	entry.importedPtr = buffer;
	entry.physical = buffer;

	return RGBufferHandle{ EncodeHandle(index, 0) };
}

RGTextureHandle RGRegistry::WriteTexture(RGTextureHandle handle) {
	ValidateTextureHandle(handle);
	const uint32 index = SlotOf(handle.id);
	auto &entry = m_Textures[index];
	const uint32 newVer = ++entry.version;

	AQUILA_ASSERT(newVer < (1u << (32u - kVersionShift)),
				  "Texture version counter overflow — too many writes to one slot");

	return RGTextureHandle{ EncodeHandle(index, newVer) };
}

RGBufferHandle RGRegistry::WriteBuffer(RGBufferHandle handle) {
	ValidateBufferHandle(handle);
	const uint32 index = SlotOf(handle.id);
	auto &entry = m_Buffers[index];
	const uint32 newVer = ++entry.version;

	AQUILA_ASSERT(newVer < (1u << (32u - kVersionShift)),
				  "Buffer version counter overflow — too many writes to one slot");

	return RGBufferHandle{ EncodeHandle(index, newVer) };
}

void RGRegistry::ResolveTexture(RGTextureHandle handle, GFX::GfxTexture *physical) {
	AQUILA_ASSERT(physical, "Resolving texture with null physical pointer");
	const uint32 index = SlotOf(handle.id);
	AQUILA_ASSERT(index < m_Textures.size(), "RGTextureHandle out of range");

	auto &entry = m_Textures[index];
	if (entry.imported) {
		AQUILA_ASSERT(physical == entry.importedPtr, "Imported texture resolved with a different pointer — "
													 "did you pass the wrong GfxTexture?");
	}
	entry.physical = physical;
}

void RGRegistry::ResolveBuffer(RGBufferHandle handle, GFX::GfxBuffer *physical) {
	AQUILA_ASSERT(physical, "Resolving buffer with null physical pointer");
	const uint32 index = SlotOf(handle.id);
	AQUILA_ASSERT(index < m_Buffers.size(), "RGBufferHandle out of range");

	auto &entry = m_Buffers[index];
	if (entry.imported) {
		AQUILA_ASSERT(physical == entry.importedPtr, "Imported buffer resolved with a different pointer");
	}
	entry.physical = physical;
}

// const RGTextureDesc &RGRegistry::GetTextureDesc(RGTextureHandle handle) const {
// 	ValidateTextureHandle(handle);
// 	return m_Textures[SlotOf(handle.id)].desc;
// }

const RGTextureDesc &RGRegistry::GetTextureDesc(RGTextureHandle handle) const {
	const uint32 index = SlotOf(handle.id);
	AQUILA_ASSERT(index < m_Textures.size(), "RGTextureHandle index out of range");
	return m_Textures[index].desc;
}

const RGBufferDesc &RGRegistry::GetBufferDesc(RGBufferHandle handle) const {
	const uint32 index = SlotOf(handle.id);
	AQUILA_ASSERT(index < m_Buffers.size(), "RGBufferHandle index out of range");
	return m_Buffers[index].desc;
}

GFX::GfxTexture &RGRegistry::GetTexture(RGTextureHandle handle) const {
	// ValidateTextureHandle(handle);
	const auto &entry = m_Textures[SlotOf(handle.id)];
	AQUILA_ASSERT(entry.physical, "Texture has not been resolved yet — called GetTexture before Execute?");
	return *entry.physical;
}

GFX::GfxBuffer &RGRegistry::GetBuffer(RGBufferHandle handle) const {
	// ValidateBufferHandle(handle);
	const auto &entry = m_Buffers[SlotOf(handle.id)];
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

bool RGRegistry::IsImportedTexture(RGTextureHandle handle) const {
	const uint32 index = SlotOf(handle.id);
	AQUILA_ASSERT(index < m_Textures.size(), "RGTextureHandle index out of range");
	return m_Textures[index].imported;
}

bool RGRegistry::IsImportedBuffer(RGBufferHandle handle) const {
	const uint32 index = SlotOf(handle.id);
	AQUILA_ASSERT(index < m_Buffers.size(), "RGBufferHandle index out of range");
	return m_Buffers[index].imported;
}

uint32 RGRegistry::GetTextureVersion(RGTextureHandle handle) const {
	AQUILA_ASSERT(SlotOf(handle.id) < m_Textures.size(), "RGTextureHandle out of range");
	return m_Textures[SlotOf(handle.id)].version;
}

uint32 RGRegistry::GetBufferVersion(RGBufferHandle handle) const {
	AQUILA_ASSERT(SlotOf(handle.id) < m_Buffers.size(), "RGBufferHandle out of range");
	return m_Buffers[SlotOf(handle.id)].version;
}

void RGRegistry::Reset() {
	m_Textures.clear();
	m_Buffers.clear();
}

void RGRegistry::ValidateTextureHandle(RGTextureHandle handle) const {
	AQUILA_ASSERT(handle.IsValid(), "Using an invalid RGTextureHandle");
	const uint32 index = SlotOf(handle.id);
	const uint32 ver = VersionOf(handle.id);
	AQUILA_ASSERT(index < m_Textures.size(), "RGTextureHandle index out of range");

	if (ver != m_Textures[index].version) {
		AQUILA_LOG_CRITICAL("Stale handle: slot={} handle_ver={} current_ver={} name={}", index, ver,
							m_Textures[index].version, m_Textures[index].desc.debugName);
	}

	AQUILA_ASSERT(ver == m_Textures[index].version, "Stale RGTextureHandle: a write pass has produced a newer version. "
													"Use the handle returned by WriteTexture() instead.");
}

void RGRegistry::ValidateBufferHandle(RGBufferHandle handle) const {
	AQUILA_ASSERT(handle.IsValid(), "Using an invalid RGBufferHandle");
	const uint32 index = SlotOf(handle.id);
	const uint32 ver = VersionOf(handle.id);
	AQUILA_ASSERT(index < m_Buffers.size(), "RGBufferHandle index out of range");
	AQUILA_ASSERT(ver == m_Buffers[index].version, "Stale RGBufferHandle: a write pass has produced a newer version. "
												   "Use the handle returned by WriteBuffer() instead.");
}

} // namespace Aquila::Graphics::RG
