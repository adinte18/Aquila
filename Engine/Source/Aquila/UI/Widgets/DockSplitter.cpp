#include "Aquila/UI/Widgets/DockSplitter.h"

namespace Aquila::UI::Core {

DockSplitter::DockSplitter(SplitDirection dir) : m_dir(dir) {
	set_input_leaf(true);
	add_class("dock-splitter");
	add_class(dir == SplitDirection::Horizontal ? "dock-splitter-h" : "dock-splitter-v");
}

void DockSplitter::set_siblings(View *before, View *after) {
	m_before = before;
	m_after = after;
}

void DockSplitter::update_sibling_ref(View *old, View *new_ptr) {
	if (m_before == old) {
		m_before = new_ptr;
	}
	if (m_after == old) {
		m_after = new_ptr;
	}
}

void DockSplitter::on_mouse_press(Platform::MouseButton btn, Vec2 pos) {
	View::on_mouse_press(btn, pos);
	if (btn != Platform::MouseButton::Left || !m_before || !m_after) {
		return;
	}
	m_drag_start_pos = pos;
	m_before_grow_start =
		(m_dir == SplitDirection::Horizontal) ? m_before->get_layout_rect().size.x : m_before->get_layout_rect().size.y;
	m_after_grow_start =
		(m_dir == SplitDirection::Horizontal) ? m_after->get_layout_rect().size.x : m_after->get_layout_rect().size.y;
}

void DockSplitter::on_mouse_release(Platform::MouseButton btn, Vec2 pos) {
	View::on_mouse_release(btn, pos);
}

void DockSplitter::on_mouse_move(Vec2 pos) {
	if (!m_is_pressed || !m_before || !m_after || !get_parent()) {
		return;
	}

	const float delta = (m_dir == SplitDirection::Horizontal) ? pos.x - m_drag_start_pos.x : pos.y - m_drag_start_pos.y;

	const float parent_px = (m_dir == SplitDirection::Horizontal) ? get_parent()->get_layout_rect().size.x
																  : get_parent()->get_layout_rect().size.y;
	if (parent_px <= 0.F) {
		return;
	}

	// Each side must stay at least kMin pixels wide/tall regardless of layout context.
	constexpr float k_min = 60.F;
	const float min_px = k_min;
	const float max_px = std::max(min_px, parent_px - 5.F - k_min);

	StyleProperties sp;
	if (m_resize_before) {
		const float new_px = std::clamp(m_before_grow_start + delta, min_px, max_px);
		const float new_pct = new_px / parent_px * 100.F;
		if (m_dir == SplitDirection::Horizontal) {
			sp.width = StyleLength::percent(new_pct);
		} else {
			sp.height = StyleLength::percent(new_pct);
		}
		m_before->merge_style(sp);
	} else {
		const float new_px = std::clamp(m_after_grow_start - delta, min_px, max_px);
		const float new_pct = new_px / parent_px * 100.F;
		if (m_dir == SplitDirection::Horizontal) {
			sp.width = StyleLength::percent(new_pct);
		} else {
			sp.height = StyleLength::percent(new_pct);
		}
		m_after->merge_style(sp);
	}

	get_parent()->invalidate_layout();
}

} // namespace Aquila::UI::Core
