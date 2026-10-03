#ifndef AQUILA_INPUT_H
#define AQUILA_INPUT_H

#include "Aquila/Platform/Events/InputEvent.h"

namespace Aquila::Platform {

using namespace Aquila::Platform::Events;

class Input {
  public:
	Input() = delete;

	[[nodiscard]] static bool is_key_pressed(KeyCode key);
	[[nodiscard]] static bool is_mouse_button_pressed(MouseButton button);
	[[nodiscard]] static Vec2 get_mouse_position();

	static void on_event(Event &event);
	static void on_window_destroyed(EventSource window);
};

} // namespace Aquila::Platform
#endif
