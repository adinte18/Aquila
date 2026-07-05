#include "UI/Managers/FontManager.h"

#include "Aquila/Foundation/Macros.h"
#include "Aquila/GFX/GfxContext.h"
#include "Aquila/UI/Core/FontRegistry.h"

namespace Editor::UI {

FontManager &FontManager::get() {
	static FontManager instance;
	return instance;
}

void FontManager::initialize(Aquila::GFX::GfxContext &ctx, const Config::FontSettings &settings) {
	if (m_initialized) {
		AQUILA_LOG_WARNING("FontManager: already initialized");
		return;
	}

	auto load = [&](const char *name, const std::string &path) {
		auto atlas = Aquila::UI::Text::FontAtlas::create_from_file(ctx, path, settings.size);
		if (!atlas) {
			AQUILA_LOG_ERROR("FontManager: failed to load '{}' from {}", name, path);
			return;
		}
		Aquila::UI::Core::FontRegistry::Register(name, atlas.get());
		m_font_map[name] = atlas.get();
		m_atlases.push_back(std::move(atlas));
	};

	load("regular", settings.regular_path);
	load("thin", settings.thin_path);
	load("medium", settings.medium_path);
	load("bold", settings.bold_path);

	m_initialized = true;
	AQUILA_LOG_INFO("FontManager: loaded {} fonts at {}pt", m_atlases.size(), settings.size);
}

void FontManager::shutdown() {
	m_font_map.clear();
	m_atlases.clear();
	m_initialized = false;
}

Aquila::UI::Text::FontAtlas *FontManager::get_font(const std::string &name) const {
	auto it = m_font_map.find(name);
	if (it != m_font_map.end()) {
		return it->second;
	}
	AQUILA_LOG_WARNING("FontManager: font '{}' not found, returning fallback", name);
	return m_atlases.empty() ? nullptr : m_atlases.front().get();
}

} // namespace Editor::UI
