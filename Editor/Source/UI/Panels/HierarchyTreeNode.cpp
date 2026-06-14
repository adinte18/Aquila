#include "UI/Panels/HierarchyTreeNode.h"

#include "UI/Panels/HierarchyTreeView.h"
#include "Aquila/Foundation/Macros.h"
#include "Aquila/Scene/EntityManager.h"

namespace Editor {

using namespace Aquila::SceneManagement;
using namespace Aquila::UI::Core;

HierarchyTreeNode::HierarchyTreeNode(std::string label, HierarchyTreeView &owner, int depth, Entity entity,
									 EntityManager &entityManager)
	: TreeNode(std::move(label), owner, depth), m_Entity(entity), m_EntityManager(entityManager),
	  m_HierarchyView(owner) {
	m_IsDraggable = true;
	m_IsAcceptingPayload = true;
}

void HierarchyTreeNode::OnDragStart(DragState &state) {
	state.payload = m_Entity;
}

void HierarchyTreeNode::OnDrop(DragState &state) {
	auto draggedEntity = std::any_cast<Entity>(state.payload);
	if (draggedEntity == m_Entity) {
		return;
	}

	HierarchyTreeNode *sourceNode = m_HierarchyView.FindNodeForEntity(draggedEntity);
	if (sourceNode == nullptr || sourceNode == this) {
		return;
	}

	m_EntityManager.AddChild(m_Entity, draggedEntity);

	View *oldParentContainer = sourceNode->GetParent();
	auto *oldParentNode = dynamic_cast<TreeNode *>(oldParentContainer ? oldParentContainer->GetParent() : nullptr);

	auto detached = oldParentContainer->DetachChild(sourceNode);

	if (oldParentNode) {
		oldParentNode->QueueRedraw();
	}

	auto *newNode = static_cast<HierarchyTreeNode *>(AddChild(std::move(detached)));
	if (newNode) {
		newNode->UpdateDepth(GetDepth() + 1);
		SetExpanded(true);
	}
}

} // namespace Editor
