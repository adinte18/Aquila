#include "UI/Panels/HierarchyPanel.h"

#include "UI/Panels/HierarchyTreeNode.h"
#include "UI/Panels/HierarchyTreeView.h"
#include "Aquila/Foundation/Macros.h"
#include "Aquila/Scene/Components/MetadataComponent.h"
#include "Aquila/Scene/Components/SceneNodeComponent.h"
#include "Aquila/Scene/EntityManager.h"
#include "Aquila/UI/Widgets/Button.h"
#include "Aquila/UI/Widgets/PopupMenu.h"
#include "Aquila/UI/Widgets/DockPanel.h"
#include "Aquila/UI/Widgets/ScrollView.h"

namespace Editor {

using namespace Aquila;
using namespace Aquila::SceneManagement;
using namespace Aquila::SceneManagement::Components;

HierarchyPanel::HierarchyPanel(EntityManager &entity_manager) : m_entity_manager(entity_manager) {}

void HierarchyPanel::build(UI::Core::DockPanel *panel, UI::Core::View *overlay_root) {
	auto ctx_uniq = std::make_unique<UI::Core::PopupMenu>();
	auto *ctx = dynamic_cast<UI::Core::PopupMenu *>(overlay_root->add_child(std::move(ctx_uniq)));
	ctx->add_item("Create Empty", [this] {
		auto entity = m_entity_manager.create_entity("New Entity");
		m_tree_view->add_entity_node(entity.get_name(), entity);
	});
	ctx->add_item("Create Cube", [] { AQUILA_LOG_INFO("HierarchyPanel: Create Cube (not yet implemented)"); });

	{
		auto *scroll = panel->find_by_id<UI::Core::ScrollView>("hierarchy-scroll");
		if (scroll == nullptr) {
			AQUILA_LOG_ERROR("HierarchyPanel: 'hierarchy-scroll' not found in layout");
			return;
		}

		auto tree_uniq = std::make_unique<HierarchyTreeView>(m_entity_manager);
		m_tree_view = dynamic_cast<HierarchyTreeView *>(scroll->add_child(std::move(tree_uniq)));

		auto node_ctx_uniq = std::make_unique<UI::Core::PopupMenu>();
		auto *node_context_menu = dynamic_cast<UI::Core::PopupMenu *>(m_tree_view->add_child(std::move(node_ctx_uniq)));

		node_context_menu->add_item("Add child", [this] {
			if (m_selected_node != nullptr) {
				auto parent_entity = m_selected_node->get_entity();
				auto new_entity = m_entity_manager.create_entity("Empty entity");
				m_entity_manager.add_child(parent_entity, new_entity);

				m_tree_view->add_entity_node(new_entity.get_name(), new_entity, m_selected_node);
				m_selected_node->set_expanded(true);
			}
		});

		node_context_menu->add_item("Delete entity", [this] {
			if (m_selected_node != nullptr) {
				auto parent_entity = m_selected_node->get_entity();
				auto new_entity = m_entity_manager.create_entity("Empty entity");
				m_entity_manager.add_child(parent_entity, new_entity);

				m_tree_view->add_entity_node(new_entity.get_name(), new_entity, m_selected_node);
				m_selected_node->set_expanded(true);
			}
		});

		populate_tree();

		m_tree_view->on_entity_selected.connect([this](Entity entity) {
			m_selected_node = m_tree_view->find_node_for_entity(entity);
			on_entity_selected(entity);
		});

		m_tree_view->on_deselected.connect([this] {
			m_selected_node = nullptr;
			on_entity_deselected();
		});

		m_tree_view->on_entity_right_clicked.connect([node_context_menu, this](Entity entity, Vec2 pos) {
			m_selected_node = m_tree_view->find_node_for_entity(entity);
			node_context_menu->open_at(pos);
		});

		m_tree_view->set_on_background_right_clicked([ctx](Vec2 pos) { ctx->open_at(pos); });
	}
}

void HierarchyPanel::add_entity(Entity entity) {
	if (m_tree_view != nullptr) {
		m_tree_view->add_entity_node(entity.get_name(), entity);
	}
}

void HierarchyPanel::rebuild() {
	if (m_tree_view == nullptr) {
		return;
	}
	m_tree_view->clear();
	m_selected_node = nullptr;
	populate_tree();
}

void HierarchyPanel::populate_tree() {
	m_entity_manager.for_each<MetadataComponent>([this](Entity entity) {
		auto *scene_node = entity.try_get_component<SceneNodeComponent>();
		if (scene_node == nullptr || !scene_node->parent.is_valid()) {
			m_tree_view->populate_from_entity(entity);
		}
	});
}

void HierarchyPanel::refresh_entity(Entity entity) {
	if (m_tree_view == nullptr) {
		return;
	}
	if (HierarchyTreeNode *node = m_tree_view->find_node_for_entity(entity)) {
		node->set_label(entity.get_name());
	}
}

void HierarchyPanel::set_tree_icons(Aquila::GFX::GfxTexture *collapsed, Aquila::GFX::GfxTexture *expanded) {
	if (m_tree_view != nullptr) {
		m_tree_view->set_expand_icons(collapsed, expanded);
	}
}

} // namespace Editor
