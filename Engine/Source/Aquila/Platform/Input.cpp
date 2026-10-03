#include "Aquila/Platform/Input.h"
#include "Aquila/Foundation/SharedConstants.h"

#include <array>
#include <unordered_map>

namespace Aquila::Platform {

namespace {

struct InputState {
	std::array<bool, SharedConstants::MAX_KEY_STATES> key_states{};
	std::array<bool, SharedConstants::MAX_MOUSE_STATES> mouse_button_states{};
	F32 mouse_x = 0.F;
	F32 mouse_y = 0.F;
};

std::unordered_map<EventSource, InputState> s_states;
EventSource s_active_window = nullptr;

}

bool Input::is_key_pressed(KeyCode key) {
	if (s_active_window == nullptr) {
		return false;
	}
	return s_states[s_active_window].key_states.at((Uint8)key);
}

bool Input::is_mouse_button_pressed(MouseButton button) {
	if (s_active_window == nullptr) {
		return false;
	}
	return s_states[s_active_window].mouse_button_states.at((Uint8)button);
}

Vec2 Input::get_mouse_position() {
	if (s_active_window == nullptr) {
		return {};
	}
	auto &state = s_states[s_active_window];
	return { state.mouse_x, state.mouse_y };
}

void Input::on_event(Event &event) {
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

void Input::on_window_destroyed(EventSource window) {
	s_states.erase(window);
	if (s_active_window == window) {
		s_active_window = nullptr;
	}
}

}
