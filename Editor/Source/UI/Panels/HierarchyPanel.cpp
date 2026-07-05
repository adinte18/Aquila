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

HierarchyPanel::HierarchyPanel(EntityManager &entityManager) : m_EntityManager(entityManager) {}

void HierarchyPanel::Build(UI::Core::DockPanel *panel, UI::Core::View *overlayRoot) {
	auto ctxUniq = CreateUnique<UI::Core::ContextMenu>();
	auto *ctx = static_cast<UI::Core::ContextMenu *>(overlayRoot->AddChild(std::move(ctxUniq)));
	ctx->AddItem("Create Empty", [this] {
		auto entity = m_EntityManager.CreateEntity("New Entity");
		m_TreeView->AddEntityNode(entity.GetName(), entity);
	});
	ctx->AddItem("Create Cube", [] { AQUILA_LOG_INFO("HierarchyPanel: Create Cube (not yet implemented)"); });

	{
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

		m_EntityManager.ForEach<MetadataComponent>(
			[this](Entity entity) { m_TreeView->AddEntityNode(entity.GetName(), entity); });

		m_TreeView->onEntitySelected.Connect([this](Entity entity) {
			m_SelectedNode = m_TreeView->FindNodeForEntity(entity);
			onEntitySelected(entity);
		});

		m_TreeView->onEntityRightClicked.Connect([nodeContextMenu, this](Entity entity, vec2 pos) {
			m_SelectedNode = m_TreeView->FindNodeForEntity(entity);
			nodeContextMenu->OpenAt(pos);
		});

		m_TreeView->SetOnBackgroundRightClicked([ctx](vec2 pos) { ctx->OpenAt(pos); });
	}
}

void HierarchyPanel::AddEntity(Entity entity) {
	if (m_TreeView) {
		m_TreeView->AddEntityNode(entity.GetName(), entity);
	}
}

} // namespace Editor
