#include "Aquila/UI/Widgets/DockTabButton.h"

namespace Aquila::UI::Core {

void DockTabButton::set_drag_info(DockDragContext *ctx, DockPanel *panel, DockNode *node) {
	m_drag_ctx = ctx;
	m_panel = panel;
	m_node = node;
}

void DockTabButton::on_mouse_press(Platform::MouseButton btn, Vec2 pos) {
	Button::on_mouse_press(btn, pos);
	if (btn == Platform::MouseButton::Left) {
		m_press_pos = pos;
		m_dragging = false;
	}
}

void DockTabButton::on_mouse_move(Vec2 pos) {
	if (!m_is_pressed || (m_drag_ctx == nullptr)) {
		return;
	}

	if (!m_dragging) {
		if (glm::length(pos - m_press_pos) < K_DRAG_THRESHOLD) {
			return;
		}
		m_dragging = true;
		m_drag_ctx->active = true;
		m_drag_ctx->panel = m_panel;
		m_drag_ctx->source_node = m_node;
	}

	if (m_drag_ctx->on_move) {
		m_drag_ctx->on_move(pos);
	}
}

void DockTabButton::on_mouse_release(Platform::MouseButton btn, Vec2 pos) {
	if (btn == Platform::MouseButton::Left && m_dragging) {
		View::on_mouse_release(btn, pos);
		m_dragging = false;
		// Copy pointer to local: onRelease may destroy `this` via DetachPanel.
		// Do not access any member after this call.
		auto *ctx = m_drag_ctx;
		if ((ctx != nullptr) && ctx->on_release) {
			ctx->on_release(pos);
		}
	} else {
		Button::on_mouse_release(btn, pos);
	}
}

} // namespace Aquila::UI::Core
