#pragma once

#include "UI/Panels/IEditorPanel.h"
#include "Aquila/Scene/Entity.h"

namespace Aquila::SceneManagement {
class EntityManager;
}

namespace Editor {

class HierarchyTreeView;
class HierarchyTreeNode;

class HierarchyPanel : public IEditorPanel {
  public:
	explicit HierarchyPanel(Aquila::SceneManagement::EntityManager &entity_manager);
	void build(Aquila::UI::Core::DockPanel *panel, Aquila::UI::Core::View *overlay_root) override;
	Signal<void(Aquila::SceneManagement::Entity)> on_entity_selected;
	void add_entity(Aquila::SceneManagement::Entity entity);

  private:
	Aquila::SceneManagement::EntityManager &m_entity_manager;
	HierarchyTreeView *m_tree_view = nullptr;
	HierarchyTreeNode *m_selected_node = nullptr;
};

} // namespace Editor
