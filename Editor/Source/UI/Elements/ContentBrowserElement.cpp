#include "UI/Elements/ContentBrowserElement.h"
#include "Platform/DebugLog.h"
#include "UI/FontManager.h"
#include "UI/UI.h"
#include "Platform/Filesystem/VirtualFileSystem.h"
#include "imgui.h"

namespace Editor::Elements {

    static std::string s_CurrentPath = "/Assets";

    ContentElement::ContentElement() {
        
        Engine::EventBus::Get()->Dispatch(QueryEvent{
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

        if (!VFS::VirtualFileSystem::Get()->IsMounted("/Assets")) {
            Debug::LogError("ContentBrowser: /Assets mount point not found!");
        }
    }

    void ContentElement::DrawFolderTree(const std::string& basePath, std::string& selectedPath) {
        if (!VFS::VirtualFileSystem::Get()->IsDirectory(basePath)) {
            return;
        }

        auto entries = VFS::VirtualFileSystem::Get()->ListDirectory(basePath);

        for (const auto& entryName : entries) {
            std::string fullPath = basePath;
            if (fullPath.back() != '/') fullPath += "/";
            fullPath += entryName;
            
            if (!VFS::VirtualFileSystem::Get()->IsDirectory(fullPath)) {
                continue;
            }

            bool is_selected = (fullPath == selectedPath);

            ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnDoubleClick | 
                                     ImGuiTreeNodeFlags_OpenOnArrow | 
                                     ImGuiTreeNodeFlags_SpanAvailWidth | 
                                     ImGuiTreeNodeFlags_DrawLinesToNodes;
            if (is_selected)
                flags |= ImGuiTreeNodeFlags_Selected;

            ImGui::PushID(entryName.c_str());

            std::string label = std::string(ICON_LC_FOLDER) + " " + entryName;
            bool open = ImGui::TreeNodeEx(label.c_str(), flags);

            if (ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left) && ImGui::IsItemHovered()) {
                selectedPath = fullPath;
                s_CurrentPath = selectedPath;
            }

            if (ImGui::IsMouseClicked(ImGuiMouseButton_Right) && ImGui::IsItemHovered()){
                m_CurrentlyHoveredPath = fullPath;
                ImGui::OpenPopup("FolderActions");
            }

            FolderActionsPopup();

            if (open) {
                DrawFolderTree(fullPath, selectedPath);
                ImGui::TreePop();
            }

            ImGui::PopID();
        }
    }

    void ContentElement::FolderActionsPopup(){
        if (ImGui::BeginPopup("FolderActions", ImGuiWindowFlags_NoMove)) {

            if (ImGui::MenuItem(ICON_LC_TRASH " Delete")) {
                m_OpenDeletePopup = true;
            }

            if (ImGui::MenuItem(ICON_LC_PEN " Rename")) {
                m_RenameBuffer.resize(128);
                std::fill(m_RenameBuffer.begin(), m_RenameBuffer.end(), 0);

                size_t lastSlash = m_CurrentlyHoveredPath.find_last_of('/');
                std::string currentName = (lastSlash != std::string::npos) 
                    ? m_CurrentlyHoveredPath.substr(lastSlash + 1) 
                    : m_CurrentlyHoveredPath;

                #ifdef AQUILA_PLATFORM_WINDOWS
                    strncpy_s(m_RenameBuffer.data(), m_RenameBuffer.size(), currentName.c_str(), _TRUNCATE);
                #elif defined(AQUILA_PLATFORM_LINUX)
                    std::strncpy(m_RenameBuffer.data(), currentName.c_str(), m_RenameBuffer.size() - 1);
                    m_RenameBuffer[m_RenameBuffer.size() - 1] = '\0';
                #endif

                m_OpenRenamePopup = true;
            }
            
            ImGui::EndPopup();
        }
    }

    void ContentElement::RenameFolderPopup() {
        // Note: VFS rename operations would need to be supported by the underlying filesystem
        // For now, this is a placeholder showing how it would work
        /*
        if (ImGui::BeginPopupModal("Rename", nullptr, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove)) {
            ImGui::PushFont(Editor::UI::fonts.Font28);
            ImGui::Text("Enter new folder name:");
            ImGui::PopFont();

            ImGui::InputText("##renameFolder", m_RenameBuffer.data(), m_RenameBuffer.size(), ImGuiInputTextFlags_EnterReturnsTrue);

            if (ImGui::Button("Rename") || ImGui::IsKeyPressed(ImGuiKey_Enter)) {
                if (!m_RenameBuffer.empty()) {
                    // Extract parent path
                    size_t lastSlash = m_CurrentlyHoveredPath.find_last_of('/');
                    std::string parentPath = (lastSlash != std::string::npos) 
                        ? m_CurrentlyHoveredPath.substr(0, lastSlash + 1) 
                        : "/";
                    
                    std::string newPath = parentPath + std::string(m_RenameBuffer.data());

                    if (!m_VFS->Exists(newPath)) {
                        // Note: You'd need to implement rename in your VFS
                        // This might involve renaming on the underlying filesystem
                        // if it supports write operations
                        
                        m_CurrentlyHoveredPath = newPath;
                        s_CurrentPath = m_CurrentlyHoveredPath;
                        ImGui::CloseCurrentPopup();
                    } else {
                        // Handle error - file exists
                        Debug::LogWarning("Rename failed: Path already exists");
                    }
                }
            }

            ImGui::SameLine();
            if (ImGui::Button("Cancel")) {
                ImGui::CloseCurrentPopup();
            }

            ImGui::EndPopup();
        }
        */
    }

    void ContentElement::DeleteItemPopup(){
        CreatePopup({500, 130});
        if (ImGui::BeginPopupModal("Delete", nullptr, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove)) {

            ImGui::PushFont(UI::FontManager::Get().GetFonts().Font28);
            ImGui::Text("Are you sure you want to delete this?");
            ImGui::PopFont();

            ImGui::PushFont(UI::FontManager::Get().GetFonts().Font16);
            ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(255, 100, 100, 255));
            ImGui::TextWrapped("Item to be deleted: %s", m_CurrentlyHoveredPath.c_str());
            ImGui::PopStyleColor();
            ImGui::PopFont();

            if (ImGui::Button(ICON_LC_CHECK " Yes", {90, 30})){
                bool success = false;
                if (VFS::VirtualFileSystem::Get()->IsDirectory(m_CurrentlyHoveredPath)) {
                    success = VFS::VirtualFileSystem::Get()->DeleteDirectory(m_CurrentlyHoveredPath);
                } else {
                    success = VFS::VirtualFileSystem::Get()->DeleteFile_aq(m_CurrentlyHoveredPath);
                }
                
                if (!success) {
                    Debug::LogError("Failed to delete: " + m_CurrentlyHoveredPath);
                }
                
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

        if (ImGui::BeginPopupModal("Create new folder", nullptr, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove)) {
            static char folderName[128] = "";

            ImGui::PushFont(UI::FontManager::Get().GetFonts().Font32);
            ImGui::Text("Enter new folder name:");
            ImGui::PopFont();

            ImGui::InputText("##foldername", folderName, IM_ARRAYSIZE(folderName));
            
            std::string newFolderPath = s_CurrentPath;
            if (newFolderPath.back() != '/') newFolderPath += "/";
            newFolderPath += folderName;

            ImGui::TextWrapped("The folder will be created at: \n%s", newFolderPath.c_str());

            if (ImGui::Button("Create")) {
                if (!VFS::VirtualFileSystem::Get()->Exists(newFolderPath)) {
                    bool success = VFS::VirtualFileSystem::Get()->CreateDir(newFolderPath);
                    if (!success) {
                        Debug::LogError("Failed to create directory: " + newFolderPath);
                    }
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

    void ContentElement::ContentActionsPopup() {
        if (ImGui::IsWindowHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Right))
            ImGui::OpenPopup("CreateNewItemPopup");

        if (ImGui::BeginPopup("CreateNewItemPopup")) {
            if (ImGui::MenuItem(ICON_LC_PALETTE " Create Material")) {
                std::string materialPath = s_CurrentPath + "/NewMaterial.mat";
                ImGui::CloseCurrentPopup();
            }
            
            if (ImGui::MenuItem(ICON_LC_FILE_CODE " Create Script")) {
                std::string scriptPath = s_CurrentPath + "/NewScript.cs";
                ImGui::CloseCurrentPopup();
            }
            
            if (ImGui::MenuItem(ICON_LC_SHAPES " Create Shader")) {
                std::string shaderPath = s_CurrentPath + "/NewShader.glsl";
                ImGui::CloseCurrentPopup();
            }

            ImGui::EndPopup();
        }
    }

    std::string ContentElement::GetFileExtension(const std::string& filename) {
        size_t dotPos = filename.find_last_of('.');
        if (dotPos != std::string::npos && dotPos != filename.length() - 1) {
            return filename.substr(dotPos);
        }
        return "";
    }

    std::string ContentElement::GetParentPath(const std::string& path) {
        size_t lastSlash = path.find_last_of('/');
        if (lastSlash != std::string::npos && lastSlash > 0) {
            return path.substr(0, lastSlash);
        }
        return "/Assets";
    }

    void ContentElement::Draw() {
        ImGui::Begin(ICON_LC_PACKAGE_OPEN " Content Browser", nullptr, ImGuiWindowFlags_NoCollapse);

        ImVec2 avail = ImGui::GetContentRegionAvail();

        ImGui::BeginChild("Navigation", ImVec2(avail.x, 40), true, ImGuiWindowFlags_NoScrollbar);

        if (s_CurrentPath != "/Assets") {
            if (ImGui::Button(ICON_LC_SQUARE_ARROW_LEFT)) {
                s_CurrentPath = GetParentPath(s_CurrentPath);
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
            ImGui::TextWrapped("%s", s_CurrentPath.c_str());
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
            DrawFolderTree("/Assets", s_CurrentPath);
        ImGui::EndChild();

        ImGui::SameLine();

        ImGui::BeginChild("Content", ImVec2(content_width, avail.y - navigation_height), true);

        if (!s_CurrentPath.empty() && VFS::VirtualFileSystem::Get()->Exists(s_CurrentPath) && VFS::VirtualFileSystem::Get()->IsDirectory(s_CurrentPath)) {
            auto entries = VFS::VirtualFileSystem::Get()->ListDirectory(s_CurrentPath);

            if (entries.empty()) {
                ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "Empty folder");
            } else {
                constexpr float cardWidth = 100.0f;
                constexpr float cardHeight = 160.0f;
                constexpr float padding = 10.0f;
                int id = 0;

                int cardsPerRow = std::max(1, static_cast<int>(content_width / (cardWidth + padding)));
                int cardIndex = 0;

                for (const auto& entryName : entries) {
                    std::string fullPath = s_CurrentPath;
                    if (fullPath.back() != '/') fullPath += "/";
                    fullPath += entryName;

                    if (cardIndex % cardsPerRow != 0) ImGui::SameLine();

                    ImGui::BeginGroup();
                    ImGui::PushID(id++);

                    ImGui::BeginChild("Card", ImVec2(cardWidth, cardHeight), true, 
                                    ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoMove | 
                                    ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoScrollWithMouse);

                    ImVec2 cardPos = ImGui::GetCursorScreenPos();

                    constexpr float iconPadding = 5.0f;
                    ImGui::SetCursorPos(ImVec2(iconPadding, iconPadding));

                    ImTextureID textureID;

                    if (m_Textures.empty()) {
                        Debug::LogError("ContentBrowserElement: No textures available for icons");
                        return;
                    }

                    bool isDirectory = VFS::VirtualFileSystem::Get()->IsDirectory(fullPath);
                    std::string extension = GetFileExtension(entryName);

                    if (isDirectory) {
                        textureID = reinterpret_cast<ImTextureID>(m_Textures[0]->GetDescriptorSet());
                    } else if (extension == ".aqscene") {
                        textureID = reinterpret_cast<ImTextureID>(m_Textures[1]->GetDescriptorSet());
                    } else {
                        textureID = reinterpret_cast<ImTextureID>(m_Textures[2]->GetDescriptorSet());
                    }

                    if (textureID) {
                        bool clicked = ImGui::ImageButton("##Icon", textureID,
                            ImVec2(cardWidth - 3.5f * iconPadding, cardWidth - 2 * iconPadding));

                        if (clicked) {
                            if (isDirectory) {
                                s_CurrentPath = fullPath;
                            }
                        }

                        if (extension == ".aqscene") {
                            if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID)) {
                                ImGui::SetDragDropPayload("SCENE_PATH", fullPath.c_str(), fullPath.size() + 1);
                                ImGui::Text("Drag scene: %s", entryName.c_str());
                                ImGui::EndDragDropSource();
                            }

                            if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) {
                                Engine::EventBus::Get()->Dispatch(UICommandEvent{
                                    UICommand::OpenScene,
                                    {{"path", fullPath}},
                                    nullptr
                                });
                            }
                        }

                        if (extension == ".gltf" || extension == ".glb" || extension == ".obj" || extension == ".fbx") {
                            if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID)) {
                                ImGui::SetDragDropPayload("MESH_ASSET", fullPath.c_str(), fullPath.size() + 1);
                                ImGui::Text("Drag model: %s", entryName.c_str());
                                ImGui::EndDragDropSource();
                            }
                        }
                        
                        if (ImGui::IsItemHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Right)) {
                            m_CurrentlyHoveredPath = fullPath;
                            ImGui::OpenPopup("FolderActions");
                        }

                        FolderActionsPopup();
                    }

                    ImGui::Separator();

                    std::string displayName = entryName;
                    if (!isDirectory) {
                        size_t dotPos = displayName.find_last_of('.');
                        if (dotPos != std::string::npos) {
                            displayName = displayName.substr(0, dotPos);
                        }
                    }

                    ImGui::PushFont(UI::FontManager::Get().GetFonts().Font14);
                    ImGui::TextWrapped("%s", displayName.c_str());
                    ImGui::PopFont();

                    std::string tag = "FILE";
                    if (isDirectory)
                        tag = "FOLDER";
                    else if (extension == ".aqscene")
                        tag = "SCENE";
                    else if (extension == ".gltf" || extension == ".glb" || extension == ".obj" || extension == ".fbx")
                        tag = "MODEL";

                    ImVec2 tagSize = ImGui::CalcTextSize(tag.c_str());
                    float tagX = cardPos.x + cardWidth - tagSize.x - 10;
                    float tagY = cardPos.y + cardHeight - tagSize.y - 14;

                    ImGui::SetCursorScreenPos(ImVec2(tagX, tagY));

                    ImGui::PushFont(UI::FontManager::Get().GetFonts().Font12);
                    ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 0.8f), "%s", tag.c_str());
                    ImGui::PopFont();

                    ImGui::EndChild();
                    ImGui::PopID();
                    ImGui::EndGroup();

                    cardIndex++;
                }
            }
        } else {
            ImGui::TextColored(ImVec4(1, 0, 0, 1), "Invalid directory or VFS mount point not found");
        }

        ContentActionsPopup();

        ImGui::EndChild();

        ImGui::End();
    }
}