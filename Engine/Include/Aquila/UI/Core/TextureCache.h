#pragma once

#include "Aquila/Foundation/Defines.h"
#include "Aquila/GFX/GfxContext.h"
#include "Aquila/GFX/GfxTexture.h"

namespace Aquila::UI::Core {

class TextureCache {
  public:
	explicit TextureCache(GFX::GfxContext &ctx, std::string base_path = {});

	AQUILA_NONCOPYABLE(TextureCache);
	AQUILA_NONMOVEABLE(TextureCache);

	[[nodiscard]] GFX::GfxTexture *load(const std::string &path);

	void evict(const std::string &path);

	void clear();

  private:
	[[nodiscard]] std::string resolve(const std::string &path) const;

	GFX::GfxContext &m_ctx;
	std::string m_base_path;

	std::unordered_map<std::string, Ref<GFX::GfxTexture>> m_cache;
};

} // namespace Aquila::UI::Core
