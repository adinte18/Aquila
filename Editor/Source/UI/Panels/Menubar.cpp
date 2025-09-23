#include "UI/Panels/Menubar.h"
#include "Engine/Events/Event.h"
#include "Platform/Filesystem/VirtualFileSystem.h"
#include "UI/ThemeManager.h"
#include "UI/UI.h"

namespace Editor::Panels {

static int selectedFontSize = 1;
static ImFont *selectedFont = UI::FontManager::Get().GetFonts().Font12;
static int selectedThemeIndex = 0;
static std::string selectedTheme = "Aquila Dark";

void Menubar::Draw() {
  auto &fontManager = UI::FontManager::Get();

  if (ImGui::BeginMainMenuBar()) {

    if (ImGui::BeginMenu("File")) {
      if (ImGui::MenuItem(ICON_LC_FILE " New Scene", "Ctrl+N")) {
        m_NewSceneOpened = true;
      }
      if (ImGui::MenuItem(ICON_LC_FOLDER_OPEN " Open scene...", "Ctrl+O")) {
        nfdchar_t *outPath = nullptr;
        nfdresult_t result = NFD_OpenDialog("aqscene", nullptr, &outPath);
        if (result == NFD_OKAY) {
          std::string path = outPath;
          std::cout << "Selected scene file: " << path << std::endl;
          auto openSceneEvent = CreateUnique<Engine::OpenSceneEvent>();
          openSceneEvent->scenePath = outPath;

          Engine::EventBus::Get()->DispatchSync(std::move(openSceneEvent));
          NFDi_Free(outPath);
        } else if (result == NFD_CANCEL) {
          std::cout << "User cancelled scene open dialog." << std::endl;
        } else {
          std::cerr << "Error opening scene file: " << NFD_GetError()
                    << std::endl;
        }
      }
      if (ImGui::MenuItem(ICON_LC_SAVE_ALL " Save scene...", "Ctrl+S")) {
        auto saveSceneEvent = CreateUnique<Engine::SaveSceneEvent>();

        Engine::EventBus::Get()->DispatchSync(std::move(saveSceneEvent));
      }
      if (ImGui::MenuItem(ICON_LC_SAVE_ALL " Save scene as...", "Ctrl+S")) {
      }
      ImGui::Separator();
      if (ImGui::MenuItem(ICON_LC_CIRCLE_X " Exit", "Ctrl+X")) {
      }
      ImGui::EndMenu();
    }

    if (ImGui::BeginMenu("Edit")) {
      if (ImGui::MenuItem(ICON_LC_COG " Editor preferences", nullptr)) {
        m_ShowPreferences = true;
      }

      ImGui::EndMenu();
    }

    if (ImGui::BeginMenu("View")) {
      ImGui::EndMenu();
    }

    if (ImGui::BeginMenu("Help")) {
      if (ImGui::MenuItem(ICON_LC_CIRCLE_HELP " About", nullptr,
                          &m_AboutOpened)) {
      }
      ImGui::EndMenu();
    }

    ImGui::EndMainMenuBar();
  }

  if (m_NewSceneOpened) {
    ImGui::OpenPopup("Create new scene");
  }

  CreatePopup({500, 170});

  if (ImGui::BeginPopupModal("Create new scene", nullptr,
                             ImGuiWindowFlags_AlwaysAutoResize |
                                 ImGuiWindowFlags_NoMove)) {
    static char sceneName[128] = "";

    ImGui::PushFont(UI::FontManager::Get().GetFonts().Font32);
    ImGui::Text("Enter new scene name:");
    ImGui::PopFont();

    auto IsValidSceneName = [](const char *name) {
      for (const char *p = name; *p; p++) {
        char c = *p;
        if (!(isalnum(c) || c == '_')) {
          return false;
        }
      }
      return strlen(name) > 0;
    };

    ImGui::InputText("##scenename", sceneName, IM_ARRAYSIZE(sceneName));

    if (ImGui::Button("Create")) {
      if (IsValidSceneName(sceneName)) {
        auto newSceneEvent = CreateUnique<Engine::NewSceneEvent>();
        newSceneEvent->sceneName = sceneName;

        Engine::EventBus::Get()->DispatchSync(std::move(newSceneEvent));
        sceneName[0] = '\0';
        m_NewSceneOpened = false;
        ImGui::CloseCurrentPopup();
      }
    }

    ImGui::SameLine();
    if (ImGui::Button("Cancel")) {
      sceneName[0] = '\0';
      m_NewSceneOpened = false;
      ImGui::CloseCurrentPopup();
    }

    ImGui::EndPopup();
  }

  if (m_AboutOpened) {
    CreatePopup({500, 400});

    ImGui::Begin(ICON_LC_INFO " About Aquila", &m_AboutOpened,
                 ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize |
                     ImGuiWindowFlags_NoDocking);

    ImGui::PushFont(fontManager.GetFonts().Font48);
    ImGui::Text("Aquila Engine");
    ImGui::PopFont();

    ImGui::Separator();

    ImGui::PushFont(fontManager.GetFonts().Font32);
    ImGui::Text("Version 0.1.0 (Dev)");
    ImGui::PopFont();

    ImGui::Spacing();

    ImGui::PushFont(fontManager.GetFonts().Font24);
    ImGui::Text("© 2025 Aquila Engine");
    ImGui::Text("Built with Vulkan, ImGui, and C++");

    ImGui::Spacing();
    ImGui::Separator();

    ImGui::Text("System Info:");
    ImGui::BulletText("Platform: %s", Core::Platform::GetPlatformInfo().name);
    ImGui::BulletText("Backend: Vulkan");
    ImGui::BulletText("ImGui Version: %s", ImGui::GetVersion());

    ImGui::PopFont();

    ImGui::Spacing();
    ImGui::Separator();

    ImGui::PushFont(fontManager.GetFonts().Font28);
    if (ImGui::Button(ICON_LC_CHECK " OK", {100, 40})) {
      m_AboutOpened = false;
      ImGui::CloseCurrentPopup();
    }

    ImGui::PopFont();
    ImGui::End();
  }

  // ===========================
  // WINDOW: EDITOR PREFERENCES
  // ===========================
  if (m_ShowPreferences) {
    CreatePopup({500, 600});

    if (ImGui::Begin("Editor Preferences", &m_ShowPreferences)) {
      // Appearance Section
      if (ImGui::CollapsingHeader("Appearance",
                                  ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::Indent();

        // Font Size
        const char *fontSizes[] = {"12", "14", "16", "18", "20", "24",
                                   "28", "32", "40", "48", "64", "72"};
        ImGui::Text("Font Size:");
        ImGui::SetNextItemWidth(150);
        if (ImGui::Combo("##fontsize", &selectedFontSize, fontSizes,
                         IM_ARRAYSIZE(fontSizes))) {
          auto &fonts = UI::FontManager::Get().GetFonts();

          switch (selectedFontSize) {
          case 0:
            selectedFont = fonts.Font12;
            break;
          case 1:
            selectedFont = fonts.Font14;
            break;
          case 2:
            selectedFont = fonts.Font16;
            break;
          case 3:
            selectedFont = fonts.Font18;
            break;
          case 4:
            selectedFont = fonts.Font20;
            break;
          case 5:
            selectedFont = fonts.Font24;
            break;
          case 6:
            selectedFont = fonts.Font28;
            break;
          case 7:
            selectedFont = fonts.Font32;
            break;
          case 8:
            selectedFont = fonts.Font40;
            break;
          case 9:
            selectedFont = fonts.Font48;
            break;
          case 10:
            selectedFont = fonts.Font64;
            break;
          case 11:
            selectedFont = fonts.Font72;
            break;
          }
        }

        ImGui::Spacing();

        const char *themes[] = {"Aquila Dark", "Aquila Green", "ImGui Dark",
                                "ImGui Light"};
        if (ImGui::Combo("##theme", &selectedThemeIndex, themes,
                         IM_ARRAYSIZE(themes))) {
          selectedTheme = themes[selectedThemeIndex];
        }
      }

      ImGui::Unindent();
    }

    // Editor Section
    if (ImGui::CollapsingHeader("Editor")) {
      ImGui::Indent();

      static bool autoSave = true;
      static int autoSaveInterval = 300;

      ImGui::Checkbox("Auto Save", &autoSave);

      if (autoSave) {
        ImGui::Text("Auto Save Interval (seconds):");
        ImGui::SetNextItemWidth(150);
        ImGui::InputInt("##autosaveinterval", &autoSaveInterval, 30, 60);
      }

      static bool showGrid = true;
      ImGui::Checkbox("Show Grid", &showGrid);

      static bool snapToGrid = false;
      ImGui::Checkbox("Snap to Grid", &snapToGrid);

      ImGui::Unindent();
    }

    // Bottom buttons
    ImGui::Separator();
    ImGui::Spacing();

    if (ImGui::Button("Apply", ImVec2(100, 0))) {
      // Apply all settings
      // TODO: Implement settings application
      if (selectedTheme == "Aquila Dark") {
        UI::ThemeManager::Get().ApplyAquilaTheme();
      } else if (selectedTheme == "Aquila Green") {
        UI::ThemeManager::Get().ApplyAquila2Theme();
      } else if (selectedTheme == "ImGui Dark") {
        UI::ThemeManager::Get().ApplyDarkTheme();
      } else if (selectedTheme == "ImGui Light") {
        UI::ThemeManager::Get().ApplyLightTheme();
      }

      if (selectedFont)
        UI::FontManager::Get().SetCurrentFont(selectedFont);
    }

    ImGui::SameLine();

    if (ImGui::Button("Reset to Defaults", ImVec2(150, 0))) {
      selectedFont = UI::FontManager::Get().GetFonts().Font14;
      selectedFontSize = 1; // 14px
      selectedTheme = "Aquila Dark";
      selectedThemeIndex = 0;

      UI::FontManager::Get().SetCurrentFont(selectedFont);
      UI::ThemeManager::Get().ApplyAquilaTheme();
    }

    ImGui::SameLine();

    f32 remaining = ImGui::GetContentRegionAvail().x;
    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + remaining - 100);

    if (ImGui::Button("Close", ImVec2(100, 0))) {
      m_ShowPreferences = false;
    }
    ImGui::End();
  }
}
} // namespace Editor::Panels
