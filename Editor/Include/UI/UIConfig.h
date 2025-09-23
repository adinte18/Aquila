#ifndef UI_CONFIG_H
#define UI_CONFIG_H

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_vulkan.h"
#include "lucide.h"
#include <string>

namespace Editor::UI {
struct Config {
  static inline ImGuiDockNodeFlags DockspaceFlags = ImGuiDockNodeFlags_None;

  static inline ImGuiWindowFlags WindowFlags =
      ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse |
      ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoBringToFrontOnFocus |
      ImGuiWindowFlags_NoNavFocus;

  static inline ImGuiConfigFlags ConfigFlags =
      ImGuiConfigFlags_NavEnableKeyboard | ImGuiConfigFlags_NavEnableGamepad |
      ImGuiConfigFlags_DockingEnable | ImGuiConfigFlags_NoMouseCursorChange;

  static inline float WindowRounding = 0.0f;
  static inline float WindowBorderSize = 0.0f;

  static inline const char *DockspaceName = "Aquila Dockspace";
  static inline ImGuiID DockspaceID = 0;

  static inline float BaseFontSize = 16.0f;

  static inline std::string CurrentPath = "/Assets";

#if defined(AQUILA_PLATFORM_WINDOWS)
  static inline const char *MainFontPath = R"(C:/Windows/Fonts/segoeui.ttf)";
  static inline std::string IconsFontPath =
      std::string(ENGINE_RESSOURCES_PATH) + "/Fonts/lucide.ttf";
#elif defined(AQUILA_PLATFORM_LINUX)
  static inline const char *MainFontPath =
      R"(/home/adinte/Desktop/Aquila/Aquila/Resources/Engine Ressources/Roboto-VariableFont_wdth,wght.ttf)";
  static inline std::string IconsFontPath =
      std::string(ENGINE_RESSOURCES_PATH) + "/Fonts/lucide.ttf";
#endif

  static inline constexpr ImWchar IconsRanges[3] = {ICON_MIN_LC, ICON_MAX_16_LC,
                                                    0};
};
} // namespace Editor::UI

#endif // UI_CONFIG_H
