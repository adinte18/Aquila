#include "UI/Panels/Menubar.h"
#include "Engine/Controller.h"
#include "Engine/EditorCamera.h"
#include "Engine/Events/Event.h"
#include "Platform/Filesystem/VirtualFileSystem.h"
#include "UI/ThemeManager.h"
#include "UI/UI.h"
#include "imgui.h"

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

  if (m_ShowPreferences) {
    CreatePopup({500, 600});

    if (ImGui::Begin("Editor Preferences", &m_ShowPreferences)) {

      if (ImGui::CollapsingHeader("Appearance",
                                  ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::Indent();

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

      ImGuiStyle &style = ImGui::GetStyle();

      ImGui::SliderFloat("Frame Rounding", &style.FrameRounding, 0.0f, 12.0f,
                         "%.1f");
      ImGui::SliderFloat("Window Rounding", &style.WindowRounding, 0.0f, 12.0f,
                         "%.1f");
      ImGui::SliderFloat("Scrollbar Size", &style.ScrollbarSize, 10.0f, 30.0f,
                         "%.1f");
      ImGui::SliderFloat("Window Padding X", &style.WindowPadding.x, 0.0f,
                         30.0f, "%.1f");
      ImGui::SliderFloat("Window Padding Y", &style.WindowPadding.y, 0.0f,
                         30.0f, "%.1f");
      ImGui::SliderFloat("Item Spacing X", &style.ItemSpacing.x, 0.0f, 20.0f,
                         "%.1f");
      ImGui::SliderFloat("Item Spacing Y", &style.ItemSpacing.y, 0.0f, 20.0f,
                         "%.1f");

      ImGui::ColorEdit3("Window Bg", (float *)&style.Colors[ImGuiCol_WindowBg]);
      ImGui::ColorEdit3("Button", (float *)&style.Colors[ImGuiCol_Button]);
      ImGui::ColorEdit3("Header", (float *)&style.Colors[ImGuiCol_Header]);
      ImGui::ColorEdit3("FrameBg", (float *)&style.Colors[ImGuiCol_FrameBg]);

      ImGui::Unindent();
    }

    if (ImGui::CollapsingHeader("Camera")) {
      ImGui::Indent();

      auto &camera = Engine::Controller::Get()->GetCamera();
      auto type = camera.GetType();
      f32 childHeight =
          camera.GetType() == Engine::EditorCamera::CameraType::Orbit ? 110.0f
                                                                      : 80.0f;
      ImGui::Text("Movement and Control");
      ImGui::Separator();

      ImGui::PushItemWidth(-1);

      ImGui::Text("Rotation Sensitivity");
      ImGui::SameLine();
      ImGui::TextDisabled(ICON_LC_INFO);
      if (ImGui::IsItemHovered())
        ImGui::SetTooltip(
            "Controls how fast the camera rotates when moving the mouse.");
      ImGui::SliderFloat("##rotSens", &camera.GetRotationSpeed(), 0.001f, 0.1f,
                         "%.3f");

      ImGui::Text("Movement Speed");
      ImGui::SameLine();
      ImGui::TextDisabled(ICON_LC_INFO);
      if (ImGui::IsItemHovered())
        ImGui::SetTooltip(
            "Controls how fast the camera moves through the scene.");
      ImGui::SliderFloat("##moveSpeed", &camera.GetMovementSpeed(), 0.1f, 20.0f,
                         "%.2f");

      ImGui::PopItemWidth();

      ImGui::Spacing();

      ImGui::Text("Projection");
      ImGui::Separator();

      bool projectionChanged = false;

      f32 &fov = camera.GetFOV();
      f32 &nearPlane = camera.GetNearPlane();
      f32 &farPlane = camera.GetFarPlane();
      f32 aspectRatio = camera.GetAspectRatio();

      ImGui::Text("Projection Type");
      ImGui::SameLine();
      ImGui::TextDisabled(ICON_LC_INFO);
      if (ImGui::IsItemHovered())
        ImGui::SetTooltip("Choose between Perspective (3D) or Orthographic "
                          "(2D-like) projection.");

      ImGui::PushItemWidth(-1);

      ImGui::Text("Field of View (FOV)");
      ImGui::SameLine();
      ImGui::TextDisabled(ICON_LC_INFO);
      if (ImGui::IsItemHovered())
        ImGui::SetTooltip(
            "Controls the vertical field of view angle of the camera.");
      if (ImGui::SliderFloat("##fov", &fov, 10.0f, 120.0f, "%.1fdeg"))
        projectionChanged = true;

      ImGui::Text("Near Plane");
      ImGui::SameLine();
      ImGui::TextDisabled(ICON_LC_INFO);
      if (ImGui::IsItemHovered())
        ImGui::SetTooltip(
            "The closest distance from the camera where rendering starts.");
      if (ImGui::SliderFloat("##near", &nearPlane, 0.01f, 10.0f, "%.2f"))
        projectionChanged = true;

      ImGui::Text("Far Plane");
      ImGui::SameLine();
      ImGui::TextDisabled(ICON_LC_INFO);
      if (ImGui::IsItemHovered())
        ImGui::SetTooltip(
            "The farthest distance from the camera where rendering ends.");
      if (ImGui::SliderFloat("##far", &farPlane, nearPlane + 1.0f, 1000.0f,
                             "%.1f"))
        projectionChanged = true;

      ImGui::PopItemWidth();

      if (projectionChanged) {
        camera.SetPerspectiveProjection(fov, aspectRatio, nearPlane, farPlane);
      }

      ImGui::Spacing();
      ImGui::Unindent();
    }

    ImGui::Separator();
    ImGui::Spacing();

    if (ImGui::Button("Apply", ImVec2(100, 0))) {

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
      selectedFontSize = 1;
      selectedTheme = "Aquila Green";
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
