#include <doctest.h>

#include "CanvasTestSupport.h"

#include "Aquila/UI/Core/Canvas.h"
#include "Aquila/UI/Core/View.h"

using namespace Aquila::UI;
using namespace Aquila::UI::Core;

namespace {

void size(View &view, float width, float height) {
	StyleProperties style;
	style.width = StyleLength::pixel(width);
	style.height = StyleLength::pixel(height);
	view.merge_style(style);
}

void column(View &view) {
	StyleProperties style;
	style.flex_direction = FlexDirection::Column;
	view.merge_style(style);
}

void frame(Canvas &canvas) {
	for (int i = 0; i < 3; ++i) {
		canvas.update(0.016F);
		canvas.compute();
	}
}

}

TEST_SUITE("Nested floating layers") {
	TEST_CASE("a popup inside a floating panel is hit before the panel's later siblings") {
		CanvasSingletonsScope singletons;
		Canvas canvas(400, 400);

		auto panel_owned = std::make_unique<View>();
		size(*panel_owned, 200, 200);
		column(*panel_owned);
		FloatingConfig panel_floating;
		panel_floating.attach_to = FloatingAttachTo::Root;
		panel_floating.element_point = FloatingAttachPoint::LeftTop;
		panel_floating.parent_point = FloatingAttachPoint::LeftTop;
		panel_floating.z_index = 20;
		panel_owned->set_floating(panel_floating);
		View *panel = canvas.get_root()->add_child(std::move(panel_owned));

		View *first = panel->add_child<View>();
		size(*first, 100, 40);
		View *second = panel->add_child<View>();
		size(*second, 100, 40);

		auto popup_owned = std::make_unique<View>();
		size(*popup_owned, 100, 100);
		FloatingConfig popup_floating;
		popup_floating.attach_to = FloatingAttachTo::Parent;
		popup_floating.element_point = FloatingAttachPoint::LeftTop;
		popup_floating.parent_point = FloatingAttachPoint::LeftBottom;
		popup_floating.z_index = 100;
		popup_owned->set_floating(popup_floating);
		View *popup = first->add_child(std::move(popup_owned));

		frame(canvas);

		const Rect second_rect = second->get_absolute_rect();
		const Vec2 inside_both = second_rect.center();
		REQUIRE(popup->get_absolute_rect().contains(inside_both));
		CHECK(canvas.hit_test(inside_both) == popup);
	}
}
