#include "Aquila/UI/Widgets/DockTabButton.h"

namespace Aquila::UI::Core {

void DockTabButton::SetDragInfo(DockDragContext *ctx, DockPanel *panel, DockNode *node) {
	m_DragCtx = ctx;
	m_Panel = panel;
	m_Node = node;
}

void DockTabButton::OnMousePress(Platform::MouseButton btn, vec2 pos) {
	Button::OnMousePress(btn, pos);
	if (btn == Platform::MouseButton::Left) {
		m_PressPos = pos;
		m_Dragging = false;
	}
}

void DockTabButton::OnMouseMove(vec2 pos) {
	if (!m_IsPressed || !m_DragCtx) {
		return;
	}

	if (!m_Dragging) {
		if (glm::length(pos - m_PressPos) < kDragThreshold) {
			return;
		}
		m_Dragging = true;
		m_DragCtx->active = true;
		m_DragCtx->panel = m_Panel;
		m_DragCtx->sourceNode = m_Node;
	}

	if (m_DragCtx->onMove) {
		m_DragCtx->onMove(pos);
	}
}

void DockTabButton::OnMouseRelease(Platform::MouseButton btn, vec2 pos) {
	if (btn == Platform::MouseButton::Left && m_Dragging) {
		View::OnMouseRelease(btn, pos);
		m_Dragging = false;
		// Copy pointer to local: onRelease may destroy `this` via DetachPanel.
		// Do not access any member after this call.
		auto *ctx = m_DragCtx;
		if (ctx && ctx->onRelease) {
			ctx->onRelease(pos);
		}
	} else {
		Button::OnMouseRelease(btn, pos);
	}
}

} // namespace Aquila::UI::Core
