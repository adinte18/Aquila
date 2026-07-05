#include "UI/Panels/HierarchyTreeView.h"

#include "UI/Panels/HierarchyTreeNode.h"
#include "Aquila/Foundation/Macros.h"
#include "Aquila/Scene/Components/SceneNodeComponent.h"
#include "Aquila/Scene/EntityManager.h"

namespace Editor {

using namespace Aquila::SceneManagement;
using namespace Aquila::SceneManagement::Components;
using namespace Aquila::UI::Core;

HierarchyTreeView::HierarchyTreeView(EntityManager &entity_manager) : m_entity_manager(entity_manager) {
	m_is_accepting_payload = true;
	m_is_draggable = false;

	on_selected.connect([this](Aquila::UI::Core::TreeNode *node) {
		auto *hierarchy_node = static_cast<HierarchyTreeNode *>(node);
		auto it = m_node_entity_map.find(hierarchy_node);
		if (it != m_node_entity_map.end()) {
			on_entity_selected(it->second);
		}
	});

	on_node_right_clicked.connect([this](Aquila::UI::Core::TreeNode *node, Vec2 pos) {
		auto *hierarchy_node = static_cast<HierarchyTreeNode *>(node);
		auto it = m_node_entity_map.find(hierarchy_node);
		if (it != m_node_entity_map.end()) {
			on_entity_right_clicked(it->second, pos);
		}
	});
}

HierarchyTreeNode *HierarchyTreeView::add_entity_node(std::string label, Entity entity) {
	auto node = create_unique<HierarchyTreeNode>(std::move(label), *this, 0, entity, m_entity_manager);
	auto *raw = static_cast<HierarchyTreeNode *>(add_child(std::move(node)));
	m_node_entity_map[raw] = entity;
	return raw;
}

HierarchyTreeNode *HierarchyTreeView::find_node_for_entity(Entity entity) const {
	for (auto &[node, e] : m_node_entity_map) {
		if (e == entity) {
			return node;
		}
	}
	return nullptr;
}

void HierarchyTreeView::on_drop(DragState &state) {
	AQUILA_LOG_DEBUG("HierarchyTreeView::OnDrop called");

	if (!state.payload.has_value()) {
		AQUILA_LOG_ERROR("Empty payload");
		return;
	}

	auto dragged_entity = std::any_cast<Entity>(state.payload);

	HierarchyTreeNode *source_node = find_node_for_entity(dragged_entity);
	if (source_node == nullptr) {
		AQUILA_LOG_ERROR("SourceNode not found");
		return;
	}

	if (source_node->get_parent() == m_content) {
		return;
	}

	auto *node = dragged_entity.try_get_component<Components::SceneNodeComponent>();
	if (node != nullptr && node->parent.is_valid()) {
		m_entity_manager.remove_child(node->parent, dragged_entity);
	}

	View *old_parent_container = source_node->get_parent();
	auto *old_parent_node = dynamic_cast<TreeNode *>(old_parent_container ? old_parent_container->get_parent() : nullptr);

	auto detached = old_parent_container->detach_child(source_node);

	if (old_parent_node) {
		old_parent_node->queue_redraw();
	}

	auto *moved_node = static_cast<HierarchyTreeNode *>(add_child(std::move(detached)));
	if (moved_node) {
		moved_node->update_depth(0);
	}
}

} // namespace Editor
