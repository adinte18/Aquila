#ifndef EDITOR_FONT_MANAGER_H
#define EDITOR_FONT_MANAGER_H

#include "Aquila/Platform/PrimitiveTypes.h"
#include "Core/EditorConfig.h"
#include <imgui.h>

namespace Editor::UI {

/**
 * @brief Manages font loading and access for the editor
 *
 * Provides a centralized system for loading, caching, and accessing fonts
 * at different sizes. Supports both text and icon fonts.
 */
class FontManager {
  public:
	struct FontDescriptor {
		std::string name;
		std::string path;
		f32 size;
		ImFont *font = nullptr;
	};

	static FontManager &Get();

	// Lifecycle
	void Initialize(const Editor::Config::FontSettings &settings);
	void Shutdown();

	// Font Loading
	bool LoadDefaultFonts();
	bool LoadFont(const std::string &name, const std::string &path, f32 size);
	bool LoadFontWithIcons(const std::string &name, const std::string &fontPath, const std::string &iconPath, f32 size);

	// Font Access
	ImFont *GetFont(const std::string &name) const;
	ImFont *GetDefaultFont() const { return m_DefaultFont; }
	ImFont *GetFontAtSize(f32 size) const;

	// Predefined Size Access
	ImFont *GetFont12() const { return m_Font12; }
	ImFont *GetFont14() const { return m_Font14; }
	ImFont *GetFont16() const { return m_Font16; }
	ImFont *GetFont18() const { return m_Font18; }
	ImFont *GetFont20() const { return m_Font20; }
	ImFont *GetFont24() const { return m_Font24; }
	ImFont *GetFont28() const { return m_Font28; }
	ImFont *GetFont32() const { return m_Font32; }
	ImFont *GetFont40() const { return m_Font40; }
	ImFont *GetFont48() const { return m_Font48; }

	// Font Management
	void ReloadFonts();
	bool IsInitialized() const { return m_Initialized; }

	// Configuration
	void SetDefaultFontSize(f32 size);
	f32 GetDefaultFontSize() const { return m_DefaultFontSize; }

	struct SystemFont {
		std::string name;		// Display name (e.g., "Roboto Regular")
		std::string family;		// Font family (e.g., "Roboto")
		std::string path;		// Full path to font file
		std::string style;		// Style (Regular, Bold, Italic, etc.)
		bool isTrueType = true; // TTF vs OTF
	};

	void ScanSystemFonts();
	const std::vector<SystemFont> &GetSystemFonts() const { return m_SystemFonts; }
	const SystemFont *FindSystemFont(const std::string &familyName, const std::string &style = "Regular") const;
	bool LoadSystemFont(const std::string &familyName, const std::string &style, f32 size);

	// UI Helpers
	void ShowFontSelector(const char *label, std::string &selectedFont, std::string &selectedStyle);

  private:
	FontManager() = default;
	~FontManager() = default;
	FontManager(const FontManager &) = delete;
	FontManager &operator=(const FontManager &) = delete;

	ImFont *LoadSingleFont(const std::string &path, f32 size, bool mergeIcons = true);
	void CachePredefinedFonts();

	// System font helpers
	void ScanFontDirectory(const std::string &directory);
	std::string ExtractFontFamily(const std::string &filename) const;
	std::string ExtractFontStyle(const std::string &filename) const;

	bool m_Initialized = false;
	Editor::Config::FontSettings m_Settings;

	// Font cache
	std::unordered_map<std::string, FontDescriptor> m_Fonts;

	// System fonts
	std::vector<SystemFont> m_SystemFonts;
	bool m_SystemFontsScanned = false;

	// Predefined size fonts
	ImFont *m_DefaultFont = nullptr;
	ImFont *m_Font12 = nullptr;
	ImFont *m_Font14 = nullptr;
	ImFont *m_Font16 = nullptr;
	ImFont *m_Font18 = nullptr;
	ImFont *m_Font20 = nullptr;
	ImFont *m_Font24 = nullptr;
	ImFont *m_Font28 = nullptr;
	ImFont *m_Font32 = nullptr;
	ImFont *m_Font40 = nullptr;
	ImFont *m_Font48 = nullptr;

	f32 m_DefaultFontSize = 16.0f;
};

} // namespace Editor::UI

#endif // EDITOR_FONT_MANAGER_H
