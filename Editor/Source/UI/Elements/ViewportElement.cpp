#include "UI/Elements/ViewportElement.h"
#include "Scene/Components/SceneNodeComponent.h"

namespace Editor::Elements {
    void ViewportElement::Draw() {
        ImGui::Begin(ICON_LC_CLAPPERBOARD " Scene", nullptr, ImGuiWindowFlags_NoCollapse);
        ImVec2 viewportSize = ImGui::GetContentRegionAvail();

        static ImVec2 lastViewportSize = { 0, 0 }; // Store last size

        // Check for resize
        if (viewportSize.x != lastViewportSize.x || viewportSize.y != lastViewportSize.y) {
            // Dispatch resize event
            std::unordered_map<std::string_view, CommandParam> params = {
                { "Width", static_cast<int>(viewportSize.x) },
                { "Height", static_cast<int>(viewportSize.y) }
            };

            Engine::EventBus::Get().Dispatch(UICommandEvent{
                UICommand::ViewportResized,
                params
            });

            lastViewportSize = viewportSize;
        }

        auto image = Engine::Controller::Get().GetRenderer().GetPassObject<Engine::GeometryPass>()->GetDescriptorSet();

        if (image != VK_NULL_HANDLE) {
            auto textureId = reinterpret_cast<ImTextureID>(image);
            ImGui::Image(textureId, { viewportSize.x, viewportSize.y }, { 0, 1 }, { 1, 0 });
        }

        if (ImGui::BeginDragDropTarget()) {
            if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("SCENE_PATH")) {
                const char* droppedPath = static_cast<const char*>(payload->Data);
                if (droppedPath) {
                    Engine::EventBus::Get().Dispatch(UICommandEvent{
                        UICommand::OpenScene,
                        {{"path", droppedPath}},
                        nullptr
                    });
                }
            }
            ImGui::EndDragDropTarget();
        }

        if (ImGui::BeginDragDropTarget()) {
            if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("MESH_ASSET")) {
                const char* meshPath = (const char*)payload->Data;
                if (meshPath) {

                    Engine::EventBus::Get().Dispatch(UICommandEvent(UICommand::LoadMesh, 
                        {
                            { "path", std::string(meshPath) }
                        }
                    ));
                }

            }
            ImGui::EndDragDropTarget();
        }


        ImGui::End();
    }
};