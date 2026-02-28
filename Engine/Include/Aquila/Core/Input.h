#ifndef AQUILA_INPUT_H
#define AQUILA_INPUT_H

#include "AquilaCore.h"
#include "Aquila/Events/InputEvent.h"

namespace Aquila::Core {

using namespace Aquila::Events;

class Input {
  public:
	Input() = default;

	[[nodiscard]] bool IsKeyPressed(KeyCode key) const;
	[[nodiscard]] bool IsMouseButtonPressed(MouseButton button) const;
	[[nodiscard]] std::pair<f32, f32> GetMousePosition() const;

	void OnEvent(Event &event);

	void Update();

  private:
	bool OnKeyPressed(KeyPressedEvent &e);
	bool OnKeyReleased(KeyReleasedEvent &e);
	bool OnMouseButtonPressed(MouseButtonPressedEvent &e);
	bool OnMouseButtonReleased(MouseButtonReleasedEvent &e);
	bool OnMouseMoved(MouseMovedEvent &e);

	std::array<bool, 512> m_KeyStates{ false };
	std::array<bool, 8> m_MouseButtonStates{ false };
	f32 m_MouseX = 0.0f;
	f32 m_MouseY = 0.0f;
};

inline bool Input::IsKeyPressed(KeyCode key) const {
	return m_KeyStates[static_cast<size_t>(key)];
}

inline bool Input::IsMouseButtonPressed(MouseButton button) const {
	return m_MouseButtonStates[static_cast<size_t>(button)];
}

inline std::pair<f32, f32> Input::GetMousePosition() const {
	return { m_MouseX, m_MouseY };
}

inline void Input::OnEvent(Event &event) {
	EventDispatcher dispatcher(event);
	dispatcher.Dispatch<KeyPressedEvent>([this](KeyPressedEvent &e) { return OnKeyPressed(e); });
	dispatcher.Dispatch<KeyReleasedEvent>([this](KeyReleasedEvent &e) { return OnKeyReleased(e); });
	dispatcher.Dispatch<MouseButtonPressedEvent>(
		[this](MouseButtonPressedEvent &e) { return OnMouseButtonPressed(e); });
	dispatcher.Dispatch<MouseButtonReleasedEvent>(
		[this](MouseButtonReleasedEvent &e) { return OnMouseButtonReleased(e); });
	dispatcher.Dispatch<MouseMovedEvent>([this](MouseMovedEvent &e) { return OnMouseMoved(e); });
}

inline void Input::Update() {}

inline bool Input::OnKeyPressed(KeyPressedEvent &e) {
	m_KeyStates[static_cast<size_t>(e.GetKeyCode())] = true;
	return false;
}

inline bool Input::OnKeyReleased(KeyReleasedEvent &e) {
	m_KeyStates[static_cast<size_t>(e.GetKeyCode())] = false;
	return false;
}

inline bool Input::OnMouseButtonPressed(MouseButtonPressedEvent &e) {
	m_MouseButtonStates[static_cast<size_t>(e.GetMouseButton())] = true;
	return false;
}

inline bool Input::OnMouseButtonReleased(MouseButtonReleasedEvent &e) {
	m_MouseButtonStates[static_cast<size_t>(e.GetMouseButton())] = false;
	return false;
}

inline bool Input::OnMouseMoved(MouseMovedEvent &e) {
	m_MouseX = e.GetX();
	m_MouseY = e.GetY();
	return false;
}

} // namespace Aquila::Core

#endif
