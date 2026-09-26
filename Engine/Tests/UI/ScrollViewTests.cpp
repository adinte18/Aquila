#include <doctest.h>

#include "CanvasTestSupport.h"

#include "Aquila/UI/Core/Canvas.h"
#include "Aquila/UI/Style/StyleParser.h"
#include "Aquila/UI/Widgets/ScrollView.h"

using namespace Aquila::UI;
using namespace Aquila::UI::Core;

namespace {

void frames(Canvas &canvas) {
	for (int i = 0; i < 4; ++i) {
		canvas.update(0.016F);
		canvas.compute();
	}
}

struct TallScroll {
	explicit TallScroll(Canvas &canvas) {
		StyleParser::LoadString("ScrollView { overflow: scroll; width: 200px; height: 100px; flex-direction: column; }",
								canvas.get_style_sheet());
		scroll = canvas.get_root()->add_child<ScrollView>();
		content = scroll->add_child(std::make_unique<View>());
		StyleProperties style;
		style.width = StyleLength::pixel(100.F);
		style.height = StyleLength::pixel(500.F);
		content->set_style(style);
		frames(canvas);
	}

	ScrollView *scroll = nullptr;
	View *content = nullptr;
};

float bottom_of(const View &view) {
	const Rect rect = view.get_absolute_rect();
	return rect.position.y + rect.size.y;
}

} // namespace

TEST_SUITE("ScrollView") {
	TEST_CASE("content starts at the top") {
		CanvasSingletonsScope singletons;
		Canvas canvas(400, 400);
		TallScroll tall(canvas);

		CHECK_FALSE(tall.scroll->is_at_bottom());
	}

	TEST_CASE("scroll_to_bottom shows the end of tall content") {
		CanvasSingletonsScope singletons;
		Canvas canvas(400, 400);
		TallScroll tall(canvas);

		tall.scroll->scroll_to_bottom();
		frames(canvas);

		CHECK(tall.scroll->is_at_bottom());
		CHECK(bottom_of(*tall.content) == doctest::Approx(bottom_of(*tall.scroll)).epsilon(0.01));
	}

	TEST_CASE("scrolling back up leaves the bottom") {
		CanvasSingletonsScope singletons;
		Canvas canvas(400, 400);
		TallScroll tall(canvas);

		tall.scroll->scroll_to_bottom();
		frames(canvas);
		canvas.set_scroll_offset(tall.scroll, 0.F);
		frames(canvas);

		CHECK_FALSE(tall.scroll->is_at_bottom());
	}
}
