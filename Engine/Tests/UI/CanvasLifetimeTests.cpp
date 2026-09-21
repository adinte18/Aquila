#include <doctest.h>

#include "CanvasTestSupport.h"

#include "Aquila/UI/Core/Canvas.h"

using namespace Aquila::UI::Core;

TEST_SUITE("Canvas lifetime") {
	TEST_CASE("a canvas can be created after another one was destroyed") {
		CanvasSingletonsScope singletons;
		{
			Canvas first(200, 200);
		}
		Canvas second(300, 100);
		CHECK(second.get_root() != nullptr);
	}

	TEST_CASE("canvases can overlap in lifetime") {
		CanvasSingletonsScope singletons;
		Canvas first(200, 200);
		{
			Canvas second(100, 100);
		}
		Canvas third(50, 50);
		CHECK(third.get_root() != nullptr);
	}
}
