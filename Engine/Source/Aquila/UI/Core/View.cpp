#include "Aquila/UI/Core/View.h"
#include "Aquila/UI/Core/Canvas.h"
#include "Aquila/UI/Core/FontRegistry.h"
#include "Aquila/UI/Rendering/DrawCmd.h"
#include "Aquila/UI/Style/StyleParserHelper.h"
#include "Aquila/UI/Style/StylePropertyList.h"

namespace Aquila::UI::Core {

static Uint32 s_NextStableId = 0;

View::View() : m_stable_id(++s_NextStableId) {}

static float apply_easing(float t, UI::TransitionEasing easing) {
	switch (easing) {
	case UI::TransitionEasing::EaseIn:
		return t * t;
	case UI::TransitionEasing::EaseOut:
		return 1.F - (1.F - t) * (1.F - t);
	case UI::TransitionEasing::EaseInOut:
		return t < 0.5f ? 2.F * t * t : 1.F - (-2.F * t + 2.F) * (-2.F * t + 2.F) * 0.5f;
	case UI::TransitionEasing::Ease:
		return t * t * (3.F - 2.F * t);
	case UI::TransitionEasing::Linear:
	default:
		return t;
	}
}

void View::set_computed_style(UI::ComputedStyle style) {
	if (!m_display_style_initialized) {
		m_display_style = style;
		m_animation_from = style;
		m_display_style_initialized = true;
	} else if (m_computed_style != style) {
		m_animation_from = m_display_style;
		m_transition_timer = 0.F;
		m_is_animation_finished = false;
		if (m_canvas) {
			m_canvas->notify_animation_started(this);
		}
	}
	m_computed_style = std::move(style);
}

void View::update_animation(float dt) {
	const float duration_sec = m_computed_style.transition_duration / 1000.F;

	if (m_is_animation_finished) {
		return;
	}

	if (duration_sec <= 0.0001f) {
		m_display_style = m_computed_style;
		m_transition_timer = 0.F;
		m_is_animation_finished = true;
		return;
	}

	m_transition_timer = std::min(m_transition_timer + dt, duration_sec);
	const float raw = m_transition_timer / duration_sec;
	const float alpha = apply_easing(raw, m_computed_style.transition_easing);

	m_display_style = m_computed_style;

#define AQ_STYLE_PROP(css, sp, cs, layout, anim, inherit) \
	AQ_STYLE_WHEN(anim, m_display_style.cs = glm::mix(m_animation_from.cs, m_computed_style.cs, alpha);)
	AQ_STYLE_PROPERTY_LIST
#undef AQ_STYLE_PROP

	if (m_transition_timer >= duration_sec) {
		m_is_animation_finished = true;
	}
}

void View::apply_xml_attribute(std::string_view name, std::string_view value, void * /*loaderCtx*/) {
	StyleProperties props;
	UI::ParserHelper::apply_declaration(props, name, value);
	merge_style(props);
}

void View::on_style_resolved() {
	const std::string &family = get_computed_style().font_family;
	m_resolved_font = family.empty() ? nullptr : FontRegistry::resolve(family);
}

const Rect &View::get_subtree_bounds() {
	if (!m_subtree_bounds_dirty) {
		return m_subtree_bounds;
	}
	m_subtree_bounds = get_absolute_rect();
	for (const auto &child : m_children) {
		if (child->get_display_style().display != Display::None && child->is_visible()) {
			m_subtree_bounds = m_subtree_bounds.Union(child->get_subtree_bounds());
		}
	}
	m_subtree_bounds_dirty = false;
	return m_subtree_bounds;
}

void View::set_enabled(bool enabled) {
	if (m_enabled == enabled) {
		return;
	}
	m_enabled = enabled;
	if (m_canvas) {
		m_canvas->notify_style_dirty(this);
	}
}

void View::request_focus() {
	if (m_canvas) {
		m_canvas->notify_focus_request(this);
	}
}

void View::merge_style(const StyleProperties &o) {
	StyleProperties &s = m_style;
#define AQ_STYLE_PROP(css, sp, cs, layout, anim, inherit) \
	if (o.sp) {                                           \
		s.sp = o.sp;                                      \
	}
	AQ_STYLE_PROPERTY_LIST
#undef AQ_STYLE_PROP

	if (o.min) {
		s.min = o.min;
	}
	if (o.max) {
		s.max = o.max;
	}
	if (o.padding_left) {
		s.padding_left = o.padding_left;
	}
	if (o.padding_right) {
		s.padding_right = o.padding_right;
	}
	if (o.padding_top) {
		s.padding_top = o.padding_top;
	}
	if (o.padding_bottom) {
		s.padding_bottom = o.padding_bottom;
	}

	if (m_canvas) {
		m_canvas->notify_style_dirty(this);
	}
}

void View::add_class(std::string cls) {
	if (std::ranges::find(m_classes, cls) != m_classes.end()) {
		return;
	}
	m_classes.push_back(std::move(cls));
	if (m_canvas) {
		m_canvas->notify_style_dirty(this);
	}
}

void View::remove_class(std::string_view cls) {
	auto it = std::ranges::find(m_classes, cls);
	if (it != m_classes.end()) {
		m_classes.erase(it);
		if (m_canvas) {
			m_canvas->notify_style_dirty(this);
		}
	}
}

void View::set_class(std::string cls, bool enabled) {
	if (enabled) {
		add_class(std::move(cls));
	} else {
		remove_class(cls);
	}
}

void View::set_hidden(bool hidden) {
	set_class("hidden", hidden);
}

View *View::add_child(Unique<View> child) {
	View *raw = child.get();
	raw->m_parent = this;
	raw->set_canvas(m_canvas);
	m_children.push_back(std::move(child));
	mark_subtree_bounds_dirty();
	raw->set_draw_dirty();
	if (m_canvas) {
		m_canvas->notify_style_dirty(raw);
		m_canvas->notify_draw_dirty(raw);
	}
	return raw;
}

void View::queue_redraw() {
	if (m_canvas) {
		m_canvas->notify_draw_dirty(this);
	}
}

void View::invalidate_layout() {
	if (m_canvas) {
		m_canvas->notify_layout_dirty(this);
	}
}

void View::set_canvas(Canvas *canvas) {
	m_canvas = canvas;
	for (const auto &child : m_children) {
		child->set_canvas(canvas);
	}
}

void View::notify_removed(View *node) {
	for (const auto &child : node->m_children) {
		notify_removed(child.get());
	}
	if (node->m_canvas) {
		node->m_canvas->notify_view_removed(node);
	}
	node->m_canvas = nullptr;
}

void View::remove_child(View *child) {
	auto iterator = std::ranges::find_if(m_children, [child](const Unique<View> &view) { return view.get() == child; });
	if (iterator != m_children.end()) {
		notify_removed(child);
		m_children.erase(iterator);
		mark_subtree_bounds_dirty();
	}
}

Unique<View> View::detach_child(View *child) {
	auto it = std::ranges::find_if(m_children, [child](const Unique<View> &view) { return view.get() == child; });
	if (it == m_children.end()) {
		return nullptr;
	}
	notify_removed(child);
	child->m_parent = nullptr;
	auto owned = std::move(*it);
	m_children.erase(it);
	mark_subtree_bounds_dirty();
	return owned;
}

View *View::replace_child(View *old, Unique<View> new_child) {
	auto it = std::ranges::find_if(m_children, [old](const Unique<View> &v) { return v.get() == old; });
	if (it == m_children.end()) {
		return nullptr;
	}
	notify_removed(old);
	*it = std::move(new_child);
	View *raw = it->get();
	raw->m_parent = this;
	raw->set_canvas(m_canvas);
	raw->set_draw_dirty();
	if (m_canvas) {
		m_canvas->notify_style_dirty(raw);
		m_canvas->notify_draw_dirty(raw);
	}
	return raw;
}

View *View::get_parent() const {
	return m_parent;
}

View *View::get_first_draggable_parent() const {
	auto cursor = const_cast<View *>(this);
	while (cursor != nullptr && !cursor->is_draggable()) {
		cursor = cursor->get_parent();
	}

	return cursor;
}

View *View::get_first_parent_that_accepts_drop() const {
	auto cursor = const_cast<View *>(this);
	while (cursor != nullptr && !cursor->is_accepting_payload()) {
		cursor = cursor->get_parent();
	}

	return cursor;
}

const std::vector<Unique<View>> &View::get_children() const {
	return m_children;
}

View *View::find_by_id(std::string_view id) {
	if (m_id == id) {
		return this;
	}

	for (const auto &child : m_children) {
		if (View *found = child->find_by_id(id)) {
			return found;
		}
	}

	return nullptr;
}

void View::on_mouse_enter() {
	if (m_is_hovered) {
		return;
	}
	m_is_hovered = true;
	if (m_canvas) {
		m_canvas->notify_style_dirty(this);
	}
}
void View::on_mouse_leave() {
	if (!m_is_hovered) {
		return;
	}
	m_is_hovered = false;
	if (m_canvas) {
		m_canvas->notify_style_dirty(this);
	}
}
void View::on_mouse_press(Platform::MouseButton btn, Vec2 pos) {
	if (btn == Platform::MouseButton::Left) {
		m_is_pressed = true;
		if (m_canvas) {
			m_canvas->notify_style_dirty(this);
		}
	}
	if (btn == Platform::MouseButton::Right) {
		on_context_menu(pos);
	}
}
void View::on_mouse_release(Platform::MouseButton btn, Vec2) {
	if (btn == Platform::MouseButton::Left) {
		m_is_pressed = false;
		if (m_canvas) {
			m_canvas->notify_style_dirty(this);
		}
	}
}
void View::on_focus_gained() {
	m_is_focused = true;
	if (m_canvas) {
		m_canvas->notify_style_dirty(this);
	}
}
void View::on_focus_lost() {
	m_is_focused = false;
	if (m_canvas) {
		m_canvas->notify_style_dirty(this);
	}
}

void View::on_drag_start(DragState &state) {}
void View::on_drop(DragState &state) {}
void View::on_drag_enter(DragState &state) {}
void View::on_drag_leave(DragState &state) {}

View *View::hit_test_absolute(Vec2 canvas_pos) {
	if (m_computed_style.display == Display::None || !m_visible) {
		return nullptr;
	}

	if (m_is_input_leaf) {
		const Rect r = get_absolute_rect();
		return r.contains(canvas_pos) ? this : nullptr;
	}

	for (int i = static_cast<int>(m_children.size()) - 1; i >= 0; --i) {
		if (View *hit = m_children[i]->hit_test_absolute(canvas_pos)) {
			return hit;
		}
	}

	const Rect r = get_absolute_rect();
	return r.contains(canvas_pos) ? this : nullptr;
}

void View::on_draw_self(Rendering::DrawList &draw_list) {
	if (!m_visible || m_computed_style.display == Display::None) {
		return;
	}

	const Rect world_rect = { .position = m_absolute_position, .size = m_layout_rect.size };
	const Int32 z = 0;

	for (const auto &shadow : m_display_style.box_shadows) {
		draw_list.draw_shadow(world_rect, shadow.offset, shadow.blur, shadow.spread, shadow.color,
							  m_display_style.border_radius, z + 0);
	}

	if (m_display_style.background_color.a > 0.F || m_display_style.border_width > 0.F) {
		draw_list.draw_rect(world_rect, m_display_style.background_color, m_display_style.border_radius,
							m_display_style.border_width, m_display_style.border_color, z + 1,
							m_display_style.border_style);
	}
}

} // namespace Aquila::UI::Core
