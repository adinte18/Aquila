#include "UI/Managers/ThemeManager.h"
#include "Aquila/Core/Defines.h"
#include <imgui.h>

namespace Editor::UI {

ThemeManager &ThemeManager::Get() {
	static ThemeManager instance;
	return instance;
}

void ThemeManager::Initialize(Editor::Config::Theme defaultTheme) {
	if (m_Initialized) {
		AQUILA_LOG_WARNING("ThemeManager already initialized");
		return;
	}

	AQUILA_LOG_INFO("Initializing ThemeManager...");

	ApplyTheme(defaultTheme);

	m_Initialized = true;
	AQUILA_LOG_INFO("ThemeManager initialized with theme: {}", static_cast<int>(defaultTheme));
}

void ThemeManager::Shutdown() {
	if (!m_Initialized) {
		return;
	}

	m_CustomThemes.clear();
	m_Initialized = false;
	AQUILA_LOG_INFO("ThemeManager shut down");
}

void ThemeManager::ApplyTheme(Editor::Config::Theme theme) {
	switch (theme) {
	case Editor::Config::Theme::Aquila:
		ApplyAquilaTheme();
		break;
	case Editor::Config::Theme::Aquila2:
		ApplyAquila2Theme();
		break;
	case Editor::Config::Theme::Dark:
		ApplyDarkTheme();
		break;
	case Editor::Config::Theme::Light:
		ApplyLightTheme();
		break;
	case Editor::Config::Theme::Custom:
		AQUILA_LOG_WARNING("Custom theme selected but no custom theme loaded");
		ApplyAquila2Theme();
		break;
	}

	m_CurrentTheme = theme;
	AQUILA_LOG_INFO("Applied theme: {}", static_cast<int>(theme));
}

void ThemeManager::ApplyCustomTheme(const ColorScheme &scheme, const StyleSettings &style) {
	ImGuiStyle &imgui_style = ImGui::GetStyle();

	// Apply colors
	for (int i = 0; i < ImGuiCol_COUNT; i++) {
		imgui_style.Colors[i] = scheme.colors[i];
	}

	// Apply style settings
	ApplyStyleSettings(style);

	m_CurrentTheme = Editor::Config::Theme::Custom;
	AQUILA_LOG_INFO("Applied custom theme: {}", scheme.name);
}

void ThemeManager::ApplyAquilaTheme() {
	ImGuiStyle &style = ImGui::GetStyle();
	ImVec4 *colors = style.Colors;

	colors[ImGuiCol_WindowBg] = HexToColor("#1b1b1f", 0.95f);
	colors[ImGuiCol_ChildBg] = HexToColor("#23272e", 1.0f);
	colors[ImGuiCol_PopupBg] = HexToColor("#181a1f", 0.98f);
	colors[ImGuiCol_MenuBarBg] = HexToColor("#1e2126", 1.0f);
	colors[ImGuiCol_ModalWindowDimBg] = HexToColor("#000000", 0.4f);

	colors[ImGuiCol_Text] = HexToColor("#e8eaed", 1.0f);
	colors[ImGuiCol_TextDisabled] = HexToColor("#6c7b7f", 1.0f);
	colors[ImGuiCol_TextSelectedBg] = HexToColor("#4285f4", 0.4f);

	colors[ImGuiCol_Border] = HexToColor("#2c2f33", 1.0f);
	colors[ImGuiCol_BorderShadow] = HexToColor("#000000", 0.0f);

	colors[ImGuiCol_FrameBg] = HexToColor("#2f3136", 1.0f);
	colors[ImGuiCol_FrameBgHovered] = HexToColor("#393c43", 1.0f);
	colors[ImGuiCol_FrameBgActive] = HexToColor("#4285f4", 0.3f);

	colors[ImGuiCol_TitleBg] = HexToColor("#1b1b1f", 1.0f);
	colors[ImGuiCol_TitleBgActive] = HexToColor("#23272e", 1.0f);
	colors[ImGuiCol_TitleBgCollapsed] = HexToColor("#1b1b1f", 0.8f);

	colors[ImGuiCol_Button] = HexToColor("#4b5058", 1.0f);
	colors[ImGuiCol_ButtonHovered] = HexToColor("#565c66", 1.0f);
	colors[ImGuiCol_ButtonActive] = HexToColor("#4285f4", 0.8f);

	colors[ImGuiCol_Header] = HexToColor("#2f3136", 1.0f);
	colors[ImGuiCol_HeaderHovered] = HexToColor("#393c43", 1.0f);
	colors[ImGuiCol_HeaderActive] = HexToColor("#4285f4", 0.6f);

	colors[ImGuiCol_ScrollbarBg] = HexToColor("#1e2126", 1.0f);
	colors[ImGuiCol_ScrollbarGrab] = HexToColor("#4b5058", 1.0f);
	colors[ImGuiCol_ScrollbarGrabHovered] = HexToColor("#565c66", 1.0f);
	colors[ImGuiCol_ScrollbarGrabActive] = HexToColor("#6c7b7f", 1.0f);

	colors[ImGuiCol_CheckMark] = HexToColor("#4285f4", 1.0f);
	colors[ImGuiCol_SliderGrab] = HexToColor("#4285f4", 1.0f);
	colors[ImGuiCol_SliderGrabActive] = HexToColor("#1a73e8", 1.0f);

	colors[ImGuiCol_Tab] = HexToColor("#2f3136", 1.0f);
	colors[ImGuiCol_TabHovered] = HexToColor("#393c43", 1.0f);
	colors[ImGuiCol_TabActive] = HexToColor("#4285f4", 0.7f);
	colors[ImGuiCol_TabUnfocused] = HexToColor("#2f3136", 0.9f);
	colors[ImGuiCol_TabUnfocusedActive] = HexToColor("#393c43", 1.0f);

	colors[ImGuiCol_DockingPreview] = HexToColor("#4285f4", 0.3f);
	colors[ImGuiCol_DockingEmptyBg] = HexToColor("#1b1b1f", 1.0f);

	colors[ImGuiCol_ResizeGrip] = HexToColor("#2f3136", 1.0f);
	colors[ImGuiCol_ResizeGripHovered] = HexToColor("#4285f4", 0.6f);
	colors[ImGuiCol_ResizeGripActive] = HexToColor("#4285f4", 0.9f);

	colors[ImGuiCol_Separator] = HexToColor("#2c2f33", 1.0f);
	colors[ImGuiCol_SeparatorHovered] = HexToColor("#4285f4", 0.6f);
	colors[ImGuiCol_SeparatorActive] = HexToColor("#4285f4", 0.9f);

	colors[ImGuiCol_PlotLines] = HexToColor("#4285f4", 1.0f);
	colors[ImGuiCol_PlotLinesHovered] = HexToColor("#1a73e8", 1.0f);
	colors[ImGuiCol_PlotHistogram] = HexToColor("#34a853", 1.0f);
	colors[ImGuiCol_PlotHistogramHovered] = HexToColor("#137333", 1.0f);

	colors[ImGuiCol_TableHeaderBg] = HexToColor("#2f3136", 1.0f);
	colors[ImGuiCol_TableBorderStrong] = HexToColor("#393c43", 1.0f);
	colors[ImGuiCol_TableBorderLight] = HexToColor("#2c2f33", 1.0f);
	colors[ImGuiCol_TableRowBg] = HexToColor("#000000", 0.0f);
	colors[ImGuiCol_TableRowBgAlt] = HexToColor("#ffffff", 0.03f);

	colors[ImGuiCol_DragDropTarget] = HexToColor("#4285f4", 0.8f);

	colors[ImGuiCol_NavHighlight] = HexToColor("#4285f4", 0.8f);
	colors[ImGuiCol_NavWindowingHighlight] = HexToColor("#ffffff", 0.8f);
	colors[ImGuiCol_NavWindowingDimBg] = HexToColor("#cccccc", 0.2f);

	// Style settings
	style.WindowPadding = ImVec2(6, 6);
	style.WindowRounding = 0.0f;
	style.WindowBorderSize = 1.0f;
	style.WindowMinSize = ImVec2(200, 100);
	style.WindowTitleAlign = ImVec2(0.5f, 0.5f);
	style.WindowMenuButtonPosition = ImGuiDir_Left;

	style.ChildRounding = 6.0f;
	style.ChildBorderSize = 1.0f;
	style.PopupRounding = 6.0f;
	style.PopupBorderSize = 1.0f;

	style.FramePadding = ImVec2(6, 3);
	style.FrameRounding = 2.0f;
	style.FrameBorderSize = 0.0f;

	style.ItemSpacing = ImVec2(7, 4);
	style.ItemInnerSpacing = ImVec2(4, 2);
	style.CellPadding = ImVec2(6, 3);
	style.IndentSpacing = 18.0f;

	style.ScrollbarSize = 10.0f;
	style.ScrollbarRounding = 4.0f;
	style.GrabMinSize = 10.0f;
	style.GrabRounding = 5.0f;

	style.TabRounding = 4.0f;
	style.TabBorderSize = 0.0f;
	style.ButtonTextAlign = ImVec2(0.5f, 0.5f);
	style.SelectableTextAlign = ImVec2(0.0f, 0.0f);

	style.Alpha = 1.0f;
	style.DisabledAlpha = 0.4f;
	style.AntiAliasedLines = true;
	style.AntiAliasedFill = true;
	style.CurveTessellationTol = 1.25f;
	style.CircleTessellationMaxError = 0.3f;
}

void ThemeManager::ApplyAquila2Theme() {
	ImGuiStyle &style = ImGui::GetStyle();
	ImVec4 *colors = style.Colors;

	colors[ImGuiCol_WindowBg] = HexToColor("#292929", 0.95f);
	colors[ImGuiCol_ChildBg] = HexToColor("#2a2a2a", 1.0f);
	colors[ImGuiCol_PopupBg] = HexToColor("#323232", 0.98f);
	colors[ImGuiCol_MenuBarBg] = HexToColor("#2a2a2a", 1.0f);
	colors[ImGuiCol_ModalWindowDimBg] = HexToColor("#000000", 0.6f);

	colors[ImGuiCol_Text] = HexToColor("#d2d2d2", 1.0f);
	colors[ImGuiCol_TextDisabled] = HexToColor("#7a7a7a", 1.0f);
	colors[ImGuiCol_TextSelectedBg] = HexToColor("#4CAF50", 0.3f);

	colors[ImGuiCol_Border] = HexToColor("#1a1a1a", 1.0f);
	colors[ImGuiCol_BorderShadow] = HexToColor("#000000", 0.0f);

	colors[ImGuiCol_FrameBg] = HexToColor("#5a5a5a", 1.0f);
	colors[ImGuiCol_FrameBgHovered] = HexToColor("#6a6a6a", 1.0f);
	colors[ImGuiCol_FrameBgActive] = HexToColor("#4CAF50", 0.3f);

	colors[ImGuiCol_TitleBg] = HexToColor("#323232", 1.0f);
	colors[ImGuiCol_TitleBgActive] = HexToColor("#454545", 1.0f);
	colors[ImGuiCol_TitleBgCollapsed] = HexToColor("#323232", 0.8f);

	colors[ImGuiCol_Button] = HexToColor("#5a5a5a", 1.0f);
	colors[ImGuiCol_ButtonHovered] = HexToColor("#6a6a6a", 1.0f);
	colors[ImGuiCol_ButtonActive] = HexToColor("#4CAF50", 0.8f);

	colors[ImGuiCol_Header] = HexToColor("#454545", 1.0f);
	colors[ImGuiCol_HeaderHovered] = HexToColor("#555555", 1.0f);
	colors[ImGuiCol_HeaderActive] = HexToColor("#4CAF50", 0.6f);

	colors[ImGuiCol_ScrollbarBg] = HexToColor("#2a2a2a", 1.0f);
	colors[ImGuiCol_ScrollbarGrab] = HexToColor("#5a5a5a", 1.0f);
	colors[ImGuiCol_ScrollbarGrabHovered] = HexToColor("#6a6a6a", 1.0f);
	colors[ImGuiCol_ScrollbarGrabActive] = HexToColor("#7a7a7a", 1.0f);

	colors[ImGuiCol_CheckMark] = HexToColor("#4CAF50", 1.0f);
	colors[ImGuiCol_SliderGrab] = HexToColor("#4CAF50", 1.0f);
	colors[ImGuiCol_SliderGrabActive] = HexToColor("#2E7D32", 1.0f);

	colors[ImGuiCol_Tab] = HexToColor("#383838", 1.0f);
	colors[ImGuiCol_TabHovered] = HexToColor("#484848", 1.0f);
	colors[ImGuiCol_TabActive] = HexToColor("#4CAF50", 0.4f);
	colors[ImGuiCol_TabUnfocused] = HexToColor("#383838", 0.9f);
	colors[ImGuiCol_TabUnfocusedActive] = HexToColor("#484848", 1.0f);

	colors[ImGuiCol_DockingPreview] = HexToColor("#4CAF50", 0.3f);
	colors[ImGuiCol_DockingEmptyBg] = HexToColor("#393939", 1.0f);
	colors[ImGuiCol_DragDropTarget] = HexToColor("#4CAF50", 0.6f);

	colors[ImGuiCol_ResizeGrip] = HexToColor("#5a5a5a", 1.0f);
	colors[ImGuiCol_ResizeGripHovered] = HexToColor("#6a6a6a", 1.0f);
	colors[ImGuiCol_ResizeGripActive] = HexToColor("#4CAF50", 0.7f);

	colors[ImGuiCol_Separator] = HexToColor("#1a1a1a", 1.0f);
	colors[ImGuiCol_SeparatorHovered] = HexToColor("#555555", 1.0f);
	colors[ImGuiCol_SeparatorActive] = HexToColor("#4CAF50", 0.8f);

	colors[ImGuiCol_PlotLines] = HexToColor("#4CAF50", 1.0f);
	colors[ImGuiCol_PlotLinesHovered] = HexToColor("#2E7D32", 1.0f);
	colors[ImGuiCol_PlotHistogram] = HexToColor("#81C784", 1.0f);
	colors[ImGuiCol_PlotHistogramHovered] = HexToColor("#66BB6A", 1.0f);

	colors[ImGuiCol_TableHeaderBg] = HexToColor("#454545", 1.0f);
	colors[ImGuiCol_TableBorderStrong] = HexToColor("#1a1a1a", 1.0f);
	colors[ImGuiCol_TableBorderLight] = HexToColor("#2a2a2a", 1.0f);
	colors[ImGuiCol_TableRowBg] = HexToColor("#000000", 0.0f);
	colors[ImGuiCol_TableRowBgAlt] = HexToColor("#ffffff", 0.04f);

	colors[ImGuiCol_NavHighlight] = HexToColor("#4CAF50", 0.6f);
	colors[ImGuiCol_NavWindowingHighlight] = HexToColor("#ffffff", 0.8f);
	colors[ImGuiCol_NavWindowingDimBg] = HexToColor("#cccccc", 0.2f);

	// Style settings (same as Aquila)
	style.WindowPadding = ImVec2(6, 6);
	style.WindowRounding = 0.0f;
	style.WindowBorderSize = 1.0f;
	style.WindowMinSize = ImVec2(200, 100);
	style.WindowTitleAlign = ImVec2(0.5f, 0.5f);
	style.WindowMenuButtonPosition = ImGuiDir_Left;

	style.ChildRounding = 6.0f;
	style.ChildBorderSize = 1.0f;
	style.PopupRounding = 6.0f;
	style.PopupBorderSize = 1.0f;

	style.FramePadding = ImVec2(6, 3);
	style.FrameRounding = 2.0f;
	style.FrameBorderSize = 0.0f;

	style.ItemSpacing = ImVec2(7, 4);
	style.ItemInnerSpacing = ImVec2(4, 2);
	style.CellPadding = ImVec2(6, 3);
	style.IndentSpacing = 18.0f;

	style.ScrollbarSize = 10.0f;
	style.ScrollbarRounding = 4.0f;
	style.GrabMinSize = 10.0f;
	style.GrabRounding = 5.0f;

	style.TabRounding = 4.0f;
	style.TabBorderSize = 0.0f;
	style.ButtonTextAlign = ImVec2(0.5f, 0.5f);
	style.SelectableTextAlign = ImVec2(0.0f, 0.0f);

	style.Alpha = 1.0f;
	style.DisabledAlpha = 0.4f;
	style.AntiAliasedLines = true;
	style.AntiAliasedFill = true;
	style.CurveTessellationTol = 1.25f;
	style.CircleTessellationMaxError = 0.3f;
}

void ThemeManager::ApplyDarkTheme() {
	ImGui::StyleColorsDark();
	AQUILA_LOG_INFO("Applied ImGui Dark theme");
}

void ThemeManager::ApplyLightTheme() {
	ImGui::StyleColorsLight();
	AQUILA_LOG_INFO("Applied ImGui Light theme");
}

bool ThemeManager::SaveCustomTheme(const std::string &name) {
	ColorScheme scheme;
	scheme.name = name;

	ImGuiStyle &style = ImGui::GetStyle();
	for (int i = 0; i < ImGuiCol_COUNT; i++) {
		scheme.colors[i] = style.Colors[i];
	}

	m_CustomThemes[name] = scheme;

	// TODO: Save to file
	AQUILA_LOG_INFO("Saved custom theme: {}", name);
	return true;
}

bool ThemeManager::LoadCustomTheme(const std::string &name) {
	auto it = m_CustomThemes.find(name);
	if (it == m_CustomThemes.end()) {
		AQUILA_LOG_WARNING("Custom theme not found: {}", name);
		return false;
	}

	ImGuiStyle &style = ImGui::GetStyle();
	for (int i = 0; i < ImGuiCol_COUNT; i++) {
		style.Colors[i] = it->second.colors[i];
	}

	m_CurrentTheme = Editor::Config::Theme::Custom;
	AQUILA_LOG_INFO("Loaded custom theme: {}", name);
	return true;
}

std::vector<std::string> ThemeManager::GetCustomThemeNames() const {
	std::vector<std::string> names;
	names.reserve(m_CustomThemes.size());

	for (const auto &[name, _] : m_CustomThemes) {
		names.push_back(name);
	}

	return names;
}

ImVec4 ThemeManager::HexToColor(const char *hex, f32 alpha) {
	int r, g, b;

	if (hex[0] == '#') {
		hex++;
	}

#ifdef AQUILA_PLATFORM_WINDOWS
	sscanf_s(hex, "%02x%02x%02x", &r, &g, &b);
#else
	sscanf(hex, "%02x%02x%02x", &r, &g, &b);
#endif

	return ImVec4(r / 255.0f, g / 255.0f, b / 255.0f, alpha);
}

std::string ThemeManager::ColorToHex(const ImVec4 &color) {
	char hex[8];
	sprintf(hex, "#%02x%02x%02x", static_cast<int>(color.x * 255), static_cast<int>(color.y * 255),
			static_cast<int>(color.z * 255));
	return std::string(hex);
}

void ThemeManager::ShowThemeEditorWindow(bool *open) {
	if (!open || !*open) {
		return;
	}

	ImGui::Begin("Theme Editor", open, ImGuiWindowFlags_AlwaysAutoResize);

	ImGuiStyle &style = ImGui::GetStyle();
	ImVec4 *colors = style.Colors;

	// Theme selector
	if (ImGui::BeginCombo("Theme Preset", "Select...")) {
		if (ImGui::Selectable("Aquila"))
			ApplyTheme(Editor::Config::Theme::Aquila);
		if (ImGui::Selectable("Aquila 2"))
			ApplyTheme(Editor::Config::Theme::Aquila2);
		if (ImGui::Selectable("Dark"))
			ApplyTheme(Editor::Config::Theme::Dark);
		if (ImGui::Selectable("Light"))
			ApplyTheme(Editor::Config::Theme::Light);
		ImGui::EndCombo();
	}

	ImGui::Separator();

	// Color sections
	if (ImGui::CollapsingHeader("Colors")) {
		for (int i = 0; i < ImGuiCol_COUNT; i++) {
			const char *name = ImGui::GetStyleColorName(i);
			ImGui::ColorEdit4(name, (f32 *)&colors[i], ImGuiColorEditFlags_AlphaBar);
		}
	}

	if (ImGui::CollapsingHeader("Window")) {
		ImGui::SliderFloat2("Window Padding", (f32 *)&style.WindowPadding, 0.0f, 20.0f);
		ImGui::SliderFloat("Window Rounding", &style.WindowRounding, 0.0f, 20.0f);
		ImGui::SliderFloat("Window Border Size", &style.WindowBorderSize, 0.0f, 5.0f);
		ImGui::SliderFloat2("Window Min Size", (f32 *)&style.WindowMinSize, 0.0f, 500.0f);
	}

	if (ImGui::CollapsingHeader("Frames")) {
		ImGui::SliderFloat2("Frame Padding", (f32 *)&style.FramePadding, 0.0f, 20.0f);
		ImGui::SliderFloat("Frame Rounding", &style.FrameRounding, 0.0f, 20.0f);
		ImGui::SliderFloat("Frame Border Size", &style.FrameBorderSize, 0.0f, 5.0f);
	}

	if (ImGui::CollapsingHeader("Spacing")) {
		ImGui::SliderFloat2("Item Spacing", (f32 *)&style.ItemSpacing, 0.0f, 20.0f);
		ImGui::SliderFloat2("Item Inner Spacing", (f32 *)&style.ItemInnerSpacing, 0.0f, 20.0f);
		ImGui::SliderFloat2("Cell Padding", (f32 *)&style.CellPadding, 0.0f, 20.0f);
		ImGui::SliderFloat("Indent Spacing", &style.IndentSpacing, 0.0f, 50.0f);
	}

	if (ImGui::CollapsingHeader("Scrollbar & Grab")) {
		ImGui::SliderFloat("Scrollbar Size", &style.ScrollbarSize, 1.0f, 30.0f);
		ImGui::SliderFloat("Scrollbar Rounding", &style.ScrollbarRounding, 0.0f, 20.0f);
		ImGui::SliderFloat("Grab Min Size", &style.GrabMinSize, 1.0f, 30.0f);
		ImGui::SliderFloat("Grab Rounding", &style.GrabRounding, 0.0f, 20.0f);
	}

	if (ImGui::CollapsingHeader("Tabs")) {
		ImGui::SliderFloat("Tab Rounding", &style.TabRounding, 0.0f, 20.0f);
		ImGui::SliderFloat("Tab Border Size", &style.TabBorderSize, 0.0f, 5.0f);
	}

	ImGui::Separator();

	// Custom theme management
	static char themeNameBuffer[128] = "";
	ImGui::InputText("Theme Name", themeNameBuffer, sizeof(themeNameBuffer));
	ImGui::SameLine();
	if (ImGui::Button("Save Custom")) {
		if (strlen(themeNameBuffer) > 0) {
			SaveCustomTheme(themeNameBuffer);
		}
	}

	// List custom themes
	auto customThemes = GetCustomThemeNames();
	if (!customThemes.empty()) {
		ImGui::Text("Custom Themes:");
		for (const auto &themeName : customThemes) {
			if (ImGui::Selectable(themeName.c_str())) {
				LoadCustomTheme(themeName);
			}
		}
	}

	ImGui::End();
}

void ThemeManager::ShowThemeSelector() {
	if (ImGui::BeginCombo("Theme", "Select Theme")) {
		if (ImGui::Selectable("Aquila", m_CurrentTheme == Editor::Config::Theme::Aquila)) {
			ApplyTheme(Editor::Config::Theme::Aquila);
		}
		if (ImGui::Selectable("Aquila 2", m_CurrentTheme == Editor::Config::Theme::Aquila2)) {
			ApplyTheme(Editor::Config::Theme::Aquila2);
		}
		if (ImGui::Selectable("Dark", m_CurrentTheme == Editor::Config::Theme::Dark)) {
			ApplyTheme(Editor::Config::Theme::Dark);
		}
		if (ImGui::Selectable("Light", m_CurrentTheme == Editor::Config::Theme::Light)) {
			ApplyTheme(Editor::Config::Theme::Light);
		}

		auto customThemes = GetCustomThemeNames();
		if (!customThemes.empty()) {
			ImGui::Separator();
			for (const auto &name : customThemes) {
				if (ImGui::Selectable(name.c_str())) {
					LoadCustomTheme(name);
				}
			}
		}

		ImGui::EndCombo();
	}
}

void ThemeManager::ApplyStyleSettings(const StyleSettings &settings) {
	ImGuiStyle &style = ImGui::GetStyle();

	style.WindowPadding = settings.windowPadding;
	style.WindowRounding = settings.windowRounding;
	style.WindowBorderSize = settings.windowBorderSize;
	style.FramePadding = settings.framePadding;
	style.FrameRounding = settings.frameRounding;
	style.FrameBorderSize = settings.frameBorderSize;
	style.ItemSpacing = settings.itemSpacing;
	style.ItemInnerSpacing = settings.itemInnerSpacing;
	style.ScrollbarSize = settings.scrollbarSize;
	style.ScrollbarRounding = settings.scrollbarRounding;
	style.GrabMinSize = settings.grabMinSize;
	style.GrabRounding = settings.grabRounding;
	style.TabRounding = settings.tabRounding;
}

ThemeManager::ColorScheme ThemeManager::GetCurrentColorScheme() const {
	ColorScheme scheme;
	scheme.name = "Current";

	ImGuiStyle &style = ImGui::GetStyle();
	for (int i = 0; i < ImGuiCol_COUNT; i++) {
		scheme.colors[i] = style.Colors[i];
	}

	return scheme;
}

ThemeManager::StyleSettings ThemeManager::GetCurrentStyleSettings() const {
	ImGuiStyle &style = ImGui::GetStyle();

	StyleSettings settings;
	settings.windowPadding = style.WindowPadding;
	settings.windowRounding = style.WindowRounding;
	settings.windowBorderSize = style.WindowBorderSize;
	settings.framePadding = style.FramePadding;
	settings.frameRounding = style.FrameRounding;
	settings.frameBorderSize = style.FrameBorderSize;
	settings.itemSpacing = style.ItemSpacing;
	settings.itemInnerSpacing = style.ItemInnerSpacing;
	settings.scrollbarSize = style.ScrollbarSize;
	settings.scrollbarRounding = style.ScrollbarRounding;
	settings.grabMinSize = style.GrabMinSize;
	settings.grabRounding = style.GrabRounding;
	settings.tabRounding = style.TabRounding;

	return settings;
}

} // namespace Editor::UI
