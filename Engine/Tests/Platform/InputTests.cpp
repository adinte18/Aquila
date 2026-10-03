#include <doctest.h>

#include "Aquila/Platform/Input.h"

using namespace Aquila::Platform;
using namespace Aquila::Platform::Events;

namespace {

void send(Event &event, const void *window) {
	event.set_source(window);
	Input::on_event(event);
}

}

TEST_SUITE("Input") {
	TEST_CASE("keys above 255 don't alias other keys") {
		const int window = 0;
		KeyPressedEvent control(KeyCode::LeftControl, 0);
		send(control, &window);

		CHECK(Input::is_key_pressed(KeyCode::LeftControl));
		CHECK_FALSE(Input::is_key_pressed(KeyCode::U));

		KeyReleasedEvent release(KeyCode::LeftControl);
		send(release, &window);
		CHECK_FALSE(Input::is_key_pressed(KeyCode::LeftControl));

		Input::on_window_destroyed(&window);
	}

	TEST_CASE("state follows the window that had the last input") {
		const int first = 0;
		const int second = 0;
		KeyPressedEvent press(KeyCode::W, 0);
		send(press, &first);
		CHECK(Input::is_key_pressed(KeyCode::W));

		MouseMovedEvent move(10.F, 20.F);
		send(move, &second);
		CHECK_FALSE(Input::is_key_pressed(KeyCode::W));

		Input::on_window_destroyed(&first);
		Input::on_window_destroyed(&second);
		CHECK_FALSE(Input::is_key_pressed(KeyCode::W));
	}
}
