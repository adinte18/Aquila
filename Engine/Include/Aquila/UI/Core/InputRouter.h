#pragma once

#include "Aquila/UI/Core/DragState.h"
#include "Aquila/UI/Core/View.h"

namespace Aquila::Application::Events {
class Event;
}

namespace Aquila::UI::Core {

class Canvas;
class DrawCompositor;

class InputRouter {
  public:
	InputRouter(Canvas &canvas, DrawCompositor &compositor) : m_canvas(canvas), m_compositor(compositor) {}

	void on_event(Application::Events::Event &event);
	void set_focus(View *view); // handles OnFocusLost/OnFocusGained bookkeeping
	void on_view_removed(View *view);

	[[nodiscard]] Vec2 mouse_pos() const { return m_mouse_pos; }
	[[nodiscard]] bool mouse_down() const { return m_mouse_down; }
	[[nodiscard]] View *hovered_view() const { return m_hovered_view; }
	[[nodiscard]] View *focused_view() const { return m_focused_view; }

	Vec2 take_scroll_delta() {
		const Vec2 delta = m_scroll_delta;
		m_scroll_delta = {};
		return delta;
	}

  private:
	Canvas &m_canvas;
	DrawCompositor &m_compositor;

	View *m_hovered_view = nullptr;
	View *m_focused_view = nullptr;
	View *m_drag_source_candidate = nullptr;
	View *m_drag_target = nullptr;
	DragState m_drag_state{};

	Vec2 m_mouse_pos = {};
	Vec2 m_drag_start_pos = {};
	bool m_mouse_down = false;
	Vec2 m_scroll_delta = {};
};

} // namespace Aquila::UI::Core
