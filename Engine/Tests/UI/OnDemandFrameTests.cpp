#include <doctest.h>

#include "CanvasTestSupport.h"

#include "Aquila/Foundation/FrameScheduler.h"
#include "Aquila/Platform/Events/InputEvent.h"
#include "Aquila/Platform/Input.h"
#include "Aquila/Rendering/CameraController.h"
#include "Aquila/UI/Core/Canvas.h"
#include "Aquila/UI/Core/View.h"

using namespace Aquila;
using namespace Aquila::UI::Core;
using Aquila::Foundation::FrameScheduler;
using Aquila::Platform::Input;
namespace Events = Aquila::Platform::Events;

namespace {

int s_window_tag = 0;

void feed(Events::Event &&event) {
	event.set_source(&s_window_tag);
	Input::on_event(event);
}

template <typename T> T sourced(T event) {
	event.set_source(&s_window_tag);
	return event;
}

bool consume() {
	return FrameScheduler::get()->consume();
}

}

TEST_SUITE("On-demand frames") {
	TEST_CASE("changes made while the canvas computes do not ask for another frame") {
		CanvasSingletonsScope singletons;
		Canvas canvas(200, 200);
		View *parent = canvas.get_root()->add_child<View>();
		parent->add_child<View>();
		canvas.update(0.016F);
		canvas.compute();
		consume();

		parent->add_child<View>();
		CHECK(consume());

		canvas.update(0.016F);
		canvas.compute();
		CHECK_FALSE(consume());
	}

	TEST_CASE("the camera only asks for frames while it is actually navigating") {
		CanvasSingletonsScope singletons;
		Rendering::CameraController controller;
		controller.set_viewport_rect(Vec2(0.F), Vec2(100.F));
		feed(Events::MouseMovedEvent(50.F, 50.F));
		consume();

		auto idle_move = sourced(Events::MouseMovedEvent(52.F, 50.F));
		controller.on_event(idle_move);
		CHECK_FALSE(consume());

		feed(Events::MouseButtonPressedEvent(Events::MouseButton::Right));
		controller.update(0.016F);
		consume();
		controller.update(0.016F);
		CHECK_FALSE(consume());

		feed(Events::MouseMovedEvent(60.F, 55.F));
		auto look = sourced(Events::MouseMovedEvent(60.F, 55.F));
		controller.on_event(look);
		CHECK(consume());

		feed(Events::KeyPressedEvent(Events::KeyCode::W, 0));
		controller.update(0.016F);
		CHECK(consume());

		feed(Events::KeyReleasedEvent(Events::KeyCode::W));
		controller.update(0.016F);
		CHECK_FALSE(consume());

		feed(Events::MouseButtonReleasedEvent(Events::MouseButton::Right));
		controller.update(0.016F);
		consume();

		auto scroll = sourced(Events::MouseScrolledEvent(0.F, 1.F));
		controller.on_event(scroll);
		CHECK(consume());

		Input::on_window_destroyed(&s_window_tag);
	}
}
