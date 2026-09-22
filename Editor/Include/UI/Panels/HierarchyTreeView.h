#pragma once

#include "Aquila/Scene/Entity.h"
#include "Aquila/UI/Widgets/TreeView.h"
#include <unordered_map>

namespace Aquila::SceneManagement {
class EntityManager;
}

namespace Aquila::GFX {
class GfxTexture;
}

namespace Editor {

class HierarchyTreeNode;

struct EntityIcons {
	Aquila::GFX::GfxTexture *empty = nullptr;
	Aquila::GFX::GfxTexture *mesh = nullptr;
	Aquila::GFX::GfxTexture *light = nullptr;
	Aquila::GFX::GfxTexture *camera = nullptr;
	Aquila::GFX::GfxTexture *sky = nullptr;
};

class HierarchyTreeView : public Aquila::UI::Core::TreeView {
  public:
	explicit HierarchyTreeView(Aquila::SceneManagement::EntityManager &entity_manager);

	HierarchyTreeNode *add_entity_node(std::string label, Aquila::SceneManagement::Entity entity,
									   HierarchyTreeNode *parent = nullptr);
	void delete_entity_node(Aquila::SceneManagement::Entity entity);
	void populate_from_entity(Aquila::SceneManagement::Entity entity, HierarchyTreeNode *parent = nullptr);
	void clear();
	void set_entity_icons(const EntityIcons &icons);
	void refresh_node_icon(HierarchyTreeNode *node);
	[[nodiscard]] HierarchyTreeNode *find_node_for_entity(Aquila::SceneManagement::Entity entity) const;
	[[nodiscard]] Aquila::SceneManagement::EntityManager &get_entity_manager() { return m_entity_manager; }

	Signal<void(Aquila::SceneManagement::Entity)> on_entity_selected;
	Signal<void(Aquila::SceneManagement::Entity, Vec2)> on_entity_right_clicked;

  private:
	void on_drop(Aquila::UI::Core::DragState &state) override;

	[[nodiscard]] Aquila::GFX::GfxTexture *icon_for(Aquila::SceneManagement::Entity entity) const;

	EntityIcons m_icons;
	Aquila::SceneManagement::EntityManager &m_entity_manager;
	std::unordered_map<HierarchyTreeNode *, Aquila::SceneManagement::Entity> m_node_entity_map;
};

} // namespace Editor
