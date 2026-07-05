#include "Aquila/UI/Core/TextureCache.h"
#include "Aquila/Foundation/Macros.h"
#include "Aquila/RHI/Backend/RHITypes.h"
#include "Aquila/Platform/Filesystem/VirtualFileSystem.h"

#include "stb/stb_image.h"

namespace Aquila::UI::Core {

TextureCache::TextureCache(GFX::GfxContext &ctx, std::string base_path) : m_ctx(ctx), m_base_path(std::move(base_path)) {}

std::string TextureCache::resolve(const std::string &path) const {
	if (m_base_path.empty() || path.find("://") != std::string::npos || (!path.empty() && path[0] == '/')) {
		return path;
	}
	return m_base_path + "/" + path;
}

GFX::GfxTexture *TextureCache::load(const std::string &path) {
	const std::string resolved = resolve(path);

	auto it = m_cache.find(resolved);
	if (it != m_cache.end()) {
		return it->second.get();
	}

	auto vfile = Platform::Filesystem::VirtualFileSystem::get()->open_file(resolved, AccessMode::Read, OpenMode::Binary);
	if (!vfile || !vfile->is_valid()) {
		AQUILA_LOG_ERROR("TextureCache: cannot open '{}'", resolved);
		return nullptr;
	}
	const Int64 file_size = vfile->size();
	std::vector<Uint8> file_data(static_cast<Usize>(file_size));
	vfile->read(file_data.data(), static_cast<Usize>(file_size));

	int width = 0, height = 0, channels = 0;
	stbi_uc *pixels =
		stbi_load_from_memory(file_data.data(), static_cast<int>(file_size), &width, &height, &channels, STBI_rgb_alpha);
	if (pixels == nullptr) {
		AQUILA_LOG_ERROR("TextureCache: failed to load '{}': {}", resolved, stbi_failure_reason());
		return nullptr;
	}

	RHI::TextureDesc desc{};
	desc.width = static_cast<Uint32>(width);
	desc.height = static_cast<Uint32>(height);
	desc.format = RHI::TextureFormat::RGBA8;
	desc.usage = RHI::TextureUsage::Sampled | RHI::TextureUsage::TransferDst;
	desc.sampler = RHI::SamplerDesc::font_atlas(); // clamp-to-edge, maxLod=0, no anisotropy
	desc.debug_name = resolved;

	Ref<GFX::GfxTexture> tex = m_ctx.create_texture(desc);
	if (!tex) {
		stbi_image_free(pixels);
		AQUILA_LOG_ERROR("TextureCache: GfxContext::CreateTexture failed for '{}'", resolved);
		return nullptr;
	}

	const Uint64 byte_size = static_cast<Uint64>(width) * static_cast<Uint64>(height) * 4u;
	m_ctx.upload_texture_data(*tex, pixels, byte_size);
	stbi_image_free(pixels);

	GFX::GfxTexture *raw = tex.get();
	m_cache.emplace(resolved, std::move(tex));
	return raw;
}

void TextureCache::evict(const std::string &path) {
	m_cache.erase(resolve(path));
}

void TextureCache::clear() {
	m_cache.clear();
}

} // namespace Aquila::UI::Core
