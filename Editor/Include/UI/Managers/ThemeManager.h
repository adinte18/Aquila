#ifndef EDITOR_THEME_MANAGER_H
#define EDITOR_THEME_MANAGER_H

#include "Core/EditorConfig.h"
#include <imgui.h>

namespace Editor::UI {

/**
 * @brief Manages visual themes for the editor
 *
 * Provides theme management including loading, saving, and applying
 * different color schemes and styles to the ImGui interface.
 */
class ThemeManager {
  public:
	struct ColorScheme {
		std::string name;
		ImVec4 colors[ImGuiCol_COUNT];
	};

	struct StyleSettings {
		ImVec2 windowPadding;
		f32 windowRounding;
		f32 windowBorderSize;
		ImVec2 framePadding;
		f32 frameRounding;
		f32 frameBorderSize;
		ImVec2 itemSpacing;
		ImVec2 itemInnerSpacing;
		f32 scrollbarSize;
		f32 scrollbarRounding;
		f32 grabMinSize;
		f32 grabRounding;
		f32 tabRounding;
	};

	static ThemeManager &Get();

	// Lifecycle
	void Initialize(Editor::Config::Theme defaultTheme = Editor::Config::Theme::Aquila2);
	void Shutdown();

	// Theme Management
	void ApplyTheme(Editor::Config::Theme theme);
	void ApplyCustomTheme(const ColorScheme &scheme, const StyleSettings &style);
	Editor::Config::Theme GetCurrentTheme() const { return m_CurrentTheme; }

	// Built-in Themes
	void ApplyAquilaTheme();
	void ApplyAquila2Theme();
	void ApplyDarkTheme();
	void ApplyLightTheme();

	// Custom Theme Management
	bool SaveCustomTheme(const std::string &name);
	bool LoadCustomTheme(const std::string &name);
	std::vector<std::string> GetCustomThemeNames() const;

	// Color Utilities
	static ImVec4 HexToColor(const char *hex, f32 alpha = 1.0f);
	static std::string ColorToHex(const ImVec4 &color);

	// UI
	void ShowThemeEditorWindow(bool *open);
	void ShowThemeSelector();

  private:
	ThemeManager() = default;
	~ThemeManager() = default;
	ThemeManager(const ThemeManager &) = delete;
	ThemeManager &operator=(const ThemeManager &) = delete;

	void ApplyStyleSettings(const StyleSettings &style);
	ColorScheme GetCurrentColorScheme() const;
	StyleSettings GetCurrentStyleSettings() const;

	bool m_Initialized = false;
	Editor::Config::Theme m_CurrentTheme = Editor::Config::Theme::Aquila2;
	std::unordered_map<std::string, ColorScheme> m_CustomThemes;
};

} // namespace Editor::UI

#endif // EDITOR_THEME_MANAGER_H
