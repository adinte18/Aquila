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
	InputRouter(Canvas &canvas, DrawCompositor &compositor) : m_Canvas(canvas), m_Compositor(compositor) {}

	void OnEvent(Application::Events::Event &event);
	void SetFocus(View *view); // handles OnFocusLost/OnFocusGained bookkeeping
	void OnViewRemoved(View *view);

	[[nodiscard]] vec2 MousePos() const { return m_MousePos; }
	[[nodiscard]] bool MouseDown() const { return m_MouseDown; }

	vec2 TakeScrollDelta() {
		const vec2 delta = m_ScrollDelta;
		m_ScrollDelta = {};
		return delta;
	}

  private:
	Canvas &m_Canvas;
	DrawCompositor &m_Compositor;

	View *m_HoveredView = nullptr;
	View *m_FocusedView = nullptr;
	View *m_DragSourceCandidate = nullptr;
	View *m_DragTarget = nullptr;
	DragState m_DragState{};

	vec2 m_MousePos = {};
	vec2 m_DragStartPos = {};
	bool m_MouseDown = false;
	vec2 m_ScrollDelta = {};
};

} // namespace Aquila::UI::Core
