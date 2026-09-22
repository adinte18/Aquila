#ifndef AQUILA_INPUT_H
#define AQUILA_INPUT_H

#include "Aquila/Foundation/SharedConstants.h"
#include "Aquila/Platform/Events/InputEvent.h"

namespace Aquila::Platform {

using namespace Aquila::Platform::Events;

class Input {
  public:
	Input() = delete;

	[[nodiscard]] static bool is_key_pressed(KeyCode key) {
		if (s_active_window == nullptr) {
			return false;
		}
		return s_states[s_active_window].key_states.at((Uint8)key);
	}
	[[nodiscard]] static bool is_mouse_button_pressed(MouseButton button) {
		if (s_active_window == nullptr) {
			return false;
		}
		return s_states[s_active_window].mouse_button_states.at((Uint8)button);
	}
	[[nodiscard]] static Vec2 get_mouse_position() {
		if (s_active_window == nullptr) {
			return {};
		}
		auto &state = s_states[s_active_window];
		return { state.mouse_x, state.mouse_y };
	}

	static void on_event(Event &event) {
		auto *window = event.get_source();
		if (window == nullptr) {
			return;
		}

		auto &state = s_states[window];
		s_active_window = window;

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

	static void on_window_destroyed(EventSource window) {
		s_states.erase(window);
		if (s_active_window == window) {
			s_active_window = nullptr;
		}
	}

  private:
	struct InputState {
		std::array<bool, SharedConstants::MAX_KEY_STATES> key_states{};
		std::array<bool, SharedConstants::MAX_MOUSE_STATES> mouse_button_states{};
		F32 mouse_x = 0.0F;
		F32 mouse_y = 0.0F;
	};

	inline static std::unordered_map<EventSource, InputState> s_states;
	inline static EventSource s_active_window = nullptr;
};

} // namespace Aquila::Platform
#endif
