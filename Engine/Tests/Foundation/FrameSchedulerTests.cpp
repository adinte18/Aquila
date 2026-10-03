#include <doctest.h>

#include "Aquila/Foundation/FrameScheduler.h"

using Aquila::Foundation::FrameScheduler;

namespace {

struct SchedulerScope {
	SchedulerScope() {
		FrameScheduler::init();
		FrameScheduler::get()->consume();
	}
	~SchedulerScope() { FrameScheduler::shutdown(); }
};

}

TEST_SUITE("FrameScheduler") {
	TEST_CASE("the first frame is pending right after init") {
		FrameScheduler::init();
		CHECK(FrameScheduler::get()->is_pending());
		CHECK(FrameScheduler::get()->consume());
		CHECK_FALSE(FrameScheduler::get()->is_pending());
		FrameScheduler::shutdown();
	}

	TEST_CASE("a request is consumed exactly once") {
		SchedulerScope scope;
		FrameScheduler *scheduler = FrameScheduler::get();
		scheduler->request_frame();
		scheduler->request_frame();
		CHECK(scheduler->is_pending());
		CHECK(scheduler->consume());
		CHECK_FALSE(scheduler->consume());
	}

	TEST_CASE("a targeted request renders only its target") {
		SchedulerScope scope;
		FrameScheduler *scheduler = FrameScheduler::get();
		int window = 0;
		int other = 0;
		scheduler->request_frame(&window);
		CHECK(scheduler->is_pending());
		CHECK_FALSE(scheduler->consume());
		CHECK_FALSE(scheduler->consume(&other));
		CHECK(scheduler->consume(&window));
		CHECK_FALSE(scheduler->consume(&window));
		CHECK_FALSE(scheduler->is_pending());
	}

	TEST_CASE("a null target is a full frame") {
		SchedulerScope scope;
		FrameScheduler *scheduler = FrameScheduler::get();
		scheduler->request_frame(nullptr);
		CHECK(scheduler->consume());
	}
}
