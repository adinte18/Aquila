#ifndef VIEWPORT_ELEM_H
#define VIEWPORT_ELEM_H

#include "UI/Panels/IPanel.h"

namespace Editor::Panels {
class Viewport : public IPanel {
private:
  struct ObjectPickingResult {
    bool hit = false;
    float distance = std::numeric_limits<float>::max();
    entt::entity entityID = entt::null;
    glm::vec3 hitPoint;
  };

  ImGuizmo::OPERATION m_GizmoOperation = ImGuizmo::TRANSLATE;
  ImGuizmo::MODE m_GizmoMode = ImGuizmo::WORLD;
  bool m_ShowGizmo = true;
  bool m_UseSnap = false;
  float m_SnapValues[3] = {1.0f, 1.0f, 1.0f};
  float m_RotationSnap = 15.0f;
  float m_ScaleSnap = 0.1f;

  void DrawGizmoToolbarOverlay(ImVec2 viewportBounds[2]);
  void DrawGizmos();
  void HandleCameraControls(const ImVec2 &viewportSize);
  void HandleMouseInput(float mouseX, float mouseY, const ImVec2 &viewportSize);
  void HandleKeyboardInput();
  void HandleMouseWheel(const ImVec2 &viewportSize);
  void HandleObjectPicking(const ImVec2 &viewportSize);
  ObjectPickingResult PerformObjectPicking(const glm::vec2 &mousePos,
                                           const glm::vec2 &viewportSize);

  bool m_IsFirstMouse = true;

public:
  void Draw() override;
};
} // namespace Editor::Panels

#endif