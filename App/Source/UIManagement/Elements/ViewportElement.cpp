#include "Elements/ViewportElement.h"
#include "UI.h"
#include "imgui.h"
#include "lucide.h"

namespace Editor::UIManagement {
    void ViewportElement::Draw(void* data) {
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

        auto textureId = reinterpret_cast<ImTextureID>(Editor::UIManager::FetchRenderedImage());
        ImGui::Image(textureId, { viewportSize.x, viewportSize.y }, { 0, 1 }, { 1, 0 });

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


        ImGui::End();
    }
};