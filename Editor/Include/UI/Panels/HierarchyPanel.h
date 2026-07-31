#pragma once

#include "UI/Panels/IEditorPanel.h"
#include "Aquila/Scene/Entity.h"

namespace Aquila::SceneManagement {
class EntityManager;
}

namespace Aquila::GFX {
class GfxTexture;
}

namespace Editor {

class HierarchyTreeView;
class HierarchyTreeNode;

class HierarchyPanel : public IEditorPanel {
  public:
	explicit HierarchyPanel(Aquila::SceneManagement::EntityManager &entity_manager);
	void build(Aquila::UI::Core::DockPanel *panel, Aquila::UI::Core::View *overlay_root) override;
	Signal<void(Aquila::SceneManagement::Entity)> on_entity_selected;
	Signal<void()> on_entity_deselected;
	void add_entity(Aquila::SceneManagement::Entity entity);
	void refresh_entity(Aquila::SceneManagement::Entity entity);
	void rebuild();
	void set_tree_icons(Aquila::GFX::GfxTexture *collapsed, Aquila::GFX::GfxTexture *expanded);

  private:
	void populate_tree();

	Aquila::SceneManagement::EntityManager &m_entity_manager;
	HierarchyTreeView *m_tree_view = nullptr;
	HierarchyTreeNode *m_selected_node = nullptr;
};

} // namespace Editor
