#pragma once

#include "Aquila/Scene/Entity.h"
#include "Aquila/UI/Widgets/TreeView.h"

namespace Aquila::SceneManagement {
class EntityManager;
}

namespace Editor {

class HierarchyTreeView;

class HierarchyTreeNode : public Aquila::UI::Core::TreeNode {
  public:
	HierarchyTreeNode(std::string label, HierarchyTreeView &owner, int depth,
	                  Aquila::SceneManagement::Entity entity,
	                  Aquila::SceneManagement::EntityManager &entityManager);

	[[nodiscard]] Aquila::SceneManagement::Entity GetEntity() const { return m_Entity; }

  private:
	void OnDragStart(Aquila::UI::Core::DragState &state) override;
	void OnDrop(Aquila::UI::Core::DragState &state) override;

	Aquila::SceneManagement::Entity m_Entity;
	Aquila::SceneManagement::EntityManager &m_EntityManager;
	HierarchyTreeView &m_HierarchyView;
};

} // namespace Editor
