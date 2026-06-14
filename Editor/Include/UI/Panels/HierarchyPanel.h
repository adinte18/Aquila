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
	void SetOnEntitySelected(Delegate<void(Aquila::SceneManagement::Entity)> callback);

  private:
	Aquila::SceneManagement::EntityManager &m_EntityManager;
	Delegate<void(Aquila::SceneManagement::Entity)> m_OnEntitySelected;
	HierarchyTreeView *m_TreeView = nullptr;
	HierarchyTreeNode *m_SelectedNode = nullptr;
};

} // namespace Editor
