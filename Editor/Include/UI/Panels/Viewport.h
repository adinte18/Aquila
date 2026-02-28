#ifndef VIEWPORT_PANEL_H
#define VIEWPORT_PANEL_H

#include "Aquila/Core/Layer.h"
#include "Aquila/Scene/Entity.h"
#include "imgui.h"
#include "ImGuizmo.h"

namespace Aquila::Core {
class Application;
}

namespace Editor {

class ViewportPanel : public Aquila::Core::Layer {
  public:
	ViewportPanel(Aquila::Core::Application &app);
	~ViewportPanel() override = default;

	void OnAttach() override;
	void OnDetach() override;
	void OnUpdate(f32 deltaTime) override;
	void OnRender() override;
	void OnImGuiRender() override;
	void OnEvent(Aquila::Events::Event &event) override;

  private:
	// TODO : Rethink this, not performant at all!
	struct ObjectPickingResult {
		bool hit = false;
		f32 distance = std::numeric_limits<f32>::max();
		Aquila::SceneManagement::Entity entity = Aquila::SceneManagement::Entity::Null();
		glm::vec3 hitPoint;
	};

	void DrawGizmos();
	void DrawGizmoToolbarOverlay(ImVec2 viewportBounds[2]);

	void HandleCameraControls(const ImVec2 &viewportSize);
	void HandleMouseInput(f32 mouseX, f32 mouseY, const ImVec2 &viewportSize);
	void HandleKeyboardInput() const;
	void HandleMouseWheel(const ImVec2 &viewportSize) const;

	void HandleObjectPicking(const ImVec2 &viewportSize);
	[[nodiscard]] ObjectPickingResult PerformObjectPicking(const glm::vec2 &mousePos,
														   const glm::vec2 &viewportSize) const;

	void OnViewportResize(const ImVec2 &newSize);

	Aquila::Core::Application &m_App;
	Aquila::SceneManagement::Entity m_SelectedEntity = Aquila::SceneManagement::Entity::Null();

	ImGuizmo::OPERATION m_GizmoOperation = ImGuizmo::TRANSLATE;
	ImGuizmo::MODE m_GizmoMode = ImGuizmo::WORLD;
	bool m_ShowGizmo = true;
	bool m_UseSnap = false;
	f32 m_SnapValues[3] = { 1.0f, 1.0f, 1.0f };
	f32 m_RotationSnap = 15.0f;
	f32 m_ScaleSnap = 0.1f;

	bool m_IsFirstMouse = true;
	ImVec2 m_LastViewportSize = { 0, 0 };
	bool m_IsDetached = false;
};

} // namespace Editor

#endif
