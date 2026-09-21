#include <doctest.h>

#include "CanvasTestSupport.h"

#include "Aquila/Foundation/FrameScheduler.h"
#include "Aquila/UI/Core/Canvas.h"
#include "Aquila/UI/Core/View.h"

using namespace Aquila::UI::Core;
using Aquila::Foundation::FrameScheduler;

namespace {

void settle(Canvas &canvas) {
	canvas.compute();
	canvas.clear_draw_list_dirty();
	FrameScheduler::get()->consume();
}

}

TEST_SUITE("View removal") {
	TEST_CASE("remove_child schedules a relayout and a frame") {
		CanvasSingletonsScope singletons;
		Canvas canvas(200, 200);
		View *parent = canvas.get_root()->add_child<View>();
		View *child = parent->add_child<View>();
		settle(canvas);

		parent->remove_child(child);

		CHECK(FrameScheduler::get()->consume());
		canvas.compute();
		CHECK(canvas.is_draw_list_dirty());
	}

	TEST_CASE("detach_child schedules a relayout and a frame") {
		CanvasSingletonsScope singletons;
		Canvas canvas(200, 200);
		View *parent = canvas.get_root()->add_child<View>();
		View *child = parent->add_child<View>();
		settle(canvas);

		auto detached = parent->detach_child(child);
		REQUIRE(detached != nullptr);

		CHECK(FrameScheduler::get()->consume());
		canvas.compute();
		CHECK(canvas.is_draw_list_dirty());
	}
}
