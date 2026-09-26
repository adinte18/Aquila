#include <doctest.h>

#include "Aquila/UI/Style/StyleParserHelper.h"

using Aquila::UI::ParserHelper::parse_color;

TEST_CASE("parse_color accepts the transparent keyword") {
	const auto color = parse_color("transparent");
	REQUIRE(color.has_value());
	CHECK(color->a == doctest::Approx(0.F));
}

TEST_CASE("parse_color keeps hex and rgba parsing") {
	const auto hex = parse_color("#4a9e5f");
	REQUIRE(hex.has_value());
	CHECK(hex->a == doctest::Approx(1.F));

	const auto rgba = parse_color("rgba(255, 255, 255, 0.08)");
	REQUIRE(rgba.has_value());
	CHECK(rgba->a == doctest::Approx(0.08F));
}
