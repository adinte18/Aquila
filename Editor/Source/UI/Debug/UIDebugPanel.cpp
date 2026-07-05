#include "UI/Debug/UIDebugPanel.h"

#include "Aquila/UI/Core/Canvas.h"
#include "Aquila/UI/Core/View.h"
#include "Aquila/UI/Style/StyleProperties.h"
#include "Aquila/UI/Widgets/Button.h"
#include "Aquila/UI/Widgets/Label.h"
#include "Aquila/UI/Widgets/ScrollView.h"
#include "Aquila/UI/Widgets/TreeView.h"

namespace Editor {

using namespace Aquila;
using namespace Aquila::UI::Core;

namespace {

std::string MakeLabel(View *view) {
	std::string label(view->GetTypeName());
	if (!view->GetId().empty()) {
		label += " #" + view->GetId();
	}
	for (const auto &cls : view->GetClasses()) {
		label += " ." + cls;
	}
	return label;
}

} // namespace

void UIDebugPanel::Build(View *overlayRoot, Canvas *target) {
	m_Target = target;

	m_Window = overlayRoot->AddChild<View>();
	m_Window->SetId("ui-debug-window");
	m_Window->AddClass("ui-debug-window");

	UI::FloatingConfig floating;
	floating.attachTo = UI::FloatingAttachTo::Root;
	floating.parentPoint = UI::FloatingAttachPoint::LeftTop;
	floating.elementPoint = UI::FloatingAttachPoint::LeftTop;
	floating.offset = { 60.f, 60.f };
	floating.zIndex = 1000;
	m_Window->SetFloating(floating);

	auto *header = m_Window->AddChild<View>();
	header->AddClass("ui-debug-header");

	auto *title = header->AddChild<Label>(std::string("UI Inspector"));
	title->AddClass("ui-debug-title");

	auto *refresh = header->AddChild<Button>(std::string("Refresh"));
	refresh->AddClass("ui-debug-refresh");
	refresh->onClick.Connect([this] { Refresh(); });

	auto *scroll = m_Window->AddChild<ScrollView>();
	scroll->AddClass("ui-debug-body");
	m_TreeHost = scroll->AddContent<View>();
	m_TreeHost->AddClass("ui-debug-tree-host");

	m_Window->SetHidden(true);
}

void UIDebugPanel::Toggle() {
	if (!m_Window) {
		return;
	}

	m_Visible = !m_Visible;

	m_Window->SetHidden(!m_Visible);

	if (m_Visible) {
		Refresh();
	}
}

TreeNode *UIDebugPanel::AddViewNode(View *view, TreeNode *parentNode) {
	TreeNode *node = parentNode ? parentNode->AddChildNode(MakeLabel(view)) : m_Tree->AddNode(MakeLabel(view));
	m_NodeToView[node] = view;

	for (const auto &child : view->GetChildren()) {
		if (child.get() == m_Window) {
			continue; // never inspect the inspector
		}
		AddViewNode(child.get(), node);
	}
	return node;
}

void UIDebugPanel::Refresh() {
	if (!m_Target || !m_TreeHost) {
		return;
	}

	m_NodeToView.clear();
	while (!m_TreeHost->GetChildren().empty()) {
		m_TreeHost->RemoveChild(m_TreeHost->GetChildren().front().get());
	}

	m_Tree = m_TreeHost->AddChild<TreeView>();

	for (const auto &child : m_Target->GetRoot()->GetChildren()) {
		if (child.get() == m_Window) {
			continue;
		}
		AddViewNode(child.get(), nullptr);
	}
}

} // namespace Editor
