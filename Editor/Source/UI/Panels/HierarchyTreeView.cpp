#include "UI/Panels/HierarchyTreeView.h"

#include "UI/Panels/HierarchyTreeNode.h"
#include "Aquila/Foundation/Macros.h"
#include "Aquila/Scene/Components/SceneNodeComponent.h"
#include "Aquila/Scene/EntityManager.h"

namespace Editor {

using namespace Aquila::SceneManagement;
using namespace Aquila::SceneManagement::Components;
using namespace Aquila::UI::Core;

HierarchyTreeView::HierarchyTreeView(EntityManager &entityManager) : m_EntityManager(entityManager) {
	m_IsAcceptingPayload = true;
	m_IsDraggable = false;

	onSelected.Connect([this](Aquila::UI::Core::TreeNode *node) {
		auto *hierarchyNode = static_cast<HierarchyTreeNode *>(node);
		auto it = m_NodeEntityMap.find(hierarchyNode);
		if (it != m_NodeEntityMap.end()) {
			onEntitySelected(it->second);
		}
	});

	onNodeRightClicked.Connect([this](Aquila::UI::Core::TreeNode *node, vec2 pos) {
		auto *hierarchyNode = static_cast<HierarchyTreeNode *>(node);
		auto it = m_NodeEntityMap.find(hierarchyNode);
		if (it != m_NodeEntityMap.end()) {
			onEntityRightClicked(it->second, pos);
		}
	});
}

HierarchyTreeNode *HierarchyTreeView::AddEntityNode(std::string label, Entity entity) {
	auto node = CreateUnique<HierarchyTreeNode>(std::move(label), *this, 0, entity, m_EntityManager);
	auto *raw = static_cast<HierarchyTreeNode *>(AddChild(std::move(node)));
	m_NodeEntityMap[raw] = entity;
	return raw;
}

HierarchyTreeNode *HierarchyTreeView::FindNodeForEntity(Entity entity) const {
	for (auto &[node, e] : m_NodeEntityMap) {
		if (e == entity) {
			return node;
		}
	}
	return nullptr;
}

void HierarchyTreeView::OnDrop(DragState &state) {
	AQUILA_LOG_DEBUG("HierarchyTreeView::OnDrop called");

	if (!state.payload.has_value()) {
		AQUILA_LOG_ERROR("Empty payload");
		return;
	}

	auto draggedEntity = std::any_cast<Entity>(state.payload);

	HierarchyTreeNode *sourceNode = FindNodeForEntity(draggedEntity);
	if (sourceNode == nullptr) {
		AQUILA_LOG_ERROR("SourceNode not found");
		return;
	}

	if (sourceNode->GetParent() == m_Content) {
		return;
	}

	auto *node = draggedEntity.TryGetComponent<Components::SceneNodeComponent>();
	if (node != nullptr && node->Parent.IsValid()) {
		m_EntityManager.RemoveChild(node->Parent, draggedEntity);
	}

	View *oldParentContainer = sourceNode->GetParent();
	auto *oldParentNode = dynamic_cast<TreeNode *>(oldParentContainer ? oldParentContainer->GetParent() : nullptr);

	auto detached = oldParentContainer->DetachChild(sourceNode);

	if (oldParentNode) {
		oldParentNode->QueueRedraw();
	}

	auto *movedNode = static_cast<HierarchyTreeNode *>(AddChild(std::move(detached)));
	if (movedNode) {
		movedNode->UpdateDepth(0);
	}
}

} // namespace Editor
