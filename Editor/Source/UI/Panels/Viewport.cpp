#include "UI/Panels/Viewport.h"
#include "AquilaPCH.h"
#include "Engine/Events/Event.h"

namespace Editor::Panels {
void Viewport::Draw() {
  ImGui::Begin(ICON_LC_CLAPPERBOARD " Scene", nullptr,
               ImGuiWindowFlags_NoCollapse);
  ImVec2 viewportSize = ImGui::GetContentRegionAvail();

  static ImVec2 lastViewportSize = {0, 0};

  if (viewportSize.x != lastViewportSize.x ||
      viewportSize.y != lastViewportSize.y) {

    auto viewportResizedEvent = CreateUnique<Engine::ViewportResizedEvent>();
    viewportResizedEvent->width = viewportSize.x;
    viewportResizedEvent->height = viewportSize.y;

    Engine::EventBus::Get()->DispatchSync(std::move(viewportResizedEvent));

    lastViewportSize = viewportSize;
  }

  auto image = Engine::Controller::Get()
                   ->GetRenderer()
                   .GetPassObject<Engine::GeometryPass>()
                   ->GetDescriptorSet();

  if (image != VK_NULL_HANDLE) {
    auto textureId = reinterpret_cast<ImTextureID>(image);
    ImGui::Image(textureId, {viewportSize.x, viewportSize.y}, {0, 1}, {1, 0});
  }

  if (ImGui::BeginDragDropTarget()) {
    if (const ImGuiPayload *payload =
            ImGui::AcceptDragDropPayload("SCENE_PATH")) {
      const char *droppedPath = static_cast<const char *>(payload->Data);
      if (droppedPath) {
        auto openSceneEvent = CreateUnique<Engine::OpenSceneEvent>();
        openSceneEvent->scenePath = droppedPath;

        Engine::EventBus::Get()->DispatchSync(std::move(openSceneEvent));
      }
    }
    ImGui::EndDragDropTarget();
  }

  if (ImGui::BeginDragDropTarget()) {
    if (const ImGuiPayload *payload =
            ImGui::AcceptDragDropPayload("MESH_ASSET")) {
      const char *meshPath = (const char *)payload->Data;
      if (meshPath) {
        auto meshLoadEvent = CreateUnique<Engine::LoadMeshEvent>();
        meshLoadEvent->meshPath = meshPath;
        Engine::EventBus::Get()->DispatchSync(std::move(meshLoadEvent));
      }
    }
    ImGui::EndDragDropTarget();
  }

  ImGui::End();
}
}; // namespace Editor::Panels