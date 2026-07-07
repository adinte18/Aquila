#include "UI/Panels/HierarchyPanel.h"

#include "UI/Panels/HierarchyTreeNode.h"
#include "UI/Panels/HierarchyTreeView.h"
#include "Aquila/Foundation/Macros.h"
#include "Aquila/Scene/Components/MetadataComponent.h"
#include "Aquila/Scene/EntityManager.h"
#include "Aquila/UI/Widgets/Button.h"
#include "Aquila/UI/Widgets/ContextMenu.h"
#include "Aquila/UI/Widgets/DockPanel.h"

namespace Editor {

using namespace Aquila;
using namespace Aquila::SceneManagement;
using namespace Aquila::SceneManagement::Components;

HierarchyPanel::HierarchyPanel(EntityManager &entity_manager) : m_entity_manager(entity_manager) {}

void HierarchyPanel::build(UI::Core::DockPanel *panel, UI::Core::View *overlay_root) {
	auto ctx_uniq = std::make_unique<UI::Core::ContextMenu>();
	auto *ctx = static_cast<UI::Core::ContextMenu *>(overlay_root->add_child(std::move(ctx_uniq)));
	ctx->add_item("Create Empty", [this] {
		auto entity = m_entity_manager.create_entity("New Entity");
		m_tree_view->add_entity_node(entity.get_name(), entity);
	});
	ctx->add_item("Create Cube", [] { AQUILA_LOG_INFO("HierarchyPanel: Create Cube (not yet implemented)"); });

	{
		auto tree_uniq = std::make_unique<HierarchyTreeView>(m_entity_manager);
		m_tree_view = static_cast<HierarchyTreeView *>(panel->add_child(std::move(tree_uniq)));

		auto node_ctx_uniq = std::make_unique<UI::Core::ContextMenu>();
		auto *node_context_menu =
			static_cast<UI::Core::ContextMenu *>(m_tree_view->View::add_child(std::move(node_ctx_uniq)));
		node_context_menu->add_item("Add child", [this] {
			if (m_selected_node != nullptr) {
				AQUILA_LOG_DEBUG("Hello");
			}
		});

		m_entity_manager.for_each<MetadataComponent>(
			[this](Entity entity) { m_tree_view->add_entity_node(entity.get_name(), entity); });

		m_tree_view->on_entity_selected.connect([this](Entity entity) {
			m_selected_node = m_tree_view->find_node_for_entity(entity);
			on_entity_selected(entity);
		});

		m_tree_view->on_entity_right_clicked.connect([node_context_menu, this](Entity entity, Vec2 pos) {
			m_selected_node = m_tree_view->find_node_for_entity(entity);
			node_context_menu->open_at(pos);
		});

		m_tree_view->set_on_background_right_clicked([ctx](Vec2 pos) { ctx->open_at(pos); });
	}
}

void HierarchyPanel::add_entity(Entity entity) {
	if (m_tree_view) {
		m_tree_view->add_entity_node(entity.get_name(), entity);
	}
}

} // namespace Editor
