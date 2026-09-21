#ifndef AQUILA_TESTS_UI_CANVAS_TEST_SUPPORT_H
#define AQUILA_TESTS_UI_CANVAS_TEST_SUPPORT_H

#include "Aquila/Foundation/FrameScheduler.h"
#include "Aquila/Foundation/Profiler.h"

struct CanvasSingletonsScope {
	CanvasSingletonsScope() {
		Aquila::Foundation::FrameScheduler::init();
		Aquila::Foundation::Profiler::init();
	}
	~CanvasSingletonsScope() {
		Aquila::Foundation::Profiler::shutdown();
		Aquila::Foundation::FrameScheduler::shutdown();
	}
};

#endif
