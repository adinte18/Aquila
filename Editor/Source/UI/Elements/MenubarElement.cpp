#include "UI/Elements//MenubarElement.h"
#include "UI/UI.h"

namespace Editor::Elements {
    void MenubarElement::Draw() {
        auto& fontManager = UI::FontManager::Get();


        if (ImGui::BeginMainMenuBar()) {

            if (ImGui::BeginMenu("File")) {
                if (ImGui::MenuItem(ICON_LC_FILE " New Scene", "Ctrl+N")) { 
                    Engine::EventBus::Get()->Dispatch(UICommandEvent{
                        UICommand::NewScene
                    });
                }
                if (ImGui::MenuItem(ICON_LC_FOLDER_OPEN " Open scene...", "Ctrl+O")) { 
                    nfdchar_t* outPath = nullptr;
                    nfdresult_t result = NFD_OpenDialog("aqscene", nullptr, &outPath);
                    if (result == NFD_OKAY) {
                        std::string path = outPath;
                        std::cout << "Selected scene file: " << path << std::endl; 
                        Engine::EventBus::Get()->Dispatch(UICommandEvent{
                            UICommand::OpenScene,
                            {{"path", path}}
                        });
                        NFDi_Free(outPath);
                    } else if (result == NFD_CANCEL) {
                        std::cout << "User cancelled scene open dialog." << std::endl;
                    } else {
                        std::cerr << "Error opening scene file: " << NFD_GetError() << std::endl;
                    }
                }
                if (ImGui::MenuItem(ICON_LC_SAVE_ALL " Save scene...", "Ctrl+S")) { 
                    Engine::EventBus::Get()->Dispatch(UICommandEvent{
                        UICommand::SaveScene
                    });
                }
                if (ImGui::MenuItem(ICON_LC_SAVE_ALL " Save scene as...", "Ctrl+S")) { 

                }
                ImGui::Separator();
                if (ImGui::MenuItem(ICON_LC_CIRCLE_X " Exit", "Ctrl+X")) { }
                ImGui::EndMenu();
            }

            if (ImGui::BeginMenu("Edit")){
                if (ImGui::MenuItem(ICON_LC_COG " Editor preferences", nullptr, &m_PreferencesOpened)){

                }

                ImGui::EndMenu();
            }

            if (ImGui::BeginMenu("View")) {
                ImGui::EndMenu();
            }

            if (ImGui::BeginMenu("Help")) {
                if (ImGui::MenuItem(ICON_LC_CIRCLE_HELP " About", nullptr, &m_AboutOpened)) {
                    
                }
                ImGui::EndMenu();
            }

            ImGui::EndMainMenuBar();
        }

        if (m_PreferencesOpened){
            CreatePopup({500, 300});

            if (ImGui::Begin(ICON_LC_COG " Editor preferences", &m_PreferencesOpened, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoDocking)) {
                
                const char* fontSizes[] = {"12", "16", "18", "20", "24", "28", "32", "40", "48", "64", "72"};
                static int selectedFontIndex = 0;
                ImFont* newFont = fontManager.GetFonts().Font16;

                if (ImGui::Combo("Font Size", &selectedFontIndex, fontSizes, IM_ARRAYSIZE(fontSizes))) {
                    switch (selectedFontIndex) {
                        case 0: newFont = fontManager.GetFonts().Font12; break;
                        case 1: newFont = fontManager.GetFonts().Font16; break;
                        case 2: newFont = fontManager.GetFonts().Font18; break;
                        case 3: newFont = fontManager.GetFonts().Font20; break;
                        case 4: newFont = fontManager.GetFonts().Font24; break;
                        case 5: newFont = fontManager.GetFonts().Font28; break;
                        case 6: newFont = fontManager.GetFonts().Font32; break;
                        case 7: newFont = fontManager.GetFonts().Font40; break;
                        case 8: newFont = fontManager.GetFonts().Font48; break;
                        case 9: newFont = fontManager.GetFonts().Font64; break;
                        case 10: newFont = fontManager.GetFonts().Font72; break;
                    }

                    fontManager.SetCurrentFont(newFont);
                }

                ImGui::End();
            }
        }

        if (m_AboutOpened) {
            CreatePopup({500, 400});

            ImGui::Begin(ICON_LC_INFO " About Aquila", &m_AboutOpened, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoDocking);


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
            ImGui::BulletText("Platform: Windows");
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

    }
}
