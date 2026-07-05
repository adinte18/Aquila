#include "Aquila/UI/Core/InputRouter.h"
#include "Aquila/UI/Core/Canvas.h"
#include "Aquila/UI/Core/DrawCompositor.h"
#include "Aquila/Application/Events/InputEvent.h"
#include "Aquila/Foundation/Math/Math.h"
#include "Aquila/Platform/Input.h"

namespace Aquila::UI::Core {

using namespace Application::Events;

void InputRouter::SetFocus(View *view) {
	if (m_FocusedView == view) {
		return;
	}
	if (m_FocusedView) {
		m_FocusedView->OnFocusLost();
	}
	m_FocusedView = view;
	if (m_FocusedView) {
		m_FocusedView->OnFocusGained();
	}
}

void InputRouter::OnViewRemoved(View *view) {
	if (m_HoveredView == view) {
		m_HoveredView = nullptr;
	}
	if (m_FocusedView == view) {
		m_FocusedView = nullptr;
	}
	if (m_DragTarget == view) {
		m_DragTarget = nullptr;
	}
	if (m_DragSourceCandidate == view) {
		m_DragSourceCandidate = nullptr;
		m_DragState.payload.reset();
		m_DragState.isDragging = false;
	}
}

void InputRouter::OnEvent(Application::Events::Event &e) {
	EventDispatcher dispatcher(e);

	dispatcher.Dispatch<MouseMovedEvent>([this](MouseMovedEvent &e) {
		m_MousePos = { e.GetX(), e.GetY() };

		if (Platform::Input::IsMouseButtonPressed(MouseButton::Left) && !m_DragState.isDragging) {
			vec2 delta = m_MousePos - m_DragStartPos;
			if (Math::Length(delta) > 5.f && m_HoveredView) {
				m_DragSourceCandidate = m_HoveredView->GetFirstDraggableParent();
				if (m_DragSourceCandidate != nullptr) {
					m_DragState.isDragging = true;
					m_DragSourceCandidate->OnDragStart(m_DragState);
					m_DragTarget = m_HoveredView->GetFirstParentThatAcceptsDrop();
					if (m_DragTarget) {
						m_DragTarget->OnDragEnter(m_DragState);
					}
					m_Canvas.MarkDirty();
				}
			}
		}

		View *hit = m_Compositor.HitTest(m_MousePos);
		if (hit != m_HoveredView) {
			if (m_HoveredView) {
				m_HoveredView->OnMouseLeave();
			}
			m_HoveredView = hit;
			if (m_HoveredView) {
				m_HoveredView->OnMouseEnter();
			}

			if (m_DragState.isDragging) {
				View *newTarget = m_HoveredView ? m_HoveredView->GetFirstParentThatAcceptsDrop() : nullptr;
				if (newTarget != m_DragTarget) {
					if (m_DragTarget) {
						m_DragTarget->OnDragLeave(m_DragState);
					}
					m_DragTarget = newTarget;
					if (m_DragTarget) {
						m_DragTarget->OnDragEnter(m_DragState);
					}
				}
			}

			m_Canvas.MarkDirty();
		}
		if (m_FocusedView && m_FocusedView->IsPressed()) {
			m_FocusedView->OnMouseMove(m_MousePos);
		}
		return false;
	});

	dispatcher.Dispatch<MouseButtonPressedEvent>([this](MouseButtonPressedEvent &e) {
		if (e.GetMouseButton() == MouseButton::Left) {
			m_MouseDown = true;
		}
		m_Canvas.DismissPopupsOutside(m_HoveredView);
		if (!m_HoveredView) {
			return false;
		}
		SetFocus(m_HoveredView);
		m_FocusedView->OnMousePress(e.GetMouseButton(), m_MousePos);

		m_DragStartPos = Platform::Input::GetMousePosition();

		return false;
	});

	dispatcher.Dispatch<MouseButtonReleasedEvent>([this](MouseButtonReleasedEvent &e) {
		if (e.GetMouseButton() == MouseButton::Left) {
			m_MouseDown = false;
		}

		if (m_DragState.isDragging) {
			if (m_HoveredView) {
				View *dropCandidate = m_HoveredView->GetFirstParentThatAcceptsDrop();

				if (dropCandidate) {
					dropCandidate->OnDrop(m_DragState);
				}
			}

			if (m_DragTarget) {
				m_DragTarget->OnDragLeave(m_DragState);
				m_DragTarget = nullptr;
			}
			m_DragState.payload.reset();
			m_DragState.isDragging = false;
			m_DragSourceCandidate = nullptr;
		}

		if (m_HoveredView) {
			m_HoveredView->OnMouseRelease(e.GetMouseButton(), m_MousePos);
		}
		if (m_FocusedView && m_FocusedView != m_HoveredView) {
			m_FocusedView->OnMouseRelease(e.GetMouseButton(), m_MousePos);
		}
		return false;
	});

	dispatcher.Dispatch<MouseScrolledEvent>([this](MouseScrolledEvent &e) {
		m_ScrollDelta += vec2(e.GetXOffset(), e.GetYOffset());
		m_Canvas.RequestLayout();
		return m_HoveredView != nullptr && !m_HoveredView->GetPassThroughScroll();
	});

	dispatcher.Dispatch<KeyPressedEvent>([this](KeyPressedEvent &e) {
		if (m_FocusedView) {
			m_FocusedView->OnKeyPress(e.GetKeyCode(), e.GetMods());
			return true; // consumed — prevents the engine layer from also acting on this key
		}
		return false;
	});

	dispatcher.Dispatch<KeyReleasedEvent>([this](KeyReleasedEvent &e) {
		if (m_FocusedView) {
			m_FocusedView->OnKeyRelease(e.GetKeyCode());
			return true;
		}
		return false;
	});

	dispatcher.Dispatch<KeyTypedEvent>([this](KeyTypedEvent &e) {
		if (m_FocusedView) {
			m_FocusedView->OnCharInput(static_cast<uint32>(e.GetKeyCode()));
			return true;
		}
		return false;
	});
}

} // namespace Aquila::UI::Core
