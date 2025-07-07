#include "Elements/ContentBrowserElement.h"
#include "UI.h"

namespace Editor::UIManagement {
    namespace fs = std::filesystem;

    static fs::path s_CurrentPath = ASSET_PATH;

    ContentElement::ContentElement() {
        Engine::EventBus::Get().Dispatch(QueryEvent{
            QueryCommand::ContentBrowserAskTextures,
            {},
            [this](UIEventResult result, CommandParam payload) {
                if (result == UIEventResult::Success) {
                    if (auto textures = std::get_if<std::vector<Ref<Engine::Texture2D>>>(&payload)) {
                        m_Textures = *textures;
                    }
                }
            }
        });

        // ensure the asset path exists
        if (fs::exists(ASSET_PATH)) {
            s_CurrentPath = ASSET_PATH;
        } else {
            fs::create_directory(ASSET_PATH);
            s_CurrentPath = ASSET_PATH;
        }
    }

    void ContentElement::DrawFolderTree(const fs::path& basePath, fs::path& selectedPath) {
        for (auto& entry : fs::directory_iterator(basePath)) {
            if (!entry.is_directory())
                continue;

            const fs::path& path = entry.path();
            const std::string name = path.filename().string();
            bool is_selected = (path == selectedPath);

            ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnDoubleClick | ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_DrawLinesToNodes ;
            if (is_selected)
                flags |= ImGuiTreeNodeFlags_Selected;

            ImGui::PushID(name.c_str());

            std::string label = std::string(ICON_LC_FOLDER) + " " + name;
            bool open = ImGui::TreeNodeEx(label.c_str(), flags);

            if (ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left) && ImGui::IsItemHovered()) {
                std::string pathStr = fs::path(path).string();
                std::replace(pathStr.begin(), pathStr.end(), '\\', '/');

                selectedPath = pathStr;
                s_CurrentPath = selectedPath;
            }

            if (ImGui::IsMouseClicked(ImGuiMouseButton_Right) && ImGui::IsItemHovered()){
                m_CurrentlyHoveredPath = path.string();
                ImGui::OpenPopup("FolderActions");
            }

            FolderActionsPopup();

            if (open) {
                DrawFolderTree(path, selectedPath);
                ImGui::TreePop();
            }

            ImGui::PopID();
        }
    }

    void ContentElement::FolderActionsPopup(){
        if (ImGui::BeginPopup("FolderActions", ImGuiWindowFlags_NoMove)) {

            if (ImGui::MenuItem(ICON_LC_TRASH " Delete")) {
                m_OpenDeletePopup = true; //https://github.com/ocornut/imgui/issues/331 
            }

            if (ImGui::MenuItem(ICON_LC_PEN " Rename")) {
                m_RenameBuffer.resize(128);
                std::fill(m_RenameBuffer.begin(), m_RenameBuffer.end(), 0);

                fs::path currentPath(m_CurrentlyHoveredPath);
                std::string currentName = currentPath.filename().string();

                #ifdef AQUILA_PLATFORM_WINDOWS
                    strncpy_s(m_RenameBuffer.data(), m_RenameBuffer.size(), currentName.c_str(), _TRUNCATE);
                #elif defined(AQUILA_PLATFORM_LINUX)
                    std::strncpy(m_RenameBuffer.data(), currentName.c_str(), m_RenameBuffer.size() - 1);
                    m_RenameBuffer[m_RenameBuffer.size() - 1] = '\0';  // Null-terminate to avoid overflow
                #endif

                m_OpenRenamePopup = true;
            }
            
            ImGui::EndPopup();
        }
    }

    // TODO : make it safe, it works but unsafe
    void ContentElement::RenameFolderPopup() {
        // later lol
        // CreatePopup({500, 150});
        // if (ImGui::BeginPopupModal("Rename", nullptr, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove)) {

        //     ImGui::PushFont(Editor::UIManager::s_Fonts.Font28);
        //     ImGui::Text("Enter new folder name:");
        //     ImGui::PopFont();

        //     ImGui::InputText("##renameFolder", m_RenameBuffer.data(), m_RenameBuffer.size(), ImGuiInputTextFlags_EnterReturnsTrue);

        //     if (ImGui::Button("Rename") || ImGui::IsKeyPressed(ImGuiKey_Enter)) {
        //         if (!m_RenameBuffer.empty()) {
        //             fs::path oldPath(m_CurrentlyHoveredPath);
        //             fs::path newPath = oldPath.parent_path() / m_RenameBuffer;

        //             if (!fs::exists(newPath)) {
        //                 fs::rename(oldPath, newPath);

        //                 std::string pathStr = fs::path(newPath).string();
        //                 std::replace(pathStr.begin(), pathStr.end(), '\\', '/');


        //                 m_CurrentlyHoveredPath = pathStr;
        //                 s_CurrentPath = m_CurrentlyHoveredPath;
        //                 ImGui::CloseCurrentPopup();
        //             } else {
        //                 // handle errors
        //             }
        //         }
        //     }

        //     ImGui::SameLine();

        //     if (ImGui::Button("Cancel")) {
        //         ImGui::CloseCurrentPopup();
        //     }

        //     ImGui::EndPopup();
        // }
    }


    void ContentElement::DeleteItemPopup(){
        CreatePopup({500, 130});
        if (ImGui::BeginPopupModal("Delete", nullptr, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove )) {

            ImGui::PushFont(Editor::UIManager::s_Fonts.Font28);
            ImGui::Text("Are you sure you want to delete this?");
            ImGui::PopFont();

            ImGui::PushFont(Editor::UIManager::s_Fonts.Font16);
            ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(255, 100, 100, 255));
            std::string displayPath = m_CurrentlyHoveredPath;
            std::replace(displayPath.begin(), displayPath.end(), '\\', '/');
            ImGui::TextWrapped("Item to be deleted: %s", displayPath.c_str());
            ImGui::PopStyleColor();
            ImGui::PopFont();

            if (ImGui::Button(ICON_LC_CHECK " Yes", {90, 30})){
                fs::remove_all(m_CurrentlyHoveredPath);
                ImGui::CloseCurrentPopup();
            }

            ImGui::SameLine();

            if (ImGui::Button(ICON_LC_X " No", {90, 30})){
                m_OpenDeletePopup = false;
                ImGui::CloseCurrentPopup();
            }

            ImGui::EndPopup();
        }
    }


    void ContentElement::CreateFolderPopup(){
        CreatePopup({500, 170});

        if (ImGui::BeginPopupModal("Create new folder", nullptr, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove )) {
            static char folderName[128] = "";

            ImGui::PushFont(Editor::UIManager::s_Fonts.Font32);
            ImGui::Text("Enter new folder name:");
            ImGui::PopFont();

            ImGui::InputText("##foldername", folderName, IM_ARRAYSIZE(folderName));
            fs::path newFolder = s_CurrentPath / folderName;
            std::string pathStr = fs::path(newFolder).string();
            std::replace(pathStr.begin(), pathStr.end(), '\\', '/');

            ImGui::TextWrapped("The folder will be created in : \n %s", pathStr.c_str());

            if (ImGui::Button("Create")) {
                if (!fs::exists(newFolder)) {
                    fs::create_directory(newFolder);
                }
                ImGui::CloseCurrentPopup();
                folderName[0] = '\0';
            }

            ImGui::SameLine();

            if (ImGui::Button("Cancel")) {
                ImGui::CloseCurrentPopup();
                folderName[0] = '\0';
            }

            ImGui::EndPopup();
        }
    }

    void ContentElement::ContentActionsPopup()
    {
        if (ImGui::IsWindowHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Right))
            ImGui::OpenPopup("CreateNewItemPopup");

        if (ImGui::BeginPopup("CreateNewItemPopup"))
        {
            if (ImGui::MenuItem(ICON_LC_PALETTE " Create Material"))
            {
                //todo
                ImGui::CloseCurrentPopup();
            }
            if (ImGui::MenuItem(ICON_LC_FILE_CODE " Create Script"))
            {
                // todo
                ImGui::CloseCurrentPopup();
            }
            if (ImGui::MenuItem(ICON_LC_SHAPES " Create Shader"))
            {
                //todo
                ImGui::CloseCurrentPopup();
            }

            ImGui::EndPopup();
        }
    }


    void ContentElement::Draw(void* data) {
        ImGui::Begin(ICON_LC_PACKAGE_OPEN " Content Browser", nullptr, ImGuiWindowFlags_NoCollapse);

        ImVec2 avail = ImGui::GetContentRegionAvail();

        ImGui::BeginChild("Navigation", ImVec2(avail.x, 40), true, ImGuiWindowFlags_NoScrollbar);

        if (s_CurrentPath != ASSET_PATH) {
            if (ImGui::Button(ICON_LC_SQUARE_ARROW_LEFT)) {
                s_CurrentPath = s_CurrentPath.parent_path();
                if (!fs::exists(s_CurrentPath) || s_CurrentPath < ASSET_PATH) {
                    s_CurrentPath = ASSET_PATH;
                }
            }
        } else {
            ImGui::BeginDisabled();
            ImGui::Button(ICON_LC_SQUARE_ARROW_LEFT);
            ImGui::EndDisabled();
        }

        ImGui::SameLine();

        ImGui::BeginDisabled();
        ImGui::Button(ICON_LC_SQUARE_ARROW_RIGHT);
        ImGui::EndDisabled();

        ImGui::SameLine();

        if (ImGui::Button(ICON_LC_FOLDER_PLUS)) {
            ImGui::OpenPopup("Create new folder");
        }

        CreateFolderPopup();

        ImGui::SameLine();

            ImGui::BeginChild("PathDisplay", ImVec2(avail.x - 50, 0), false);
                ImGui::TextWrapped("%s", s_CurrentPath.string().c_str());
            ImGui::EndChild();

        ImGui::EndChild();

        if (m_OpenDeletePopup == true) {
            ImGui::OpenPopup("Delete");
            m_OpenDeletePopup = false;
        }

        if (m_OpenRenamePopup == true) {
            ImGui::OpenPopup("Rename");
            m_OpenRenamePopup = false;
        }

        DeleteItemPopup();
        RenameFolderPopup();

        float navigation_height = 45.0f;
        float folders_width = avail.x * 0.10f;
        float content_width = avail.x - folders_width - 10.0f;

        ImGui::BeginChild("Folders", ImVec2(folders_width, avail.y - navigation_height), true);
            DrawFolderTree(ASSET_PATH, s_CurrentPath);
        ImGui::EndChild();

        ImGui::SameLine();

        ImGui::BeginChild("Content", ImVec2(content_width, avail.y - navigation_height), true);

        if (!s_CurrentPath.empty() && fs::exists(s_CurrentPath) && fs::is_directory(s_CurrentPath)) {
            bool isEmpty = fs::directory_iterator(s_CurrentPath) == fs::directory_iterator();

            if (isEmpty) {
                ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "Empty folder");
            } else {
                constexpr float cardWidth = 100.0f;
                constexpr float cardHeight = 160.0f;
                constexpr float padding = 10.0f;
                int id = 0;

                int cardsPerRow = std::max(1, static_cast<int>(content_width / (cardWidth + padding)));
                int cardIndex = 0;

                for (auto& entry : fs::directory_iterator(s_CurrentPath)) {
                    std::string name = entry.path().filename().string();

                    if (cardIndex % cardsPerRow != 0) ImGui::SameLine();

                    ImGui::BeginGroup();
                    ImGui::PushID(id++);

                    ImGui::BeginChild("Card", ImVec2(cardWidth, cardHeight), true, ImGuiWindowFlags_NoScrollbar);

                    ImVec2 cardPos = ImGui::GetCursorScreenPos();

                    constexpr float iconPadding = 5.0f;
                    ImGui::SetCursorPos(ImVec2(iconPadding, iconPadding));

                    ImTextureID textureID;

                    if (entry.is_directory()) {
                        textureID = reinterpret_cast<ImTextureID>(m_Textures[0]->GetDescriptorSet());
                    } else if (entry.path().extension() == ".aqscene") {
                        textureID = reinterpret_cast<ImTextureID>(m_Textures[1]->GetDescriptorSet());
                    } else {
                        textureID = reinterpret_cast<ImTextureID>(m_Textures[2]->GetDescriptorSet());
                    }

                    if (textureID) {
                        if (ImGui::ImageButton("##Icon", textureID,
                                            ImVec2(cardWidth - 2 * iconPadding, cardWidth - 2 * iconPadding))) {
                            if (entry.is_directory()) {
                                s_CurrentPath = entry.path().string();
                            }
                        }
                        
                        if (ImGui::IsItemHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Right)) {
                            m_CurrentlyHoveredPath = entry.path().string();
                            ImGui::OpenPopup("FolderActions");
                        }

                        FolderActionsPopup();
                    }

                    ImGui::Separator();

                    ImGui::TextWrapped("%s", name.c_str());

                    std::string tag = "FILE";
                    if (entry.is_directory())
                        tag = "FOLDER";
                    else if (entry.path().extension() == ".aqscene")
                        tag = "SCENE";

                    ImVec2 tagSize = ImGui::CalcTextSize(tag.c_str());
                    float tagX = cardPos.x + cardWidth - tagSize.x - 10;
                    float tagY = cardPos.y + cardHeight - tagSize.y - 14;

                    ImGui::SetCursorScreenPos(ImVec2(tagX, tagY));

                    ImGui::PushFont(UIManager::s_Fonts.Font12);
                    ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 0.8f), "%s", tag.c_str());
                    ImGui::PopFont();

                    ImGui::EndChild();
                    ImGui::PopID();
                    ImGui::EndGroup();

                    cardIndex++;
                }
            }
        } else {
            ImGui::TextColored(ImVec4(1, 0, 0, 1), "Invalid directory");
        }

        ContentActionsPopup();

        ImGui::EndChild();


        ImGui::End();
    }


}