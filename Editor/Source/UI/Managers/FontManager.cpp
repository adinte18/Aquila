#include "UI/Managers/FontManager.h"

#include "Aquila/Foundation/Macros.h"
#include "Aquila/GFX/GfxContext.h"
#include "Aquila/UI/Core/FontRegistry.h"

namespace Editor::UI {

namespace {

std::string family_to_path(const std::string &family) {
	if (family == "Inconsolata") {
		return "/resources/Engine/Fonts/Inconsolata/Inconsolata-Regular.ttf";
	}
	return "/resources/Engine/Fonts/Outfit/Outfit-Regular.ttf";
}

} // namespace

FontManager &FontManager::get() {
	static FontManager instance;
	return instance;
}

void FontManager::initialize(Aquila::GFX::GfxContext &ctx, const Config::FontSettings &settings) {
	if (m_initialized) {
		AQUILA_LOG_WARNING("FontManager: already initialized");
		return;
	}
	reload(ctx, settings);
}

void FontManager::reload(Aquila::GFX::GfxContext &ctx, const Config::FontSettings &settings) {
	std::unordered_map<std::string, Aquila::UI::Text::FontAtlas *> font_map;

	auto load = [&](const char *name, const std::string &path) {
		auto atlas = Aquila::UI::Text::FontAtlas::create_from_file(ctx, path);
		if (!atlas) {
			AQUILA_LOG_ERROR("FontManager: failed to load '{}' from {}", name, path);
			return;
		}
		Aquila::UI::Core::FontRegistry::Register(name, atlas.get());
		font_map[name] = atlas.get();
		m_atlases.push_back(std::move(atlas));
	};

	load("regular", family_to_path(settings.main_family));
	load("mono", family_to_path(settings.mono_family));

	if (auto it = font_map.find("regular"); it != font_map.end()) {
		Aquila::UI::Core::FontRegistry::Register("medium", it->second);
		Aquila::UI::Core::FontRegistry::Register("bold", it->second);
		font_map["medium"] = it->second;
		font_map["bold"] = it->second;
	}

	m_font_map = std::move(font_map);
	m_initialized = true;
	AQUILA_LOG_INFO("FontManager: main '{}', mono '{}' at {}pt, {} atlases retained", settings.main_family,
					settings.mono_family, settings.size, m_atlases.size());
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
