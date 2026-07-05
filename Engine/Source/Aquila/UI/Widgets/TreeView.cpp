#include "Aquila/UI/Widgets/TreeView.h"

namespace Aquila::UI::Core {

TreeView::TreeView() {
	AddClass("tree-view");

	auto content = CreateUnique<View>();
	content->AddClass("tree-view-content");
	m_Content = View::AddChild(std::move(content));
}

TreeNode *TreeView::AddNode(std::string label) {
	auto node = CreateUnique<TreeNode>(std::move(label), *this, 0);
	return static_cast<TreeNode *>(AddChild(std::move(node)));
}

View *TreeView::AddChild(Unique<View> child) {
	return m_Content->View::AddChild(std::move(child));
}

void TreeView::OnDragEnter(DragState &) {
	AddClass("drag-target");
}

void TreeView::OnDragLeave(DragState &) {
	RemoveClass("drag-target");
}

static bool IsDescendantOf(View *candidate, View *ancestor) {
	while (candidate != nullptr) {
		if (candidate == ancestor) {
			return true;
		}
		candidate = candidate->GetParent();
	}
	return false;
}

void TreeView::RemoveNode(TreeNode *node) {
	if (node == nullptr) {
		return;
	}

	if (node->GetParent() == nullptr) {
		return;
	}

	if (m_Selected != nullptr && IsDescendantOf(m_Selected, node)) {
		m_Selected = nullptr;
	}

	node->GetParent()->RemoveChild(node);
}

void TreeView::SetOnBackgroundRightClicked(Delegate<void(vec2)> callback) {
	m_Content->onContextMenu.Set(std::move(callback));
}

void TreeView::SelectNode(TreeNode *node) {
	if (m_Selected != nullptr) {
		m_Selected->SetSelected(false);
	}
	m_Selected = node;
	if (m_Selected != nullptr) {
		m_Selected->SetSelected(true);
	}
}

void TreeView::NotifySelected(TreeNode *node) {
	SelectNode(node);
	onSelected(node);
}

void TreeView::NotifyRightClicked(TreeNode *node, vec2 pos) {
	onNodeRightClicked(node, pos);
}

static constexpr float kIndentPerDepth = 16.f;

TreeNode::TreeNode(std::string label, TreeView &owner, int depth)
	: m_Owner(owner), m_Label(std::move(label)), m_Depth(depth) {
	AddClass("tree-node");

	auto header = CreateUnique<Button>();
	{
		StyleProperties hp;
		const float indent = kIndentPerDepth * static_cast<float>(m_Depth);
		hp.padding = StyleEdges{
			StyleLength::Pixel(2.f),
			StyleLength::Pixel(4.f),
			StyleLength::Pixel(2.f),
			StyleLength::Pixel(4.f + indent),
		};
		header->SetStyle(hp);
		header->AddClass("tree-node-header");
	}
	header->onClick.Connect([this] { OnHeaderClicked(); });
	header->onContextMenu.Connect([this](vec2 pos) { OnHeaderRightClicked(pos); });
	m_Header = static_cast<Button *>(View::AddChild(std::move(header)));

	auto children = CreateUnique<View>();
	children->AddClass("tree-node-children");
	m_Children = View::AddChild(std::move(children));
	UpdateHeaderText();
}

View *TreeNode::AddChild(Unique<View> node) {
	auto *added = m_Children->View::AddChild(std::move(node));
	UpdateHeaderText();
	QueueRedraw();
	return added;
}

void TreeNode::OnDragEnter(DragState &) {
	m_Header->AddClass("drag-target");
}

void TreeNode::OnDragLeave(DragState &) {
	m_Header->RemoveClass("drag-target");
}

TreeNode *TreeNode::AddChildNode(std::string label) {
	auto node = CreateUnique<TreeNode>(std::move(label), m_Owner, m_Depth + 1);
	return static_cast<TreeNode *>(AddChild(std::move(node)));
}

void TreeNode::UpdateDepth(int newDepth) {
	m_Depth = newDepth;

	const float indent = kIndentPerDepth * static_cast<float>(m_Depth);
	StyleProperties hp;
	hp.padding = StyleEdges{
		StyleLength::Pixel(2.f),
		StyleLength::Pixel(4.f),
		StyleLength::Pixel(2.f),
		StyleLength::Pixel(4.f + indent),
	};
	m_Header->MergeStyle(hp);

	for (const auto &child : m_Children->GetChildren()) {
		if (auto *childNode = dynamic_cast<TreeNode *>(child.get())) {
			childNode->UpdateDepth(m_Depth + 1);
		}
	}
}

void TreeNode::SetLabel(std::string label) {
	m_Label = std::move(label);
	UpdateHeaderText();
}

void TreeNode::SetExpanded(bool expanded) {
	if (expanded == m_Expanded) {
		return;
	}
	m_Expanded = expanded;
	ApplyState();
	UpdateHeaderText();
	QueueRedraw();
}

void TreeNode::SetSelected(bool selected) {
	if (selected) {
		m_Header->AddClass("tree-node-selected");
	} else {
		m_Header->RemoveClass("tree-node-selected");
	}
}

void TreeNode::ApplyState() {
	StyleProperties p;
	p.display = m_Expanded ? Display::Flex : Display::None;
	m_Children->MergeStyle(p);
}

void TreeNode::OnHeaderClicked() {
	SetExpanded(!m_Expanded);
	m_Owner.NotifySelected(this);
}

void TreeNode::OnHeaderRightClicked(vec2 pos) {
	m_Owner.NotifyRightClicked(this, pos);
}

void TreeNode::UpdateHeaderText() {
	if (!m_Header || !m_Children) {
		return;
	}
	if (m_Children->GetChildren().empty()) {
		m_Header->SetText(m_Label);
	} else {
		m_Header->SetText(std::string(m_Expanded ? "> " : "v ") + m_Label);
	}
}

} // namespace Aquila::UI::Core
