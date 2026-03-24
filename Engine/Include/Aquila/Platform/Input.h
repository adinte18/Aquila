#ifndef AQUILA_INPUT_H
#define AQUILA_INPUT_H

#include "Aquila/Foundation/SharedConstants.h"
#include "Aquila/Application/Events/InputEvent.h"

namespace Aquila::Platform {

using namespace Aquila::Application::Events;

class Input {
  public:
	Input() = default;

	[[nodiscard]] bool IsKeyPressed(KeyCode key) const;
	[[nodiscard]] bool IsMouseButtonPressed(MouseButton button) const;
	[[nodiscard]] std::pair<f32, f32> GetMousePosition() const;

	void OnEvent(Event &eventvent);

	void Update();

  private:
	bool OnKeyPressed(KeyPressedEvent &event);
	bool OnKeyReleased(KeyReleasedEvent &event);
	bool OnMouseButtonPressed(MouseButtonPressedEvent &event);
	bool OnMouseButtonReleased(MouseButtonReleasedEvent &event);
	bool OnMouseMoved(MouseMovedEvent &event);

	std::array<bool, SharedConstants::MAX_KEY_STATES> m_KeyStates{ false };
	std::array<bool, SharedConstants::MAX_MOUSE_STATES> m_MouseButtonStates{ false };
	f32 m_MouseX = 0.0F;
	f32 m_MouseY = 0.0F;
};

AQUILA_FORCE_INLINE bool Input::IsKeyPressed(KeyCode key) const {
	return m_KeyStates.at(static_cast<usize>(key));
}

AQUILA_FORCE_INLINE bool Input::IsMouseButtonPressed(MouseButton button) const {
	return m_MouseButtonStates.at(static_cast<usize>(button));
}

AQUILA_FORCE_INLINE std::pair<f32, f32> Input::GetMousePosition() const {
	return { m_MouseX, m_MouseY };
}

AQUILA_FORCE_INLINE void Input::OnEvent(Event &event) {
	EventDispatcher dispatcher(event);
	dispatcher.Dispatch<KeyPressedEvent>([this](KeyPressedEvent &event) { return OnKeyPressed(event); });
	dispatcher.Dispatch<KeyReleasedEvent>([this](KeyReleasedEvent &event) { return OnKeyReleased(event); });
	dispatcher.Dispatch<MouseButtonPressedEvent>(
		[this](MouseButtonPressedEvent &event) { return OnMouseButtonPressed(event); });
	dispatcher.Dispatch<MouseButtonReleasedEvent>(
		[this](MouseButtonReleasedEvent &event) { return OnMouseButtonReleased(event); });
	dispatcher.Dispatch<MouseMovedEvent>([this](MouseMovedEvent &event) { return OnMouseMoved(event); });
}

AQUILA_FORCE_INLINE void Input::Update() {}

AQUILA_FORCE_INLINE bool Input::OnKeyPressed(KeyPressedEvent &event) {
	m_KeyStates.at(static_cast<usize>(event.GetKeyCode())) = true;
	return false;
}

AQUILA_FORCE_INLINE bool Input::OnKeyReleased(KeyReleasedEvent &event) {
	m_KeyStates.at(static_cast<usize>(event.GetKeyCode())) = false;
	return false;
}

AQUILA_FORCE_INLINE bool Input::OnMouseButtonPressed(MouseButtonPressedEvent &event) {
	m_MouseButtonStates.at(static_cast<usize>(event.GetMouseButton())) = true;
	return false;
}

AQUILA_FORCE_INLINE bool Input::OnMouseButtonReleased(MouseButtonReleasedEvent &event) {
	m_MouseButtonStates.at(static_cast<usize>(event.GetMouseButton())) = false;
	return false;
}

AQUILA_FORCE_INLINE bool Input::OnMouseMoved(MouseMovedEvent &event) {
	m_MouseX = event.GetX();
	m_MouseY = event.GetY();
	return false;
}

} // namespace Aquila::Platform

#endif
