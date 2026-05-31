#pragma once

#include "Aquila/Foundation/Defines.h"
#include "Aquila/GFX/GfxContext.h"
#include "Aquila/GFX/GfxTexture.h"

namespace Aquila::UI::Core {

class TextureCache {
  public:
	explicit TextureCache(GFX::GfxContext &ctx, std::string basePath = {});

	AQUILA_NONCOPYABLE(TextureCache);
	AQUILA_NONMOVEABLE(TextureCache);

	[[nodiscard]] GFX::GfxTexture *Load(const std::string &path);

	void Evict(const std::string &path);

	void Clear();

  private:
	[[nodiscard]] std::string Resolve(const std::string &path) const;

	GFX::GfxContext &m_Ctx;
	std::string m_BasePath;

	std::unordered_map<std::string, Ref<GFX::GfxTexture>> m_Cache;
};

} // namespace Aquila::UI::Core
