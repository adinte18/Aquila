#pragma once

#include "Core/EditorConfig.h"
#include "Aquila/UI/Text/FontAtlas.h"
#include <string>
#include <unordered_map>
#include <vector>

namespace Aquila::GFX {
class GfxContext;
}

namespace Editor::UI {

class FontManager {
  public:
	static FontManager &get();

	void initialize(Aquila::GFX::GfxContext &ctx, const Config::FontSettings &settings);
	void reload(Aquila::GFX::GfxContext &ctx, const Config::FontSettings &settings);
	void shutdown();

	[[nodiscard]] bool is_initialized() const { return m_initialized; }
	[[nodiscard]] Aquila::UI::Text::FontAtlas *get_font(const std::string &name) const;

  private:
	FontManager() = default;
	~FontManager() = default;
	FontManager(const FontManager &) = delete;
	FontManager &operator=(const FontManager &) = delete;

	bool m_initialized = false;
	std::vector<Unique<Aquila::UI::Text::FontAtlas>> m_atlases;
	std::unordered_map<std::string, Aquila::UI::Text::FontAtlas *> m_font_map;
};

} // namespace Editor::UI
