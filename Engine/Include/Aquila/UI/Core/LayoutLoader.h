#pragma once

#include "Aquila/Foundation/Macros.h"
#include "Aquila/UI/Core/View.h"
#include "Aquila/UI/Core/TextureCache.h"
#include "Aquila/UI/Core/TextureIconBank.h"
#include "Aquila/UI/Text/FontAtlas.h"
#include <string>
#include <vector>

namespace Aquila::UI::Core {

class LayoutLoader {
  public:
	using WidgetFactory = Delegate<Unique<View>(std::string_view text, Text::FontAtlas *font)>;

	LayoutLoader();

	void RegisterFont(const std::string &name, Text::FontAtlas *font);
	void SetDefaultFont(Text::FontAtlas *font);
	void RegisterWidget(const std::string &typeName, WidgetFactory factory);

	// Texture-based image loading for <Image src="..."/>.
	void RegisterTextureCache(TextureCache *cache);
	[[nodiscard]] GFX::GfxTexture *ResolveTexture(const std::string &path) const;

	// Texture-based icon banks for <Image icon="name" bank="bankName"/>.
	void RegisterTextureIconBank(const std::string &name, TextureIconBank *bank);
	[[nodiscard]] TextureIconBank *ResolveTextureIconBank(const std::string &name) const;

	void RegisterCommand(const std::string &name, Delegate<void()> command);
	[[nodiscard]] Delegate<void()> ResolveCommand(const std::string &name) const;

	Unique<View> LoadFile(const std::string &path);
	Unique<View> LoadString(std::string_view xml);

	[[nodiscard]] Text::FontAtlas *ResolveFont(const std::string &name) const;
	[[nodiscard]] Unique<View> CreateWidget(const std::string &type, std::string_view text,
											Text::FontAtlas *font) const;

  private:
	std::unordered_map<std::string, Text::FontAtlas *> m_Fonts;
	Text::FontAtlas *m_DefaultFont = nullptr;
	std::unordered_map<std::string, WidgetFactory> m_Factories;

	TextureCache *m_TextureCache = nullptr;
	std::unordered_map<std::string, TextureIconBank *> m_IconBanks;
	std::unordered_map<std::string, Delegate<void()>> m_Commands;

	std::string m_CurrentDir;
	std::vector<std::string> m_IncludeStack;

	template <typename T> void Register(const std::string &typeName) {
		m_Factories[typeName] = [](std::string_view, Text::FontAtlas *) -> Unique<View> { return CreateUnique<T>(); };
	}

	void RegisterBuiltins();
};

} // namespace Aquila::UI::Core
