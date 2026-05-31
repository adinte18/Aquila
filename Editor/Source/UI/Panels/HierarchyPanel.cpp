#include "UI/Panels/HierarchyPanel.h"

#include "Aquila/Foundation/Macros.h"
#include "Aquila/Scene/EntityManager.h"
#include "Aquila/Scene/Components/MetadataComponent.h"
#include "Aquila/UI/Widgets/ContextMenu.h"
#include "Aquila/UI/Widgets/DockPanel.h"
#include "Aquila/UI/Widgets/TreeView.h"

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

	auto treeUniq = CreateUnique<UI::Core::TreeView>();
	m_TreeView = static_cast<UI::Core::TreeView *>(panel->AddChild(std::move(treeUniq)));

	m_EntityManager.ForEach<MetadataComponent>([this](Entity entity) {
		auto *node = m_TreeView->AddNode(entity.GetName());
		m_NodeEntityMap[node] = entity;
	});

	m_TreeView->SetOnSelected([this](UI::Core::TreeNode *node) {
		auto it = m_NodeEntityMap.find(node);
		if (it != m_NodeEntityMap.end() && m_OnEntitySelected) {
			m_OnEntitySelected(it->second);
		}
	});

	m_TreeView->SetOnRightClicked([ctx](UI::Core::TreeNode *, vec2 pos) { ctx->OpenAt(pos); });
}

void HierarchyPanel::SetOnEntitySelected(std::function<void(Entity)> callback) {
	m_OnEntitySelected = std::move(callback);
}

} // namespace Editor
