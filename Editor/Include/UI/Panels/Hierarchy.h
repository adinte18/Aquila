#ifndef SCENE_HIERARCHY_PANEL_H
#define SCENE_HIERARCHY_PANEL_H

#include "Aquila/Core/Layer.h"
#include "Aquila/Scene/Entity.h"

namespace Aquila::Core {
class Application;
}

namespace Editor {

class SceneHierarchyPanel : public Aquila::Core::Layer {
  public:
	SceneHierarchyPanel(Aquila::Core::Application &app);
	~SceneHierarchyPanel() override = default;

	void OnAttach() override;
	void OnDetach() override;
	void OnUpdate(f32 deltaTime) override;
	void OnImGuiRender() override;
	void OnEvent(Aquila::Events::Event &event) override;

	[[nodiscard]] Aquila::SceneManagement::Entity GetSelectedEntity() const { return m_SelectedEntity; }
	void SetSelectedEntity(Aquila::SceneManagement::Entity entity) { m_SelectedEntity = entity; }

  private:
	void DrawPopupMenu();
	void ToggleVisibility(Aquila::SceneManagement::Entity entity, bool value);
	void DrawEntityNode(Aquila::SceneManagement::Entity entity);
	void DrawEntityTree(Aquila::SceneManagement::Entity entityHandle);
	void DrawContextMenu();
	void HandleEntitySelection(Aquila::SceneManagement::Entity entity);

	void CreateEntity(Aquila::SceneManagement::EntityPreset preset);
	void CreateChildEntity(Aquila::SceneManagement::Entity parent);
	void DeleteEntity(Aquila::SceneManagement::Entity entity);

	Aquila::Core::Application &m_App;
	Aquila::SceneManagement::Entity m_SelectedEntity = Aquila::SceneManagement::Entity::Null();
};

} // namespace Editor

#endif // SCENE_HIERARCHY_PANEL_H
