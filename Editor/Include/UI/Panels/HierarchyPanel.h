#pragma once

#include "UI/Panels/IEditorPanel.h"
#include "Aquila/Scene/Entity.h"
#include <functional>
#include <unordered_map>

namespace Aquila::SceneManagement {
class EntityManager;
}

namespace Aquila::UI::Core {
class TreeView;
class TreeNode;
} // namespace Aquila::UI::Core

namespace Editor {

class HierarchyPanel : public IEditorPanel {
  public:
	explicit HierarchyPanel(Aquila::SceneManagement::EntityManager &entityManager);
	void Build(Aquila::UI::Core::DockPanel *panel, Aquila::UI::Core::View *overlayRoot) override;
	void SetOnEntitySelected(std::function<void(Aquila::SceneManagement::Entity)> callback);

  private:
	Aquila::SceneManagement::EntityManager &m_EntityManager;
	std::function<void(Aquila::SceneManagement::Entity)> m_OnEntitySelected;
	Aquila::UI::Core::TreeView *m_TreeView = nullptr;
	std::unordered_map<Aquila::UI::Core::TreeNode *, Aquila::SceneManagement::Entity> m_NodeEntityMap;
};

} // namespace Editor
