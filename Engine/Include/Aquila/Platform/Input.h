#ifndef AQUILA_INPUT_H
#define AQUILA_INPUT_H

#include "Aquila/Foundation/SharedConstants.h"
#include "Aquila/Application/Events/InputEvent.h"

namespace Aquila::Application {
class Window;
}

namespace Aquila::Platform {

using namespace Aquila::Application::Events;

class Input {
  public:
	Input() = delete;

	[[nodiscard]] static bool IsKeyPressed(KeyCode key) {
		if (!s_ActiveWindow) {
			return false;
		}
		return s_States[s_ActiveWindow].KeyStates.at((uint8)key);
	}
	[[nodiscard]] static bool IsMouseButtonPressed(MouseButton button) {
		if (!s_ActiveWindow) {
			return false;
		}
		return s_States[s_ActiveWindow].MouseButtonStates.at((uint8)button);
	}
	[[nodiscard]] static vec2 GetMousePosition() {
		if (!s_ActiveWindow) {
			return {};
		}
		auto &state = s_States[s_ActiveWindow];
		return { state.MouseX, state.MouseY };
	}

	static void OnEvent(Event &event) {
		auto *window = event.GetSource();
		if (!window) {
			return;
		}

		auto &state = s_States[window];
		s_ActiveWindow = window;

		EventDispatcher dispatcher(event);

		dispatcher.Dispatch<KeyPressedEvent>([&](KeyPressedEvent &e) {
			state.KeyStates.at((uint8)e.GetKeyCode()) = true;
			return false;
		});

		dispatcher.Dispatch<KeyReleasedEvent>([&](KeyReleasedEvent &e) {
			state.KeyStates.at((uint8)e.GetKeyCode()) = false;
			return false;
		});

		dispatcher.Dispatch<MouseButtonPressedEvent>([&](MouseButtonPressedEvent &e) {
			state.MouseButtonStates.at((uint8)e.GetMouseButton()) = true;
			return false;
		});

		dispatcher.Dispatch<MouseButtonReleasedEvent>([&](MouseButtonReleasedEvent &e) {
			state.MouseButtonStates.at((uint8)e.GetMouseButton()) = false;
			return false;
		});

		dispatcher.Dispatch<MouseMovedEvent>([&](MouseMovedEvent &e) {
			state.MouseX = e.GetX();
			state.MouseY = e.GetY();
			return false;
		});
	}

	static void OnWindowDestroyed(Application::Window *window) {
		s_States.erase(window);
		if (s_ActiveWindow == window) {
			s_ActiveWindow = nullptr;
		}
	}

  private:
	struct InputState {
		std::array<bool, SharedConstants::MAX_KEY_STATES> KeyStates{};
		std::array<bool, SharedConstants::MAX_MOUSE_STATES> MouseButtonStates{};
		f32 MouseX = 0.0f;
		f32 MouseY = 0.0f;
	};

	inline static std::unordered_map<Application::Window *, InputState> s_States;
	inline static Application::Window *s_ActiveWindow = nullptr;
};

} // namespace Aquila::Platform
#endif
