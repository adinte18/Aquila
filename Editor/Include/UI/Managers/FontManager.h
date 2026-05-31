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
	static FontManager &Get();

	void Initialize(Aquila::GFX::GfxContext &ctx, const Config::FontSettings &settings);
	void Shutdown();

	[[nodiscard]] bool IsInitialized() const { return m_Initialized; }
	[[nodiscard]] Aquila::UI::Text::FontAtlas *GetFont(const std::string &name) const;

  private:
	FontManager() = default;
	~FontManager() = default;
	FontManager(const FontManager &) = delete;
	FontManager &operator=(const FontManager &) = delete;

	bool m_Initialized = false;
	std::vector<Unique<Aquila::UI::Text::FontAtlas>> m_Atlases;
	std::unordered_map<std::string, Aquila::UI::Text::FontAtlas *> m_FontMap;
};

} // namespace Editor::UI
