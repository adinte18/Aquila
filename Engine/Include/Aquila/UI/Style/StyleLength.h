#pragma once

#include "Aquila/UI/Style/StyleTypes.h"

namespace Aquila::UI {

struct StyleLength {
	LengthUnit unit = LengthUnit::Auto;
	float value = 0.0f;

	constexpr static StyleLength Pixel(float pixels) { return { .unit = LengthUnit::Pixel, .value = pixels }; }
	constexpr static StyleLength Percent(float percent) { return { .unit = LengthUnit::Percent, .value = percent }; }
	constexpr static StyleLength Auto() { return { .unit = LengthUnit::Auto, .value = 0.f }; }
	constexpr static StyleLength Grow() { return { .unit = LengthUnit::Grow, .value = 0.f }; }

	[[nodiscard]] bool IsAuto() const { return unit == LengthUnit::Auto; }
	[[nodiscard]] bool IsGrow() const { return unit == LengthUnit::Grow; }
	[[nodiscard]] bool IsPixel() const { return unit == LengthUnit::Pixel; }
	[[nodiscard]] bool IsPercent() const { return unit == LengthUnit::Percent; }

	[[nodiscard]] float Resolve(float parentSize) const {
		switch (unit) {
		case LengthUnit::Pixel:
			return value;
		case LengthUnit::Percent:
			return parentSize * (value / 100.0f);
		default:
			return 0.f;
		}
	}

	bool operator==(const StyleLength &other) const { return unit == other.unit && value == other.value; }
	bool operator!=(const StyleLength &other) const { return !(*this == other); }
};

struct StyleEdges {
	StyleLength top, right, bottom, left;

	constexpr static StyleEdges All(StyleLength value) {
		return { .top = value, .right = value, .bottom = value, .left = value };
	}
	constexpr static StyleEdges Axes(StyleLength vertical, StyleLength horizontal) {
		return { .top = vertical, .right = horizontal, .bottom = vertical, .left = horizontal };
	}
	constexpr static StyleEdges Zero() { return All(StyleLength::Pixel(0.f)); }

	bool operator==(const StyleEdges &other) const {
		return top == other.top && right == other.right && bottom == other.bottom && left == other.left;
	}
};

} // namespace Aquila::UI
