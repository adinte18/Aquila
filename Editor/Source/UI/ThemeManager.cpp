#include "UI/ThemeManager.h"
#include "imgui.h"

namespace Editor::UI {
ThemeManager &ThemeManager::Get() {
  static ThemeManager instance;
  return instance;
}
void ThemeManager::ApplyAquila2Theme() {
  ImGuiStyle &style = ImGui::GetStyle();
  ImVec4 *colors = style.Colors;

  colors[ImGuiCol_WindowBg] = HEXAtoIV4("#292929ff", 0.95f);
  colors[ImGuiCol_ChildBg] = HEXAtoIV4("#2a2a2a", 1.0f);
  colors[ImGuiCol_PopupBg] = HEXAtoIV4("#323232", 0.98f);
  colors[ImGuiCol_MenuBarBg] = HEXAtoIV4("#2a2a2a", 1.0f);
  colors[ImGuiCol_ModalWindowDimBg] = HEXAtoIV4("#000000", 0.6f);

  colors[ImGuiCol_Text] = HEXAtoIV4("#d2d2d2", 1.0f);
  colors[ImGuiCol_TextDisabled] = HEXAtoIV4("#7a7a7a", 1.0f);
  colors[ImGuiCol_TextSelectedBg] = HEXAtoIV4("#4CAF50", 0.3f);

  colors[ImGuiCol_Border] = HEXAtoIV4("#1a1a1a", 1.0f);
  colors[ImGuiCol_BorderShadow] = HEXAtoIV4("#000000", 0.0f);

  colors[ImGuiCol_FrameBg] = HEXAtoIV4("#5a5a5a", 1.0f);
  colors[ImGuiCol_FrameBgHovered] = HEXAtoIV4("#6a6a6a", 1.0f);
  colors[ImGuiCol_FrameBgActive] = HEXAtoIV4("#4CAF50", 0.3f);

  colors[ImGuiCol_TitleBg] = HEXAtoIV4("#323232", 1.0f);
  colors[ImGuiCol_TitleBgActive] = HEXAtoIV4("#454545", 1.0f);
  colors[ImGuiCol_TitleBgCollapsed] = HEXAtoIV4("#323232", 0.8f);

  colors[ImGuiCol_Button] = HEXAtoIV4("#5a5a5a", 1.0f);
  colors[ImGuiCol_ButtonHovered] = HEXAtoIV4("#6a6a6a", 1.0f);
  colors[ImGuiCol_ButtonActive] = HEXAtoIV4("#4CAF50", 0.8f);

  colors[ImGuiCol_Header] = HEXAtoIV4("#454545", 1.0f);
  colors[ImGuiCol_HeaderHovered] = HEXAtoIV4("#555555", 1.0f);
  colors[ImGuiCol_HeaderActive] = HEXAtoIV4("#4CAF50", 0.6f);

  colors[ImGuiCol_ScrollbarBg] = HEXAtoIV4("#2a2a2a", 1.0f);
  colors[ImGuiCol_ScrollbarGrab] = HEXAtoIV4("#5a5a5a", 1.0f);
  colors[ImGuiCol_ScrollbarGrabHovered] = HEXAtoIV4("#6a6a6a", 1.0f);
  colors[ImGuiCol_ScrollbarGrabActive] = HEXAtoIV4("#7a7a7a", 1.0f);

  colors[ImGuiCol_CheckMark] = HEXAtoIV4("#4CAF50", 1.0f);
  colors[ImGuiCol_SliderGrab] = HEXAtoIV4("#4CAF50", 1.0f);
  colors[ImGuiCol_SliderGrabActive] = HEXAtoIV4("#2E7D32", 1.0f);

  colors[ImGuiCol_Tab] = HEXAtoIV4("#383838", 1.0f);
  colors[ImGuiCol_TabHovered] = HEXAtoIV4("#484848", 1.0f);
  colors[ImGuiCol_TabActive] = HEXAtoIV4("#4CAF50", 0.4f);
  colors[ImGuiCol_TabUnfocused] = HEXAtoIV4("#383838", 0.9f);
  colors[ImGuiCol_TabUnfocusedActive] = HEXAtoIV4("#484848", 1.0f);

  colors[ImGuiCol_DockingPreview] = HEXAtoIV4("#4CAF50", 0.3f);
  colors[ImGuiCol_DockingEmptyBg] = HEXAtoIV4("#393939", 1.0f);
  colors[ImGuiCol_DragDropTarget] = HEXAtoIV4("#4CAF50", 0.6f);

  colors[ImGuiCol_ResizeGrip] = HEXAtoIV4("#5a5a5a", 1.0f);
  colors[ImGuiCol_ResizeGripHovered] = HEXAtoIV4("#6a6a6a", 1.0f);
  colors[ImGuiCol_ResizeGripActive] = HEXAtoIV4("#4CAF50", 0.7f);
  colors[ImGuiCol_Separator] = HEXAtoIV4("#1a1a1a", 1.0f);
  colors[ImGuiCol_SeparatorHovered] = HEXAtoIV4("#555555", 1.0f);
  colors[ImGuiCol_SeparatorActive] = HEXAtoIV4("#4CAF50", 0.8f);

  colors[ImGuiCol_PlotLines] = HEXAtoIV4("#4CAF50", 1.0f);
  colors[ImGuiCol_PlotLinesHovered] = HEXAtoIV4("#2E7D32", 1.0f);
  colors[ImGuiCol_PlotHistogram] = HEXAtoIV4("#81C784", 1.0f);
  colors[ImGuiCol_PlotHistogramHovered] = HEXAtoIV4("#66BB6A", 1.0f);
  colors[ImGuiCol_TableHeaderBg] = HEXAtoIV4("#454545", 1.0f);
  colors[ImGuiCol_TableBorderStrong] = HEXAtoIV4("#1a1a1a", 1.0f);
  colors[ImGuiCol_TableBorderLight] = HEXAtoIV4("#2a2a2a", 1.0f);
  colors[ImGuiCol_TableRowBg] = HEXAtoIV4("#000000", 0.0f);
  colors[ImGuiCol_TableRowBgAlt] = HEXAtoIV4("#ffffff", 0.04f);

  colors[ImGuiCol_NavHighlight] = HEXAtoIV4("#4CAF50", 0.6f);
  colors[ImGuiCol_NavWindowingHighlight] = HEXAtoIV4("#ffffff", 0.8f);
  colors[ImGuiCol_NavWindowingDimBg] = HEXAtoIV4("#cccccc", 0.2f);

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

void ThemeManager::ApplyAquilaTheme() {
  ImGuiStyle &style = ImGui::GetStyle();
  ImVec4 *colors = style.Colors;

  colors[ImGuiCol_WindowBg] = HEXAtoIV4("#1b1b1f", 0.95f);
  colors[ImGuiCol_ChildBg] = HEXAtoIV4("#23272e", 1.0f);
  colors[ImGuiCol_PopupBg] = HEXAtoIV4("#181a1f", 0.98f);
  colors[ImGuiCol_MenuBarBg] = HEXAtoIV4("#1e2126", 1.0f);
  colors[ImGuiCol_ModalWindowDimBg] = HEXAtoIV4("#000000", 0.4f);

  colors[ImGuiCol_Text] = HEXAtoIV4("#e8eaed", 1.0f);
  colors[ImGuiCol_TextDisabled] = HEXAtoIV4("#6c7b7f", 1.0f);
  colors[ImGuiCol_TextSelectedBg] = HEXAtoIV4("#4285f4", 0.4f);

  colors[ImGuiCol_Border] = HEXAtoIV4("#2c2f33", 1.0f);
  colors[ImGuiCol_BorderShadow] = HEXAtoIV4("#000000", 0.0f);

  colors[ImGuiCol_FrameBg] = HEXAtoIV4("#2f3136", 1.0f);
  colors[ImGuiCol_FrameBgHovered] = HEXAtoIV4("#393c43", 1.0f);
  colors[ImGuiCol_FrameBgActive] = HEXAtoIV4("#4285f4", 0.3f);

  colors[ImGuiCol_TitleBg] = HEXAtoIV4("#1b1b1f", 1.0f);
  colors[ImGuiCol_TitleBgActive] = HEXAtoIV4("#23272e", 1.0f);
  colors[ImGuiCol_TitleBgCollapsed] = HEXAtoIV4("#1b1b1f", 0.8f);

  colors[ImGuiCol_Button] = HEXAtoIV4("#4b5058", 1.0f);
  colors[ImGuiCol_ButtonHovered] = HEXAtoIV4("#565c66", 1.0f);
  colors[ImGuiCol_ButtonActive] = HEXAtoIV4("#4285f4", 0.8f);

  colors[ImGuiCol_Header] = HEXAtoIV4("#2f3136", 1.0f);
  colors[ImGuiCol_HeaderHovered] = HEXAtoIV4("#393c43", 1.0f);
  colors[ImGuiCol_HeaderActive] = HEXAtoIV4("#4285f4", 0.6f);

  colors[ImGuiCol_ScrollbarBg] = HEXAtoIV4("#1e2126", 1.0f);
  colors[ImGuiCol_ScrollbarGrab] = HEXAtoIV4("#4b5058", 1.0f);
  colors[ImGuiCol_ScrollbarGrabHovered] = HEXAtoIV4("#565c66", 1.0f);
  colors[ImGuiCol_ScrollbarGrabActive] = HEXAtoIV4("#6c7b7f", 1.0f);

  colors[ImGuiCol_CheckMark] = HEXAtoIV4("#4285f4", 1.0f);

  colors[ImGuiCol_SliderGrab] = HEXAtoIV4("#4285f4", 1.0f);
  colors[ImGuiCol_SliderGrabActive] = HEXAtoIV4("#1a73e8", 1.0f);

  colors[ImGuiCol_Tab] = HEXAtoIV4("#2f3136", 1.0f);
  colors[ImGuiCol_TabHovered] = HEXAtoIV4("#393c43", 1.0f);
  colors[ImGuiCol_TabActive] = HEXAtoIV4("#4285f4", 0.7f);
  colors[ImGuiCol_TabUnfocused] = HEXAtoIV4("#2f3136", 0.9f);
  colors[ImGuiCol_TabUnfocusedActive] = HEXAtoIV4("#393c43", 1.0f);

  colors[ImGuiCol_DockingPreview] = HEXAtoIV4("#4285f4", 0.3f);
  colors[ImGuiCol_DockingEmptyBg] = HEXAtoIV4("#1b1b1f", 1.0f);

  colors[ImGuiCol_ResizeGrip] = HEXAtoIV4("#2f3136", 1.0f);
  colors[ImGuiCol_ResizeGripHovered] = HEXAtoIV4("#4285f4", 0.6f);
  colors[ImGuiCol_ResizeGripActive] = HEXAtoIV4("#4285f4", 0.9f);

  colors[ImGuiCol_Separator] = HEXAtoIV4("#2c2f33", 1.0f);
  colors[ImGuiCol_SeparatorHovered] = HEXAtoIV4("#4285f4", 0.6f);
  colors[ImGuiCol_SeparatorActive] = HEXAtoIV4("#4285f4", 0.9f);

  colors[ImGuiCol_PlotLines] = HEXAtoIV4("#4285f4", 1.0f);
  colors[ImGuiCol_PlotLinesHovered] = HEXAtoIV4("#1a73e8", 1.0f);
  colors[ImGuiCol_PlotHistogram] = HEXAtoIV4("#34a853", 1.0f);
  colors[ImGuiCol_PlotHistogramHovered] = HEXAtoIV4("#137333", 1.0f);

  colors[ImGuiCol_TableHeaderBg] = HEXAtoIV4("#2f3136", 1.0f);
  colors[ImGuiCol_TableBorderStrong] = HEXAtoIV4("#393c43", 1.0f);
  colors[ImGuiCol_TableBorderLight] = HEXAtoIV4("#2c2f33", 1.0f);
  colors[ImGuiCol_TableRowBg] = HEXAtoIV4("#000000", 0.0f);
  colors[ImGuiCol_TableRowBgAlt] = HEXAtoIV4("#ffffff", 0.03f);

  colors[ImGuiCol_DragDropTarget] = HEXAtoIV4("#4285f4", 0.8f);

  colors[ImGuiCol_NavHighlight] = HEXAtoIV4("#4285f4", 0.8f);
  colors[ImGuiCol_NavWindowingHighlight] = HEXAtoIV4("#ffffff", 0.8f);
  colors[ImGuiCol_NavWindowingDimBg] = HEXAtoIV4("#cccccc", 0.2f);

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
void ThemeManager::ApplyDarkTheme() { ImGui::StyleColorsDark(); }

void ThemeManager::ApplyLightTheme() { ImGui::StyleColorsLight(); }

ImVec4 ThemeManager::HEXAtoIV4(const char *hex, float alpha) {
  int r, g, b;

  if (hex[0] == '#')
    hex++;

#ifdef AQUILA_PLATFORM_WINDOWS
  sscanf_s(hex, "%02x%02x%02x", &r, &g, &b);
#elif defined(AQUILA_PLATFORM_LINUX)
  sscanf(hex, "%02x%02x%02x", &r, &g, &b);
#endif

  return ImVec4(r / 255.0f, g / 255.0f, b / 255.0f, alpha);
}
} // namespace Editor::UI