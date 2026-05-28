#include "Aquila/UI/Core/TextureCache.h"
#include "Aquila/Foundation/Macros.h"
#include "Aquila/RHI/Backend/RHITypes.h"
#include "Aquila/Platform/Filesystem/VirtualFileSystem.h"

#include "stb/stb_image.h"

namespace Aquila::UI::Core {

TextureCache::TextureCache(GFX::GfxContext &ctx, std::string basePath) : m_Ctx(ctx), m_BasePath(std::move(basePath)) {}

std::string TextureCache::Resolve(const std::string &path) const {
	if (m_BasePath.empty() || path.find("://") != std::string::npos || (!path.empty() && path[0] == '/')) {
		return path;
	}
	return m_BasePath + "/" + path;
}

GFX::GfxTexture *TextureCache::Load(const std::string &path) {
	const std::string resolved = Resolve(path);

	auto it = m_Cache.find(resolved);
	if (it != m_Cache.end()) {
		return it->second.get();
	}

	auto vfile = Platform::Filesystem::VirtualFileSystem::Get()->OpenFile(resolved, AccessMode::Read, OpenMode::Binary);
	if (!vfile || !vfile->IsValid()) {
		AQUILA_LOG_ERROR("TextureCache: cannot open '{}'", resolved);
		return nullptr;
	}
	const int64 fileSize = vfile->Size();
	std::vector<uint8> fileData(static_cast<usize>(fileSize));
	vfile->Read(fileData.data(), static_cast<usize>(fileSize));

	int width = 0, height = 0, channels = 0;
	stbi_uc *pixels =
		stbi_load_from_memory(fileData.data(), static_cast<int>(fileSize), &width, &height, &channels, STBI_rgb_alpha);
	if (pixels == nullptr) {
		AQUILA_LOG_ERROR("TextureCache: failed to load '{}': {}", resolved, stbi_failure_reason());
		return nullptr;
	}

	RHI::TextureDesc desc{};
	desc.width = static_cast<uint32>(width);
	desc.height = static_cast<uint32>(height);
	desc.format = RHI::TextureFormat::RGBA8;
	desc.usage = RHI::TextureUsage::Sampled | RHI::TextureUsage::TransferDst;
	desc.sampler = RHI::SamplerDesc::FontAtlas(); // clamp-to-edge, maxLod=0, no anisotropy
	desc.debugName = resolved;

	Ref<GFX::GfxTexture> tex = m_Ctx.CreateTexture(desc);
	if (!tex) {
		stbi_image_free(pixels);
		AQUILA_LOG_ERROR("TextureCache: GfxContext::CreateTexture failed for '{}'", resolved);
		return nullptr;
	}

	const uint64 byteSize = static_cast<uint64>(width) * static_cast<uint64>(height) * 4u;
	m_Ctx.UploadTextureData(*tex, pixels, byteSize);
	stbi_image_free(pixels);

	GFX::GfxTexture *raw = tex.get();
	m_Cache.emplace(resolved, std::move(tex));
	return raw;
}

void TextureCache::Evict(const std::string &path) {
	m_Cache.erase(Resolve(path));
}

void TextureCache::Clear() {
	m_Cache.clear();
}

} // namespace Aquila::UI::Core
