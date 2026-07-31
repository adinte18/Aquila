#include "UI/Debug/UIDebugWindow.h"

#include "Aquila/Application/Events/Event.h"
#include "Aquila/Application/Events/WindowEvent.h"
#include "Aquila/UI/Core/Canvas.h"
#include "Aquila/UI/Core/View.h"
#include "Aquila/UI/Style/ComputedStyle.h"
#include "Aquila/UI/Style/StyleParser.h"
#include "Aquila/UI/Widgets/Button.h"
#include "Aquila/UI/Widgets/Label.h"
#include "Aquila/UI/Widgets/ScrollView.h"
#include "Aquila/UI/Widgets/TreeView.h"

#include <algorithm>
#include <cstdio>

namespace Editor {

using namespace Aquila;
using namespace Aquila::UI::Core;

namespace {

using namespace Aquila::UI;

std::string make_label(View *view) {
	std::string label(view->get_type_name());
	if (!view->get_id().empty()) {
		label += " #" + view->get_id();
	}
	for (const auto &cls : view->get_classes()) {
		label += " ." + cls;
	}
	return label;
}

const char *str(Display d) {
	return d == Display::Flex ? "flex" : "none";
}
const char *str(Overflow o) {
	return o == Overflow::Visible ? "visible" : (o == Overflow::Hidden ? "hidden" : "scroll");
}
const char *str(Position p) {
	switch (p) {
	case Position::Relative:
		return "relative";
	case Position::Absolute:
		return "absolute";
	default:
		return "static";
	}
}
const char *str(FlexDirection f) {
	switch (f) {
	case FlexDirection::Column:
		return "column";
	case FlexDirection::RowReverse:
		return "row-reverse";
	case FlexDirection::ColumnReverse:
		return "column-reverse";
	default:
		return "row";
	}
}
const char *str(JustifyContent j) {
	switch (j) {
	case JustifyContent::End:
		return "end";
	case JustifyContent::Center:
		return "center";
	default:
		return "start";
	}
}
const char *str(AlignItems a) {
	switch (a) {
	case AlignItems::End:
		return "end";
	case AlignItems::Center:
		return "center";
	case AlignItems::Stretch:
		return "stretch";
	default:
		return "start";
	}
}
const char *str(TextAlign t) {
	return t == TextAlign::Center ? "center" : (t == TextAlign::Right ? "right" : "left");
}

std::string num(F32 v) {
	char buf[32];
	std::snprintf(buf, sizeof(buf), "%g", v);
	return buf;
}

std::string len_str(StyleLength l) {
	switch (l.unit) {
	case LengthUnit::Pixel:
		return num(l.value) + "px";
	case LengthUnit::Percent:
		return num(l.value) + "%";
	case LengthUnit::Grow:
		return "grow";
	default:
		return "auto";
	}
}

std::string edges_str(const StyleEdges &e) {
	return len_str(e.top) + " " + len_str(e.right) + " " + len_str(e.bottom) + " " + len_str(e.left);
}

std::string color_str(Vec4 c) {
	auto ch = [](float v) { return static_cast<int>(std::clamp(v, 0.F, 1.F) * 255.F + 0.5f); };
	char buf[40];
	std::snprintf(buf, sizeof(buf), "#%02X%02X%02X%02X", ch(c.r), ch(c.g), ch(c.b), ch(c.a));
	return buf;
}

} // namespace

UIDebugWindow::UIDebugWindow() = default;
UIDebugWindow::~UIDebugWindow() = default;

void UIDebugWindow::build(Canvas *target, Uint32 width, Uint32 height, const std::string &style_path) {
	m_target = target;
	m_canvas = std::make_unique<Canvas>(width, height);
	UI::StyleParser::load_file(style_path, m_canvas->get_style_sheet());

	auto *root = m_canvas->get_root();
	root->set_id("ui-debug-root");
	root->add_class("ui-debug-root");

	auto *header = root->add_child<View>();
	header->add_class("ui-debug-header");
	auto *title = header->add_child<Label>(std::string("UI Inspector"));
	title->add_class("ui-debug-title");
	auto *refresh = header->add_child<Button>(std::string("Refresh"));
	refresh->add_class("ui-debug-refresh");
	refresh->on_click.connect([this] { this->refresh(); });

	auto *pick = header->add_child<Button>(std::string("Pick"));
	pick->add_class("ui-debug-refresh");
	pick->on_click.connect([this] {
		if (on_pick_requested) {
			on_pick_requested();
		}
	});

	m_tree_host = root->add_child<View>();
	m_tree_host->add_class("ui-debug-tree-pane");

	auto *detail_scroll = root->add_child<ScrollView>();
	detail_scroll->add_class("ui-debug-detail-pane");
	m_details_host = detail_scroll->add_child<View>();
	m_details_host->add_class("ui-debug-details");

	m_canvas->reload_styles();
	this->refresh();
}

void UIDebugWindow::update(F32 delta_time) {
	m_canvas->update(delta_time);
	m_canvas->compute();
}

void UIDebugWindow::render(Graphics::QuadBatcher &batcher, GFX::GfxCommandList &cmd) {
	m_canvas->submit_to_quad_batcher(batcher, cmd);
}

void UIDebugWindow::on_event(Application::Events::Event &event) {
	Application::Events::EventDispatcher dispatcher(event);
	dispatcher.dispatch<Application::Events::WindowResizeEvent>([this](Application::Events::WindowResizeEvent &e) {
		if (e.get_width() > 0 && e.get_height() > 0) {
			m_canvas->resize(e.get_width(), e.get_height());
		}
		return false;
	});

	m_canvas->on_event(event);
}

TreeNode *UIDebugWindow::add_view_node(View *view, TreeNode *parent_node) {
	TreeNode *node = parent_node ? parent_node->add_child_node(make_label(view)) : m_tree->add_node(make_label(view));
	m_node_to_view[node] = view;

	for (const auto &child : view->get_children()) {
		add_view_node(child.get(), node);
	}
	return node;
}

void UIDebugWindow::refresh() {
	if (!m_target || !m_tree_host) {
		return;
	}

	m_node_to_view.clear();
	while (!m_tree_host->get_children().empty()) {
		m_tree_host->remove_child(m_tree_host->get_children().front().get());
	}

	m_tree = m_tree_host->add_child<TreeView>();
	m_tree->on_selected.connect([this](TreeNode *node) {
		auto it = m_node_to_view.find(node);
		show_details(it != m_node_to_view.end() ? it->second : nullptr);
	});

	for (const auto &child : m_target->get_root()->get_children()) {
		add_view_node(child.get(), nullptr);
	}
}

void UIDebugWindow::select_view(View *view) {
	if (!view || !m_tree) {
		show_details(view);
		return;
	}

	TreeNode *node = nullptr;
	for (auto &[n, v] : m_node_to_view) {
		if (v == view) {
			node = n;
			break;
		}
	}
	if (!node) {
		show_details(view);
		return;
	}

	for (View *parent = view->get_parent(); parent; parent = parent->get_parent()) {
		for (auto &[n, v] : m_node_to_view) {
			if (v == parent) {
				n->set_expanded(true);
				break;
			}
		}
	}
	m_tree->select_node(node);
	show_details(view);
	m_canvas->scroll_into_view(node);
}

void UIDebugWindow::show_details(View *view) {
	if (!m_details_host) {
		return;
	}

	while (!m_details_host->get_children().empty()) {
		m_details_host->remove_child(m_details_host->get_children().front().get());
	}

	if (!view) {
		auto *empty = m_details_host->add_child<Label>(std::string("Select a node to inspect"));
		empty->add_class("ui-debug-key");
		return;
	}

	auto section = [&](const std::string &name) {
		auto *s = m_details_host->add_child<Label>(name);
		s->add_class("ui-debug-section");
	};
	auto row = [&](const std::string &key, const std::string &value) {
		auto *r = m_details_host->add_child<View>();
		r->add_class("ui-debug-row");
		auto *k = r->add_child<Label>(key);
		k->add_class("ui-debug-key");
		auto *v = r->add_child<Label>(value);
		v->add_class("ui-debug-val");
	};

	section("Identity");
	row("type", std::string(view->get_type_name()));
	row("id", view->get_id().empty() ? "(none)" : view->get_id());
	std::string classes;
	for (const auto &c : view->get_classes()) {
		classes += (classes.empty() ? "" : " ") + c;
	}
	row("classes", classes.empty() ? "(none)" : classes);
	row("stableId", std::to_string(view->get_stable_id()));
	row("clayId", std::to_string(view->get_clay_id()));
	row("children", std::to_string(view->get_children().size()));
	if (View *parent = view->get_parent()) {
		row("parent", std::string(parent->get_type_name()));
	}

	const Rect rect = view->get_absolute_rect();
	section("Geometry");
	row("position", num(rect.position.x) + ", " + num(rect.position.y));
	row("size", num(rect.size.x) + " x " + num(rect.size.y));
	row("effectiveZ", std::to_string(view->get_effective_z()));

	section("State");
	row("visible", view->is_visible() ? "true" : "false");
	row("enabled", view->is_enabled() ? "true" : "false");
	row("hovered", view->is_hovered() ? "true" : "false");
	row("pressed", view->is_pressed() ? "true" : "false");
	row("focused", view->is_focused() ? "true" : "false");

	const UI::ComputedStyle &cs = view->get_computed_style();
	section("Layout");
	row("display", str(cs.display));
	row("position", str(cs.position));
	row("overflow", str(cs.overflow));
	row("flex-direction", str(cs.flex_direction));
	row("justify", str(cs.justify));
	row("align", str(cs.align));
	row("flex-grow", num(cs.flex_grow));
	row("width", len_str(cs.width));
	row("height", len_str(cs.height));
	row("min w/h", len_str(cs.min_width) + " / " + len_str(cs.min_height));
	row("max w/h", len_str(cs.max_width) + " / " + len_str(cs.max_height));
	row("padding", edges_str(cs.padding));
	row("gap", num(cs.gap));
	if (cs.aspect_ratio > 0.F) {
		row("aspect-ratio", num(cs.aspect_ratio));
	}
	row("inset", len_str(cs.top) + " " + len_str(cs.right) + " " + len_str(cs.bottom) + " " + len_str(cs.left));
	row("z-index", std::to_string(cs.z_index));

	section("Appearance");
	row("background", color_str(cs.background_color));
	row("color", color_str(cs.color));
	row("border", num(cs.border_width) + "px " + color_str(cs.border_color));
	row("border-radius", len_str(StyleLength::pixel(cs.border_radius.x)) + " (x)");
	row("opacity", num(cs.opacity));
	row("box-shadows", std::to_string(cs.box_shadows.size()));

	section("Text");
	row("font-size", num(cs.font_size));
	row("font-family", cs.font_family.empty() ? "(inherit)" : cs.font_family);
	row("text-align", str(cs.text_align));
}

} // namespace Editor
