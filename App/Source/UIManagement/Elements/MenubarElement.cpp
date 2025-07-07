#include "Elements/MenubarElement.h"
#include "UI.h"

namespace Editor::UIManagement {
    void MenubarElement::Draw(void* data) {
        if (ImGui::BeginMainMenuBar()) {

            if (ImGui::BeginMenu("File")) {
                if (ImGui::MenuItem(ICON_LC_FILE " New Scene", "Ctrl+N")) { 
                    Engine::EventBus::Get().Dispatch(UICommandEvent{
                        UICommand::NewScene
                    });
                }
                if (ImGui::MenuItem(ICON_LC_FOLDER_OPEN " Open scene...", "Ctrl+O")) { 
                    nfdchar_t* outPath = nullptr;
                    nfdresult_t result = NFD_OpenDialog("json", nullptr, &outPath);
                    if (result == NFD_OKAY) {
                        std::string path = outPath;
                        std::cout << "Selected scene file: " << path << std::endl; 
                        Engine::EventBus::Get().Dispatch(UICommandEvent{
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
                    Engine::EventBus::Get().Dispatch(UICommandEvent{
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
                ImFont* newFont = Editor::UIManager::s_Fonts.Font16;

                if (ImGui::Combo("Font Size", &selectedFontIndex, fontSizes, IM_ARRAYSIZE(fontSizes))) {
                    switch (selectedFontIndex) {
                        case 0: newFont = Editor::UIManager::s_Fonts.Font12; break;
                        case 1: newFont = Editor::UIManager::s_Fonts.Font16; break;
                        case 2: newFont = Editor::UIManager::s_Fonts.Font18; break;
                        case 3: newFont = Editor::UIManager::s_Fonts.Font20; break;
                        case 4: newFont = Editor::UIManager::s_Fonts.Font24; break;
                        case 5: newFont = Editor::UIManager::s_Fonts.Font28; break;
                        case 6: newFont = Editor::UIManager::s_Fonts.Font32; break;
                        case 7: newFont = Editor::UIManager::s_Fonts.Font40; break;
                        case 8: newFont = Editor::UIManager::s_Fonts.Font48; break;
                        case 9: newFont = Editor::UIManager::s_Fonts.Font64; break;
                        case 10: newFont = Editor::UIManager::s_Fonts.Font72; break;
                    }

                    Editor::UIManager::s_CurrentFont = newFont;
                }

                ImGui::End();
            }
        }

        if (m_AboutOpened) {
            CreatePopup({500, 400});

            ImGui::Begin(ICON_LC_INFO " About Aquila", &m_AboutOpened, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoDocking);


            ImGui::PushFont(Editor::UIManager::s_Fonts.Font48);
            ImGui::Text("Aquila Engine");
            ImGui::PopFont();

            ImGui::Separator();

            ImGui::PushFont(Editor::UIManager::s_Fonts.Font32);
            ImGui::Text("Version 0.1.0 (Dev)");
            ImGui::PopFont();

            ImGui::Spacing();

            ImGui::PushFont(Editor::UIManager::s_Fonts.Font24);
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

            ImGui::PushFont(Editor::UIManager::s_Fonts.Font28);
            if (ImGui::Button(ICON_LC_CHECK " OK", {100, 40})) {
                m_AboutOpened = false;
                ImGui::CloseCurrentPopup();
            }

            ImGui::PopFont();
            ImGui::End();
        }

    }
}
