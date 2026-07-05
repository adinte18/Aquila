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

	[[nodiscard]] static bool is_key_pressed(KeyCode key) {
		if (!s_ActiveWindow) {
			return false;
		}
		return s_States[s_ActiveWindow].key_states.at((Uint8)key);
	}
	[[nodiscard]] static bool is_mouse_button_pressed(MouseButton button) {
		if (!s_ActiveWindow) {
			return false;
		}
		return s_States[s_ActiveWindow].mouse_button_states.at((Uint8)button);
	}
	[[nodiscard]] static Vec2 get_mouse_position() {
		if (!s_ActiveWindow) {
			return {};
		}
		auto &state = s_States[s_ActiveWindow];
		return { state.mouse_x, state.mouse_y };
	}

	static void on_event(Event &event) {
		auto *window = event.get_source();
		if (!window) {
			return;
		}

		auto &state = s_States[window];
		s_ActiveWindow = window;

		EventDispatcher dispatcher(event);

		dispatcher.dispatch<KeyPressedEvent>([&](KeyPressedEvent &e) {
			state.key_states.at((Uint8)e.get_key_code()) = true;
			return false;
		});

		dispatcher.dispatch<KeyReleasedEvent>([&](KeyReleasedEvent &e) {
			state.key_states.at((Uint8)e.get_key_code()) = false;
			return false;
		});

		dispatcher.dispatch<MouseButtonPressedEvent>([&](MouseButtonPressedEvent &e) {
			state.mouse_button_states.at((Uint8)e.get_mouse_button()) = true;
			return false;
		});

		dispatcher.dispatch<MouseButtonReleasedEvent>([&](MouseButtonReleasedEvent &e) {
			state.mouse_button_states.at((Uint8)e.get_mouse_button()) = false;
			return false;
		});

		dispatcher.dispatch<MouseMovedEvent>([&](MouseMovedEvent &e) {
			state.mouse_x = e.get_x();
			state.mouse_y = e.get_y();
			return false;
		});
	}

	static void on_window_destroyed(Application::Window *window) {
		s_States.erase(window);
		if (s_ActiveWindow == window) {
			s_ActiveWindow = nullptr;
		}
	}

  private:
	struct InputState {
		std::array<bool, SharedConstants::MAX_KEY_STATES> key_states{};
		std::array<bool, SharedConstants::MAX_MOUSE_STATES> mouse_button_states{};
		F32 mouse_x = 0.0f;
		F32 mouse_y = 0.0f;
	};

	inline static std::unordered_map<Application::Window *, InputState> s_States;
	inline static Application::Window *s_ActiveWindow = nullptr;
};

} // namespace Aquila::Platform
#endif
