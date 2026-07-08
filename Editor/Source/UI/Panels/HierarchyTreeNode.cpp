#include "UI/Panels/HierarchyTreeNode.h"

#include "UI/Panels/HierarchyTreeView.h"
#include "Aquila/Foundation/Macros.h"
#include "Aquila/Scene/EntityManager.h"

namespace Editor {

using namespace Aquila::SceneManagement;
using namespace Aquila::UI::Core;

HierarchyTreeNode::HierarchyTreeNode(std::string label, HierarchyTreeView &owner, int depth, Entity entity,
									 EntityManager &entity_manager)
	: TreeNode(std::move(label), owner, depth), m_entity(entity), m_entity_manager(entity_manager),
	  m_hierarchy_view(owner) {
	m_is_draggable = true;
	m_is_accepting_payload = true;
}

void HierarchyTreeNode::on_drag_start(DragState &state) {
	state.payload = m_entity;
}

void HierarchyTreeNode::on_drop(DragState &state) {
	if (!state.payload.has_value()) {
		return;
	}
	auto dragged_entity = std::any_cast<Entity>(state.payload);
	if (dragged_entity == m_entity) {
		return;
	}

	HierarchyTreeNode *source_node = m_hierarchy_view.find_node_for_entity(dragged_entity);
	if (source_node == nullptr || source_node == this) {
		return;
	}

	m_entity_manager.add_child(m_entity, dragged_entity);

	View *old_parent_container = source_node->get_parent();
	auto *old_parent_node = dynamic_cast<TreeNode *>(old_parent_container ? old_parent_container->get_parent() : nullptr);

	auto detached = old_parent_container->detach_child(source_node);

	if (old_parent_node) {
		old_parent_node->queue_redraw();
	}

	auto *new_node = static_cast<HierarchyTreeNode *>(add_child(std::move(detached)));
	if (new_node) {
		new_node->update_depth(get_depth() + 1);
		set_expanded(true);
	}
}

} // namespace Editor
