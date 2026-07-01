#pragma once

#include "UI/Panels/IEditorPanel.h"
#include "Aquila/Scene/Entity.h"
#include <functional>

namespace Aquila::SceneManagement {
class EntityManager;
}

namespace Editor {

class HierarchyTreeView;
class HierarchyTreeNode;

class HierarchyPanel : public IEditorPanel {
  public:
	explicit HierarchyPanel(Aquila::SceneManagement::EntityManager &entityManager);
	void Build(Aquila::UI::Core::DockPanel *panel, Aquila::UI::Core::View *overlayRoot) override;
	Signal<void(Aquila::SceneManagement::Entity)> onEntitySelected;
	void AddEntity(Aquila::SceneManagement::Entity entity);

  private:
	Aquila::SceneManagement::EntityManager &m_EntityManager;
	HierarchyTreeView *m_TreeView = nullptr;
	HierarchyTreeNode *m_SelectedNode = nullptr;
};

} // namespace Editor
