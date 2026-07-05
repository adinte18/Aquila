#include "Aquila/UI/Widgets/Toggle.h"

namespace Aquila::UI::Core {

Toggle::Toggle() {
	set_input_leaf(true);
	add_class("toggle");
}

Toggle::Toggle(bool on) {
	set_input_leaf(true);
	add_class("toggle");
	set_value_without_notify(on);
}

void Toggle::on_value_updated() {
	if (get_value()) {
		add_class("toggle-on");
	} else {
		remove_class("toggle-on");
	}
	queue_redraw();
}

void Toggle::on_mouse_release(Platform::MouseButton btn, Vec2 pos) {
	if (btn == Platform::MouseButton::Left && m_is_hovered) {
		set_value(!get_value());
	}
	View::on_mouse_release(btn, pos);
}

void Toggle::on_draw_self(Rendering::DrawList &draw_list) {
	View::on_draw_self(draw_list);

	using namespace Rendering;
	const Rect rect = get_absolute_rect();
	const auto &style = get_display_style();
	const Int32 z = 0;

	const Vec4 track_color = style.effective_accent_color();
	const Vec4 thumb_color = style.color;

	// Track
	const float track_h = rect.size.y * 0.55f;
	const float track_y = rect.position.y + (rect.size.y - track_h) * 0.5f;
	const Rect track = { .position = { rect.position.x, track_y }, .size = { rect.size.x, track_h } };
	draw_list.draw_rect(track, track_color, Vec4(track_h * 0.5f), 0.F, Vec4(0.F), z + 2);

	// Thumb — drawn on top of track
	const float thumb_diam = rect.size.y - 4.F;
	const float thumb_y = rect.position.y + 2.F;
	const float thumb_x = get_value() ? rect.position.x + rect.size.x - thumb_diam - 2.F : rect.position.x + 2.F;
	const Rect thumb = { .position = { thumb_x, thumb_y }, .size = { thumb_diam, thumb_diam } };
	draw_list.draw_rect(thumb, thumb_color, Vec4(thumb_diam * 0.5f), 0.F, Vec4(0.F), z + 3);
}

} // namespace Aquila::UI::Core
