#include "Aquila/UI/Widgets/FloatingOverlay.h"
#include "Aquila/UI/Core/Canvas.h"

namespace Aquila::UI::Core {

FloatingOverlay::FloatingOverlay(int16_t /*backdropZTier*/) {
	set_hidden(true);
}

void FloatingOverlay::open() {
	if (m_open) {
		return;
	}
	m_open = true;
	apply_display_state();
	if (m_dismiss_on_click_away) {
		if (Canvas *canvas = get_canvas()) {
			canvas->register_popup(this, [this] { close(); });
		}
	}
}

void FloatingOverlay::close() {
	if (!m_open) {
		return;
	}
	m_open = false;
	apply_display_state();
	if (Canvas *canvas = get_canvas()) {
		canvas->unregister_popup(this);
	}
}

void FloatingOverlay::toggle() {
	m_open ? close() : open();
}

void FloatingOverlay::set_dismiss_on_click_away(bool v) {
	m_dismiss_on_click_away = v;
	if (!m_open) {
		return;
	}
	if (Canvas *canvas = get_canvas()) {
		if (v) {
			canvas->register_popup(this, [this] { close(); });
		} else {
			canvas->unregister_popup(this);
		}
	}
}

void FloatingOverlay::apply_display_state() {
	set_hidden(!m_open);
}

View *FloatingOverlay::hit_test_absolute(Vec2 canvas_pos) {
	if (get_computed_style().display == Display::None) {
		return nullptr;
	}

	for (int i = static_cast<int>(get_children().size()) - 1; i >= 0; --i) {
		if (View *hit = get_children()[i]->hit_test_absolute(canvas_pos)) {
			return hit;
		}
	}

	const Rect r = get_absolute_rect();
	if (r.contains(canvas_pos)) {
		return this;
	}
	return nullptr;
}

} // namespace Aquila::UI::Core
