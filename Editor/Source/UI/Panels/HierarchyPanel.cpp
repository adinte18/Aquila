#include "UI/Panels/HierarchyPanel.h"

#include "UI/Panels/HierarchyTreeNode.h"
#include "UI/Panels/HierarchyTreeView.h"
#include "Aquila/Foundation/Macros.h"
#include "Aquila/Scene/Components/MetadataComponent.h"
#include "Aquila/Scene/EntityManager.h"
#include "Aquila/UI/Widgets/ContextMenu.h"
#include "Aquila/UI/Widgets/DockPanel.h"

namespace Editor {

using namespace Aquila;
using namespace Aquila::SceneManagement;
using namespace Aquila::SceneManagement::Components;

HierarchyPanel::HierarchyPanel(EntityManager &entityManager) : m_EntityManager(entityManager) {}

void HierarchyPanel::Build(UI::Core::DockPanel *panel, UI::Core::View *overlayRoot) {
	auto ctxUniq = CreateUnique<UI::Core::ContextMenu>();
	auto *ctx = static_cast<UI::Core::ContextMenu *>(overlayRoot->AddChild(std::move(ctxUniq)));
	ctx->AddItem("Create Empty", [] { AQUILA_LOG_INFO("HierarchyPanel: Create Empty"); });
	ctx->AddItem("Create Cube", [] { AQUILA_LOG_INFO("HierarchyPanel: Create Cube"); });
	panel->SetContextView([ctx](vec2 pos) { ctx->OpenAt(pos); });

	auto treeUniq = CreateUnique<HierarchyTreeView>(m_EntityManager);
	m_TreeView = static_cast<HierarchyTreeView *>(panel->AddChild(std::move(treeUniq)));

	auto nodeCtxUniq = CreateUnique<UI::Core::ContextMenu>();
	auto *nodeContextMenu =
		static_cast<UI::Core::ContextMenu *>(m_TreeView->View::AddChild(std::move(nodeCtxUniq)));
	nodeContextMenu->AddItem("Add child", [this] {
		if (m_SelectedNode != nullptr) {
			AQUILA_LOG_DEBUG("Hello");
		}
	});

	m_EntityManager.ForEach<MetadataComponent>([this](Entity entity) {
		m_TreeView->AddEntityNode(entity.GetName(), entity);
	});

	m_TreeView->SetOnSelected([this](UI::Core::TreeNode *node) {
		auto *hierarchyNode = static_cast<HierarchyTreeNode *>(node);
		m_SelectedNode = hierarchyNode;
		if (m_OnEntitySelected) {
			m_OnEntitySelected(hierarchyNode->GetEntity());
		}
	});

	m_TreeView->SetOnRightClicked([nodeContextMenu, this](UI::Core::TreeNode *node, vec2 pos) {
		m_SelectedNode = static_cast<HierarchyTreeNode *>(node);
		nodeContextMenu->OpenAt(pos);
	});
}

void HierarchyPanel::SetOnEntitySelected(std::function<void(Entity)> callback) {
	m_OnEntitySelected = std::move(callback);
}

} // namespace Editor
