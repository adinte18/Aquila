#include "Scene/Components/TransformComponent.h"
#include "UI/UI.h"

namespace Editor::Elements {
    void PropertiesElement::Draw() {
        auto* scene = Engine::Controller::Get().GetSceneManager().GetActiveScene();
        if (!scene) return;

        auto& registry = scene->GetRegistry();
        entt::entity selected = UIManager::Get().GetSelectedEntity();

        ImGui::Begin("Properties");

        if (selected == entt::null || !registry.valid(selected)) {
            ImGui::Text("No entity selected.");
            ImGui::End();
            return;
        }

        if (registry.any_of<MetadataComponent>(selected))
            DrawComponent_Metadata(registry, selected);

        if (registry.any_of<TransformComponent>(selected))
            DrawComponent_Transform(registry, selected);

        if (registry.any_of<MeshComponent>(selected))
            DrawComponent_Mesh(registry, selected);

        if (registry.any_of<CameraComponent>(selected))
            DrawComponent_Camera(registry, selected);


        ImGui::Separator();
        ImGui::Spacing();

        if (ImGui::Button(ICON_LC_PLUS " Add Component", ImVec2(-1, 0))) {
            ImGui::OpenPopup("AddComponentPopup");
        }

        if (ImGui::BeginPopup("AddComponentPopup")) {
            if (!registry.any_of<MetadataComponent>(selected)) {
                if (ImGui::MenuItem(ICON_LC_FILE_CODE_2 " Metadata")) {
                    registry.emplace<MetadataComponent>(selected);
                    ImGui::CloseCurrentPopup();
                }
            }


            if (!registry.any_of<TransformComponent>(selected)) {
                if (ImGui::MenuItem(ICON_LC_MOVE_3D " Transform")) {
                    registry.emplace<TransformComponent>(selected);
                    ImGui::CloseCurrentPopup();
                }
            }

            if (!registry.any_of<MeshComponent>(selected)) {
                if (ImGui::MenuItem(ICON_LC_GRID_3X3 " Mesh")) {
                    registry.emplace<MeshComponent>(selected);
                    if (!registry.any_of<TransformComponent>(selected)) {
                        registry.emplace<TransformComponent>(selected);
                    }
                    ImGui::CloseCurrentPopup();
                }
            }

            if (!registry.any_of<CameraComponent>(selected)) {
                if (ImGui::MenuItem(ICON_LC_CAMERA " Camera")) {
                    registry.emplace<CameraComponent>(selected);
                    ImGui::CloseCurrentPopup();
                }
            }


            ImGui::EndPopup();
        }

        ImGui::End();
    }

    void UpdateChildrenTransforms(entt::registry& registry, entt::entity parentEntity) {
        if (!registry.valid(parentEntity) || !registry.all_of<SceneNodeComponent, TransformComponent>(parentEntity))
            return;

        auto& parentNode = registry.get<SceneNodeComponent>(parentEntity);
        auto& parentTransform = registry.get<TransformComponent>(parentEntity);
        const glm::mat4& parentWorldMatrix = parentTransform.GetWorldMatrix();

        for (entt::entity child : parentNode.Children) {
            if (!registry.valid(child) || !registry.all_of<TransformComponent>(child))
                continue;

            auto& childTransform = registry.get<TransformComponent>(child);
            childTransform.UpdateWorldMatrix(parentWorldMatrix);

            // Recursively update child's children
            UpdateChildrenTransforms(registry, child);
        }
    }


    void PropertiesElement::DrawComponent_Transform(entt::registry& registry, entt::entity entity) {
        auto& transform = registry.get<TransformComponent>(entity);
        auto& position = transform.GetLocalPosition();
        auto& rotation = transform.GetLocalRotation();
        auto& scale = transform.GetLocalScale();
        

        if (ImGui::CollapsingHeader(ICON_LC_MOVE_3D " TRANSFORM", ImGuiTreeNodeFlags_DefaultOpen)) {
            bool changed = false;

            auto DrawVector3Control = [](const char* label, glm::vec3& values, float resetValue = 0.0f) -> bool {
                bool modified = false;
                ImGui::PushID(label);
                ImGui::Columns(2);
                ImGui::SetColumnWidth(0, 100);
                ImGui::Text("%s", label);
                ImGui::NextColumn();

                const char* axisLabels[] = { "X", "Y", "Z" };
                ImVec4 axisColors[] = {
                    { 0.8f, 0.1f, 0.15f, 1.0f },
                    { 0.2f, 0.7f, 0.2f, 1.0f },
                    { 0.2f, 0.3f, 0.9f, 1.0f }
                };

                constexpr float buttonSize = 20.0f;
                constexpr float inputWidth = 60.0f;

                for (int i = 0; i < 3; i++) {
                    ImGui::PushStyleColor(ImGuiCol_Button, axisColors[i]);
                    if (ImGui::Button(axisLabels[i], ImVec2(buttonSize, buttonSize))) {
                        values[i] = resetValue;
                        modified = true;
                    }
                    ImGui::PopStyleColor();

                    ImGui::SameLine(0.0f, 0.0f);
                    ImGui::SetNextItemWidth(inputWidth);
                    modified |= ImGui::DragFloat(("##" + std::string(axisLabels[i]) + label).c_str(), &values[i], 0.1f, -FLT_MAX, FLT_MAX, "%.2f");

                    if (i < 2) ImGui::SameLine();
                }

                ImGui::Columns(1);
                ImGui::PopID();
                return modified;
            };

            changed |= DrawVector3Control("Position", position);

            glm::vec3 eulerRotation = glm::degrees(glm::eulerAngles(rotation));
            if (DrawVector3Control("Rotation", eulerRotation)) {
                rotation = glm::quat(glm::radians(eulerRotation));
                changed = true;
            }

            changed |= DrawVector3Control("Scale", scale, 1.0f);

            if (changed) {
                glm::mat4 parentWorldMatrix = glm::mat4(1.0f);
                if (registry.all_of<SceneNodeComponent>(entity)) {
                    entt::entity parent = registry.get<SceneNodeComponent>(entity).Parent;
                    if (parent != entt::null && registry.valid(parent) && registry.all_of<TransformComponent>(parent))
                        parentWorldMatrix = registry.get<TransformComponent>(parent).GetWorldMatrix();
                }

                transform.UpdateWorldMatrix(parentWorldMatrix);
                UpdateChildrenTransforms(registry, entity);
            }
        }
    }


    void PropertiesElement::DrawComponent_Metadata(entt::registry& registry, entt::entity entity) {
        auto& meta = registry.get<MetadataComponent>(entity);
        char buffer[256];

        #ifdef AQUILA_PLATFORM_WINDOWS
            strncpy_s(buffer, meta.Name.c_str(), sizeof(buffer));
        #elif defined(AQUILA_PLATFORM_LINUX)
            strncpy(buffer, meta.Name.c_str(), sizeof(buffer));
            buffer[sizeof(buffer) - 1] = '\0'; // ensure null-termination
        #endif
            
        if (ImGui::CollapsingHeader(ICON_LC_FILE_CODE_2 " METADATA", ImGuiTreeNodeFlags_DefaultOpen)) {
            ImGui::Text("Name");
            ImGui::SameLine();
            ImGui::InputText("##Name", buffer, sizeof(buffer));
            if (ImGui::IsItemDeactivatedAfterEdit()) {
                meta.Name = std::string(buffer);
            }

            ImGui::PushFont(UI::FontManager::Get().GetFonts().Font14);
            ImGui::Text("UUID: %s", meta.ID.ToString().c_str());
            ImGui::PopFont();
        }
    }

    void PropertiesElement::DrawComponent_Camera(entt::registry& registry, entt::entity entity) {
        auto& component = registry.get<CameraComponent>(entity);

        if (ImGui::CollapsingHeader(ICON_LC_CAMERA " CAMERA", ImGuiTreeNodeFlags_DefaultOpen)) {

        }
    }


    void PropertiesElement::DrawComponent_Mesh(entt::registry &registry, entt::entity entity) {
        auto& component = registry.get<MeshComponent>(entity);

        if (ImGui::CollapsingHeader(ICON_LC_GRID_3X3 " MESH", ImGuiTreeNodeFlags_DefaultOpen)) {
            ImGui::Text("Mesh:");

            ImGui::SameLine();

            std::string meshLabel = (component.data != nullptr && !component.data->GetPath().empty()) ? component.data->GetPath() : "None";
            if (ImGui::Button(meshLabel.c_str(), ImVec2(200, 0))) {
                nfdchar_t *outPath = nullptr;
                nfdresult_t result = NFD_OpenDialog( "gltf, fbx", nullptr, &outPath );

                if (result == NFD_OKAY) {
                    std::cout << "Selected file: " << outPath << std::endl;

                    Engine::EventBus::Get().Dispatch(UICommandEvent(UICommand::LoadMesh, 
                        {
                        { "entity", entity }, 
                        { "path", std::string(outPath) }
                        }
                    ));


                    NFDi_Free(outPath);
                } else if (result == NFD_CANCEL) {
                    std::cout << "User cancelled." << std::endl;
                } else {
                    std::cout << "Error: " << NFD_GetError() << std::endl;
                }
            }


            if (ImGui::BeginDragDropTarget()) {
                if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("MESH_ASSET")) {
                    const char* outPath = static_cast<const char*>(payload->Data);
                    if (outPath) {
                        Engine::EventBus::Get().Dispatch(UICommandEvent(UICommand::LoadMesh, 
                            {
                            { "entity", entity }, 
                            { "path", std::string(outPath) }
                            }
                        ));
                    }
                }
                ImGui::EndDragDropTarget();
            }
        }
    }
}
