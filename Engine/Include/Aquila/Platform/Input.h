#ifndef AQUILA_INPUT_H
#define AQUILA_INPUT_H

#include "Aquila/Foundation/SharedConstants.h"
#include "Aquila/Application/Events/InputEvent.h"

namespace Aquila::Platform {

using namespace Aquila::Application::Events;

class Input {
  public:
	Input() = delete;

	[[nodiscard]] static bool IsKeyPressed(KeyCode key) { return s_KeyStates.at(static_cast<usize>(key)); }
	[[nodiscard]] static bool IsMouseButtonPressed(MouseButton button) {
		return s_MouseButtonStates.at(static_cast<usize>(button));
	}
	[[nodiscard]] static std::pair<f32, f32> GetMousePosition() { return { s_MouseX, s_MouseY }; }

	static void OnEvent(Event &event) {
		EventDispatcher dispatcher(event);
		dispatcher.Dispatch<KeyPressedEvent>([](KeyPressedEvent &e) {
			s_KeyStates.at(static_cast<usize>(e.GetKeyCode())) = true;
			return false;
		});
		dispatcher.Dispatch<KeyReleasedEvent>([](KeyReleasedEvent &e) {
			s_KeyStates.at(static_cast<usize>(e.GetKeyCode())) = false;
			return false;
		});
		dispatcher.Dispatch<MouseButtonPressedEvent>([](MouseButtonPressedEvent &e) {
			s_MouseButtonStates.at(static_cast<usize>(e.GetMouseButton())) = true;
			return false;
		});
		dispatcher.Dispatch<MouseButtonReleasedEvent>([](MouseButtonReleasedEvent &e) {
			s_MouseButtonStates.at(static_cast<usize>(e.GetMouseButton())) = false;
			return false;
		});
		dispatcher.Dispatch<MouseMovedEvent>([](MouseMovedEvent &e) {
			s_MouseX = e.GetX();
			s_MouseY = e.GetY();
			return false;
		});
	}

  private:
	inline static std::array<bool, SharedConstants::MAX_KEY_STATES> s_KeyStates{};
	inline static std::array<bool, SharedConstants::MAX_MOUSE_STATES> s_MouseButtonStates{};
	inline static f32 s_MouseX = 0.0f;
	inline static f32 s_MouseY = 0.0f;
};

} // namespace Aquila::Platform
#endif
