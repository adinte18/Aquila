#include <doctest.h>

#include "Aquila/UI/Core/FontRegistry.h"
#include "Aquila/UI/Core/View.h"
#include "Aquila/UI/Style/StyleParser.h"
#include "Aquila/UI/Style/StyleSheet.h"

using namespace Aquila::UI;

namespace {

struct ScaleScope {
	explicit ScaleScope(float scale) { Core::FontRegistry::set_ui_scale(scale); }
	~ScaleScope() { Core::FontRegistry::set_ui_scale(1.F); }
};

ComputedStyle resolve_box(const char *css, Core::View &view) {
	StyleSheet sheet;
	StyleParser::LoadString(css, sheet);
	return sheet.resolve(view, nullptr);
}

} // namespace

TEST_SUITE("UI scale") {
	TEST_CASE("pixel metrics from the stylesheet follow the scale") {
		ScaleScope scale(0.5F);
		Core::View view;
		view.add_class("box");
		const ComputedStyle style =
			resolve_box(".box { width: 100px; height: 40px; padding: 10px; gap: 8px; border-radius: 6px; border-width: 2px; font-size: 20px; }", view);

		CHECK(style.width.value == doctest::Approx(50.F));
		CHECK(style.height.value == doctest::Approx(20.F));
		CHECK(style.padding.left.value == doctest::Approx(5.F));
		CHECK(style.gap == doctest::Approx(4.F));
		CHECK(style.border_radius.x == doctest::Approx(3.F));
		CHECK(style.border_width == doctest::Approx(1.F));
		CHECK(style.font_size == doctest::Approx(10.F));
	}

	TEST_CASE("percent and grow lengths are not scaled") {
		ScaleScope scale(0.5F);
		Core::View view;
		view.add_class("box");
		const ComputedStyle style = resolve_box(".box { width: 50%; height: grow; }", view);

		CHECK(style.width.unit == LengthUnit::Percent);
		CHECK(style.width.value == doctest::Approx(50.F));
		CHECK(style.height.unit == LengthUnit::Grow);
	}

	TEST_CASE("inline styles carry measured pixels and are left alone") {
		ScaleScope scale(0.5F);
		Core::View view;
		StyleProperties inline_style;
		inline_style.width = StyleLength::pixel(100.F);
		view.set_style(inline_style);

		const ComputedStyle style = resolve_box("", view);
		CHECK(style.width.value == doctest::Approx(100.F));
	}

	TEST_CASE("at scale one nothing changes") {
		Core::View view;
		view.add_class("box");
		const ComputedStyle style = resolve_box(".box { width: 100px; padding: 10px; }", view);

		CHECK(style.width.value == doctest::Approx(100.F));
		CHECK(style.padding.left.value == doctest::Approx(10.F));
	}
}
