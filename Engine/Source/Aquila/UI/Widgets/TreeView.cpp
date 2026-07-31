#include "Aquila/UI/Widgets/TreeView.h"

namespace Aquila::UI::Core {

TreeView::TreeView() {
	add_class("tree-view");

	auto content = std::make_unique<View>();
	content->add_class("tree-view-content");
	m_content = View::add_child(std::move(content));

	m_content->on_pressed.connect([this](Vec2) { deselect(); });
	on_pressed.connect([this](Vec2) { deselect(); });
}

TreeNode *TreeView::add_node(std::string label) {
	auto node = std::make_unique<TreeNode>(std::move(label), *this, 0);
	return static_cast<TreeNode *>(add_child(std::move(node)));
}

View *TreeView::add_child(Unique<View> child) {
	return m_content->View::add_child(std::move(child));
}

void TreeView::on_drag_enter(DragState &) {
	add_class("drag-target");
}

void TreeView::on_drag_leave(DragState &) {
	remove_class("drag-target");
}

static bool is_descendant_of(View *candidate, View *ancestor) {
	while (candidate != nullptr) {
		if (candidate == ancestor) {
			return true;
		}
		candidate = candidate->get_parent();
	}
	return false;
}

void TreeView::remove_node(TreeNode *node) {
	if (node == nullptr) {
		return;
	}

	if (node->get_parent() == nullptr) {
		return;
	}

	if (m_selected != nullptr && is_descendant_of(m_selected, node)) {
		m_selected = nullptr;
	}

	View *container = node->get_parent();
	container->remove_child(node);
	if (auto *owner_node = view_cast<TreeNode>(container->get_parent())) {
		owner_node->refresh_indicator();
	}
}

void TreeView::set_on_background_right_clicked(Delegate<void(Vec2)> callback) {
	m_content->on_context_menu.set(std::move(callback));
}

void TreeView::select_node(TreeNode *node) {
	if (m_selected != nullptr) {
		m_selected->set_selected(false);
	}
	m_selected = node;
	if (m_selected != nullptr) {
		m_selected->set_selected(true);
	}
}

void TreeView::deselect() {
	if (m_selected == nullptr) {
		return;
	}
	select_node(nullptr);
	on_deselected();
}

void TreeView::notify_selected(TreeNode *node) {
	select_node(node);
	on_selected(node);
}

void TreeView::notify_right_clicked(TreeNode *node, Vec2 pos) {
	on_node_right_clicked(node, pos);
}

static constexpr float K_INDENT_PER_DEPTH = 16.F;

TreeNode::TreeNode(std::string label, TreeView &owner, int depth)
	: m_owner(owner), m_label(std::move(label)), m_depth(depth) {
	add_class("tree-node");

	auto header = std::make_unique<Button>();
	{
		StyleProperties hp;
		const float indent = K_INDENT_PER_DEPTH * static_cast<float>(m_depth);
		hp.padding = StyleEdges{
			StyleLength::pixel(2.F),
			StyleLength::pixel(4.F),
			StyleLength::pixel(2.F),
			StyleLength::pixel(4.F + indent),
		};
		header->set_style(hp);
		header->add_class("tree-node-header");
	}
	header->on_click.connect([this] { on_header_clicked(); });
	header->on_context_menu.connect([this](Vec2 pos) { on_header_right_clicked(pos); });
	m_header = static_cast<Button *>(View::add_child(std::move(header)));

	auto children = std::make_unique<View>();
	children->add_class("tree-node-children");
	m_children = View::add_child(std::move(children));
	update_header_text();
}

View *TreeNode::add_child(Unique<View> node) {
	auto *added = m_children->View::add_child(std::move(node));
	update_header_text();
	queue_redraw();
	return added;
}

void TreeNode::on_drag_enter(DragState &) {
	m_header->add_class("drag-target");
}

void TreeNode::on_drag_leave(DragState &) {
	m_header->remove_class("drag-target");
}

TreeNode *TreeNode::add_child_node(std::string label) {
	auto node = std::make_unique<TreeNode>(std::move(label), m_owner, m_depth + 1);
	return static_cast<TreeNode *>(add_child(std::move(node)));
}

void TreeNode::update_depth(int new_depth) {
	m_depth = new_depth;

	const float indent = K_INDENT_PER_DEPTH * static_cast<float>(m_depth);
	StyleProperties hp;
	hp.padding = StyleEdges{
		StyleLength::pixel(2.F),
		StyleLength::pixel(4.F),
		StyleLength::pixel(2.F),
		StyleLength::pixel(4.F + indent),
	};
	m_header->merge_style(hp);

	for (const auto &child : m_children->get_children()) {
		if (auto *child_node = view_cast<TreeNode>(child.get())) {
			child_node->update_depth(m_depth + 1);
		}
	}
}

void TreeNode::set_label(std::string label) {
	m_label = std::move(label);
	update_header_text();
}

void TreeNode::set_expanded(bool expanded) {
	if (expanded == m_expanded) {
		return;
	}
	m_expanded = expanded;
	apply_state();
	update_header_text();
	queue_redraw();
}

void TreeNode::set_selected(bool selected) {
	if (selected) {
		m_header->add_class("tree-node-selected");
	} else {
		m_header->remove_class("tree-node-selected");
	}
}

void TreeNode::apply_state() {
	StyleProperties p;
	p.display = m_expanded ? Display::Flex : Display::None;
	m_children->merge_style(p);
}

void TreeNode::on_header_clicked() {
	set_expanded(!m_expanded);
	m_owner.notify_selected(this);
}

void TreeNode::on_header_right_clicked(Vec2 pos) {
	m_owner.notify_right_clicked(this, pos);
}

void TreeNode::update_header_text() {
	if (!m_header || !m_children) {
		return;
	}
	const bool has_children = !m_children->get_children().empty();
	const bool use_icons = (m_owner.m_icon_collapsed != nullptr) || (m_owner.m_icon_expanded != nullptr);

	if (use_icons) {
		m_header->set_reserve_icon_space(true);
		m_header->set_icon(has_children ? (m_expanded ? m_owner.m_icon_expanded : m_owner.m_icon_collapsed) : nullptr);
		m_header->set_text(m_label);
		return;
	}

	m_header->set_reserve_icon_space(false);
	m_header->set_icon(nullptr);
	if (has_children) {
		m_header->set_text(std::string(m_expanded ? "v " : "> ") + m_label);
	} else {
		m_header->set_text(m_label);
	}
}

void TreeNode::refresh_indicator() {
	update_header_text();
}

void TreeView::set_expand_icons(GFX::GfxTexture *collapsed, GFX::GfxTexture *expanded) {
	m_icon_collapsed = collapsed;
	m_icon_expanded = expanded;
	if (m_content != nullptr) {
		refresh_indicators(m_content);
	}
}

void TreeView::refresh_indicators(View *node) {
	for (const auto &child : node->get_children()) {
		if (auto *tree_node = view_cast<TreeNode>(child.get())) {
			tree_node->refresh_indicator();
		}
		refresh_indicators(child.get());
	}
}

} // namespace Aquila::UI::Core
