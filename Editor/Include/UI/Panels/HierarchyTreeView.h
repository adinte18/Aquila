#pragma once

#include "Aquila/Scene/Entity.h"
#include "Aquila/UI/Widgets/TreeView.h"
#include <unordered_map>

namespace Aquila::SceneManagement {
class EntityManager;
}

namespace Editor {

class HierarchyTreeNode;

class HierarchyTreeView : public Aquila::UI::Core::TreeView {
  public:
	explicit HierarchyTreeView(Aquila::SceneManagement::EntityManager &entityManager);

	HierarchyTreeNode *AddEntityNode(std::string label, Aquila::SceneManagement::Entity entity);
	[[nodiscard]] HierarchyTreeNode *FindNodeForEntity(Aquila::SceneManagement::Entity entity) const;
	[[nodiscard]] Aquila::SceneManagement::EntityManager &GetEntityManager() { return m_EntityManager; }

  private:
	void OnDrop(Aquila::UI::Core::DragState &state) override;

	Aquila::SceneManagement::EntityManager &m_EntityManager;
	std::unordered_map<HierarchyTreeNode *, Aquila::SceneManagement::Entity> m_NodeEntityMap;
};

} // namespace Editor
