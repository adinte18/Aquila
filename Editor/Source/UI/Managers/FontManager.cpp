#include "UI/Managers/FontManager.h"

#include "Aquila/Foundation/Macros.h"
#include "Aquila/GFX/GfxContext.h"
#include "Aquila/UI/Core/FontRegistry.h"

namespace Editor::UI {

FontManager &FontManager::Get() {
	static FontManager instance;
	return instance;
}

void FontManager::Initialize(Aquila::GFX::GfxContext &ctx, const Config::FontSettings &settings) {
	if (m_Initialized) {
		AQUILA_LOG_WARNING("FontManager: already initialized");
		return;
	}

	auto load = [&](const char *name, const std::string &path) {
		auto atlas = Aquila::UI::Text::FontAtlas::CreateFromFile(ctx, path, settings.size);
		if (!atlas) {
			AQUILA_LOG_ERROR("FontManager: failed to load '{}' from {}", name, path);
			return;
		}
		Aquila::UI::Core::FontRegistry::Register(name, atlas.get());
		m_FontMap[name] = atlas.get();
		m_Atlases.push_back(std::move(atlas));
	};

	load("regular", settings.regularPath);
	load("thin",    settings.thinPath);
	load("medium",  settings.mediumPath);
	load("bold",    settings.boldPath);

	m_Initialized = true;
	AQUILA_LOG_INFO("FontManager: loaded {} fonts at {}pt", m_Atlases.size(), settings.size);
}

void FontManager::Shutdown() {
	m_FontMap.clear();
	m_Atlases.clear();
	m_Initialized = false;
}

Aquila::UI::Text::FontAtlas *FontManager::GetFont(const std::string &name) const {
	auto it = m_FontMap.find(name);
	if (it != m_FontMap.end()) {
		return it->second;
	}
	AQUILA_LOG_WARNING("FontManager: font '{}' not found, returning fallback", name);
	return m_Atlases.empty() ? nullptr : m_Atlases.front().get();
}

} // namespace Editor::UI
