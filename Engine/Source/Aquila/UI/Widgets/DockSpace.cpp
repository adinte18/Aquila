#include "Aquila/UI/Widgets/DockSpace.h"

#include "Aquila/Foundation/Macros.h"
#include "Aquila/Foundation/Math/Rect.h"
#include "Aquila/UI/Core/Canvas.h"
#include "Aquila/UI/Style/StyleTypes.h"
#include "Aquila/UI/Widgets/DockNode.h"
#include "Aquila/UI/Widgets/DockPanel.h"
#include "Aquila/UI/Widgets/DockSplitter.h"

namespace Aquila::UI::Core {

DockSpace::DockSpace() {
	add_class("dock-space");

	auto preview = create_unique<View>();
	preview->add_class("dock-drop-preview");
	{
		FloatingConfig cfg;
		cfg.attach_to = FloatingAttachTo::Root;
		cfg.element_point = FloatingAttachPoint::LeftTop;
		cfg.parent_point = FloatingAttachPoint::LeftTop;
		cfg.z_index = 30;
		preview->set_floating(cfg);
	}
	preview->set_hidden(true);
	m_drop_preview = add_child(std::move(preview));

	m_drag_ctx.on_node_emptied = [this](DockNode *node) {
		collapse_node(node);
		if (!has_any_panels() && m_on_emptied) {
			m_on_emptied();
		}
	};

	m_drag_ctx.on_move = [this](Vec2 pos) {
		if (is_outside_canvas(pos)) {
			if (m_drop_target) {
				m_drop_target->show_drop_zones(false);
				m_drop_target->highlight_drop_zone(DropZone::None);
				m_drop_target = nullptr;
			}
			update_preview(nullptr, DropZone::None);
			m_current_zone = DropZone::None;
			m_drag_left_canvas = true;
			if (m_on_external_drag_move) {
				m_on_external_drag_move(pos);
			}
			return;
		}
		if (m_drag_left_canvas) {
			m_drag_left_canvas = false;
			if (m_on_external_drag_clear) {
				m_on_external_drag_clear();
			}
		}

		DockNode *hovered = m_root->hit_test_node(pos);
		bool is_self_drag = (hovered != nullptr && hovered == m_drag_ctx.source_node);

		if (is_self_drag) {
			View *bar = m_drag_ctx.source_node ? m_drag_ctx.source_node->get_tab_bar() : nullptr;
			const bool over_bar = bar && bar->get_absolute_rect().contains(pos);
			if (!over_bar || m_drag_ctx.source_node->get_tab_count() < 2) {
				hovered = nullptr;
				is_self_drag = false;
			}
		}

		if (hovered != m_drop_target) {
			if (m_drop_target) {
				m_drop_target->show_drop_zones(false);
				m_drop_target->highlight_drop_zone(DropZone::None);
			}
			update_preview(nullptr, DropZone::None);
			m_drop_target = hovered;
			if (m_drop_target && !is_self_drag) {
				m_drop_target->show_drop_zones(true);
			}
			m_current_zone = DropZone::None;
		}

		if (!m_drop_target) {
			return;
		}

		if (is_self_drag) {
			// Reorder mode — no zone indicators, zone locked to Center.
			m_current_zone = DropZone::Center;
		} else {
			DropZone zone = m_drop_target->hit_test_drop_zone(pos);
			if (zone != m_current_zone) {
				m_drop_target->highlight_drop_zone(zone);
				m_current_zone = zone;
				update_preview(m_drop_target, zone);
			}
		}
	};

	m_drag_ctx.on_release = [this](Vec2 pos) {
		if (m_drag_left_canvas) {
			m_drag_left_canvas = false;
			if (m_on_external_drag_clear) {
				m_on_external_drag_clear();
			}
		}
		const bool has_valid_drop = (m_drop_target && m_drag_ctx.active && m_current_zone != DropZone::None);

		if (m_drop_target) {
			m_drop_target->show_drop_zones(false);
			m_drop_target->highlight_drop_zone(DropZone::None);
		}
		update_preview(nullptr, DropZone::None);

		if (has_valid_drop) {
			execute_drop(m_drop_target, m_current_zone, pos);
		} else if (m_drag_ctx.active && m_drag_ctx.panel && m_drag_ctx.source_node && m_on_tear_off) {
			// No dock zone under the cursor. If the tab was pulled off its bar, tear it off — the
			View *bar = m_drag_ctx.source_node->get_tab_bar();
			const bool left_bar = !bar || !bar->get_absolute_rect().contains(pos);
			if (left_bar) {
				DockNode *source = m_drag_ctx.source_node;
				DockPanel *panel = m_drag_ctx.panel;
				std::string title(panel->get_title());
				auto panel_view = source->detach_panel(panel);
				if (panel_view) {
					if (source->is_empty()) {
						collapse_node(source);
					}
					m_on_tear_off(std::move(panel_view), std::move(title), pos);
				}
			}
		}

		m_drop_target = nullptr;
		m_current_zone = DropZone::None;
		m_drag_ctx.active = false;
		m_drag_ctx.panel = nullptr;
		m_drag_ctx.source_node = nullptr;
	};

	auto root = create_unique<DockNode>(&m_drag_ctx);
	m_root = static_cast<DockNode *>(add_child(std::move(root)));
}

namespace {

std::vector<View *> declared_dock_children(View *node) {
	std::vector<View *> out;
	for (auto &child : node->get_children()) {
		View *c = child.get();
		if (dynamic_cast<DockNode *>(c) != nullptr || dynamic_cast<DockPanel *>(c) != nullptr) {
			out.push_back(c);
		}
	}
	return out;
}

} // namespace

void DockSpace::on_xml_loaded() {
	DockNode *decl_root = nullptr;
	for (auto &child : get_children()) {
		auto *dn = dynamic_cast<DockNode *>(child.get());
		if (dn != nullptr && dn != m_root) {
			decl_root = dn;
			break;
		}
	}
	if (decl_root == nullptr) {
		return; // no declarative layout — keep the default empty root
	}

	compile_declaration(m_root, decl_root);
	remove_child(decl_root);
}

void DockSpace::compile_declaration(DockNode *real_node, DockNode *decl_node) {
	const Option<SplitDirection> split = decl_node->get_declared_split();
	const std::vector<View *> slots = declared_dock_children(decl_node);

	if (!split.has_value()) {
		for (View *slot : slots) {
			if (auto *panel = dynamic_cast<DockPanel *>(slot)) {
				realize_panel(real_node, panel);
			}
		}
		return;
	}

	if (slots.empty()) {
		return;
	}
	if (slots.size() == 1) {
		realize_slot(real_node, slots[0]);
		return;
	}

	std::vector<DockNode *> leaves;
	auto [first, second] = real_node->split(*split);
	leaves.push_back(first);
	leaves.push_back(second);
	for (size_t i = 2; i < slots.size(); ++i) {
		DockNode *leaf = real_node->append_leaf(*split);
		if (leaf == nullptr) {
			break;
		}
		leaves.push_back(leaf);
	}

	for (size_t i = 0; i < leaves.size(); ++i) {
		realize_slot(leaves[i], slots[i]);
	}
}

void DockSpace::realize_slot(DockNode *real_leaf, View *slot) {
	if (auto *child_node = dynamic_cast<DockNode *>(slot)) {
		compile_declaration(real_leaf, child_node);
	} else if (auto *panel = dynamic_cast<DockPanel *>(slot)) {
		realize_panel(real_leaf, panel);
	}
}

void DockSpace::realize_panel(DockNode *real_leaf, DockPanel *decl_panel) {
	DockPanel *real_panel = real_leaf->add_panel(decl_panel->get_title(), decl_panel->get_tab_icon());
	real_panel->set_id(decl_panel->get_id());
	for (const auto &cls : decl_panel->get_classes()) {
		real_panel->add_class(cls);
	}

	std::vector<View *> content;
	for (const auto &child : decl_panel->get_children()) {
		content.push_back(child.get());
	}
	for (View *child : content) {
		real_panel->add_child(decl_panel->detach_child(child));
	}
}

static DockNode *find_leaf_with_tabs(View *view) {
	if (auto *node = dynamic_cast<DockNode *>(view)) {
		if (node->get_tab_count() > 0) {
			return node;
		}
	}
	for (auto &child : view->get_children()) {
		if (DockNode *found = find_leaf_with_tabs(child.get())) {
			return found;
		}
	}
	return nullptr;
}

bool DockSpace::has_any_panels() const {
	return m_root && find_leaf_with_tabs(m_root) != nullptr;
}

DockNode *DockSpace::first_leaf_with_tabs() const {
	return m_root ? find_leaf_with_tabs(m_root) : nullptr;
}

void DockSpace::update_preview(DockNode *target, DropZone zone) {
	if (!m_drop_preview) {
		return;
	}

	if (!target || zone == DropZone::None) {
		m_drop_preview->set_hidden(true);
		return;
	}

	Rect r = target->get_absolute_rect();
	Rect preview = r;

	switch (zone) {
	case DropZone::Center:

		break;
	case DropZone::Left:
		preview.size.x *= 0.5f;
		break;
	case DropZone::Right:
		preview.position.x += r.size.x * 0.5f;
		preview.size.x *= 0.5f;
		break;
	case DropZone::Top:
		preview.size.y *= 0.5f;
		break;
	case DropZone::Bottom:
		preview.position.y += r.size.y * 0.5f;
		preview.size.y *= 0.5f;
		break;
	default:
		break;
	}

	FloatingConfig cfg;
	cfg.attach_to = FloatingAttachTo::Root;
	cfg.element_point = FloatingAttachPoint::LeftTop;
	cfg.parent_point = FloatingAttachPoint::LeftTop;
	cfg.offset = preview.position;
	cfg.z_index = 30;
	m_drop_preview->set_floating(cfg);
	m_drop_preview->invalidate_layout();

	StyleProperties sp;
	sp.width = StyleLength::pixel(preview.size.x);
	sp.height = StyleLength::pixel(preview.size.y);
	m_drop_preview->merge_style(sp);
	m_drop_preview->set_hidden(false);
}

void DockSpace::execute_drop(DockNode *target, DropZone zone, Vec2 release_pos) {
	if (!target) {
		return;
	}

	DockNode *source = m_drag_ctx.source_node;

	// External drag: no source node — the panel subtree travels in the drag context.
	if (!source) {
		if (!m_drag_ctx.external_view) {
			return;
		}
		Unique<View> panel_view = std::move(m_drag_ctx.external_view);
		target->accept_panel(std::move(panel_view), m_drag_ctx.title, zone);
		return;
	}

	DockPanel *panel = m_drag_ctx.panel;
	if (!panel) {
		return;
	}

	const bool self_drop = (target == source);

	if (self_drop && zone == DropZone::Center) {
		source->reorder_panel(panel, release_pos);
		return;
	}

	if (self_drop && source->get_tab_count() < 2) {
		return;
	}

	std::string title = std::string(panel->get_title());

	auto panel_view = source->detach_panel(panel);
	if (!panel_view) {
		return;
	}

	if (!self_drop && source->is_empty()) {
		collapse_node(source);
	}

	target->accept_panel(std::move(panel_view), std::move(title), zone);
}

void DockSpace::collapse_node(DockNode *node) {
	if (node == m_root) {
		return;
	}

	auto *parent = dynamic_cast<DockNode *>(node->get_parent());
	if (!parent) {
		return;
	}

	DockSplitter *left_split = nullptr;
	DockSplitter *right_split = nullptr;
	for (auto &child : parent->get_children()) {
		auto *ds = dynamic_cast<DockSplitter *>(child.get());
		if (!ds) {
			continue;
		}
		if (ds->get_after() == node) {
			left_split = ds;
		}
		if (ds->get_before() == node) {
			right_split = ds;
		}
	}

	DockNode *survivor = nullptr;
	if (left_split && right_split) {
		left_split->set_siblings(left_split->get_before(), right_split->get_after());
		parent->remove_child(right_split);
	} else if (left_split) {
		survivor = dynamic_cast<DockNode *>(left_split->get_before());
		parent->remove_child(left_split);
	} else if (right_split) {
		survivor = dynamic_cast<DockNode *>(right_split->get_after());
		parent->remove_child(right_split);
	}

	parent->remove_child(node);

	if (survivor) {
		StyleProperties sp;
		sp.width = StyleLength::grow();
		sp.height = StyleLength::grow();
		sp.flex_grow = 1.F;
		survivor->merge_style(sp);
	}

	// A container reduced to a single node is redundant — hoist that node into the container's slot
	DockNode *only_child = nullptr;
	int node_count = 0;
	for (auto &child : parent->get_children()) {
		if (auto *dn = dynamic_cast<DockNode *>(child.get())) {
			++node_count;
			only_child = dn;
		}
	}
	if (node_count == 1 && only_child) {
		hoist_single_child(parent, only_child);
	} else {
		parent->invalidate_layout();
	}
}

void DockSpace::hoist_single_child(DockNode *container, DockNode *only) {
	View *grandparent = container->get_parent();
	Unique<View> owned = container->detach_child(only);
	if (!owned) {
		return;
	}

	StyleProperties sp;
	sp.width = StyleLength::grow();
	sp.height = StyleLength::grow();
	sp.flex_grow = 1.F;
	only->merge_style(sp);

	if (container == m_root) {
		replace_child(container, std::move(owned));
		m_root = only;
	} else if (auto *gp_node = dynamic_cast<DockNode *>(grandparent)) {
		for (auto &child : gp_node->get_children()) {
			if (auto *ds = dynamic_cast<DockSplitter *>(child.get())) {
				ds->update_sibling_ref(container, only);
			}
		}
		gp_node->replace_child(container, std::move(owned));
	} else if (grandparent) {
		grandparent->replace_child(container, std::move(owned));
	}

	if (grandparent) {
		grandparent->invalidate_layout();
	}
}

bool DockSpace::is_outside_canvas(Vec2 pos) const {
	const Canvas *canvas = get_canvas();
	if (!canvas) {
		return false;
	}
	const auto w = static_cast<float>(canvas->get_width());
	const auto h = static_cast<float>(canvas->get_height());
	return pos.x < 0.F || pos.y < 0.F || pos.x >= w || pos.y >= h;
}

void DockSpace::preview_external_drag(Vec2 local_pos) {
	DockNode *hovered = m_root->hit_test_node(local_pos);
	if (hovered != m_drop_target) {
		if (m_drop_target) {
			m_drop_target->show_drop_zones(false);
			m_drop_target->highlight_drop_zone(DropZone::None);
		}
		update_preview(nullptr, DropZone::None);
		m_drop_target = hovered;
		if (m_drop_target) {
			m_drop_target->show_drop_zones(true);
		}
		m_current_zone = DropZone::None;
	}

	if (!m_drop_target) {
		return;
	}

	DropZone zone = m_drop_target->hit_test_drop_zone(local_pos);
	if (zone != m_current_zone) {
		m_drop_target->highlight_drop_zone(zone);
		m_current_zone = zone;
		update_preview(m_drop_target, zone);
	}
}

void DockSpace::clear_external_drag() {
	if (m_drop_target) {
		m_drop_target->show_drop_zones(false);
		m_drop_target->highlight_drop_zone(DropZone::None);
	}
	update_preview(nullptr, DropZone::None);
	m_drop_target = nullptr;
	m_current_zone = DropZone::None;
}

void DockSpace::begin_external_drag(DockPanel *panel, const std::string &title) {
	m_drag_ctx.active = true;
	m_drag_ctx.source_node = nullptr;
	m_drag_ctx.panel = panel;
	m_drag_ctx.title = title;
	m_drag_ctx.external_view.reset();
	m_drop_target = nullptr;
	m_current_zone = DropZone::None;
}

bool DockSpace::try_dock_external(Unique<View> &panel_view, const std::string &title, Vec2 local_pos) {
	preview_external_drag(local_pos);

	DockNode *target = m_drop_target;
	DropZone zone = m_current_zone;

	const bool can_dock = (target != nullptr && zone != DropZone::None);
	if (can_dock) {
		// resolves it — no direct AcceptPanel from the external path.
		if (!title.empty()) {
			m_drag_ctx.title = title;
		}
		m_drag_ctx.external_view = std::move(panel_view);
		execute_drop(target, zone, local_pos);
	}

	clear_external_drag();
	m_drag_ctx.active = false;
	m_drag_ctx.source_node = nullptr;
	m_drag_ctx.panel = nullptr;
	m_drag_ctx.title.clear();
	m_drag_ctx.external_view.reset();
	return can_dock;
}

} // namespace Aquila::UI::Core
