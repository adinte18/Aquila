#pragma once

#include "Aquila/UI/Style/StyleTypes.h"

namespace Aquila::UI {

struct StyleLength {
	Type type = Type::Auto;
	float value = 0.0F;

	constexpr static StyleLength Pixel(float value) { return { .type = Type::Pixel, .value = value }; }
	constexpr static StyleLength Percent(float value) { return { .type = Type::Percent, .value = value }; }
	constexpr static StyleLength Auto() { return { .type = Type::Auto, .value = 0 }; }
	constexpr static StyleLength Grow() { return { .type = Type::Grow, .value = 0 }; }

	[[nodiscard]] bool IsAuto() const { return type == Type::Auto; }
	[[nodiscard]] bool IsGrow() const { return type == Type::Grow; }
	[[nodiscard]] bool IsAbsolute() const { return type == Type::Pixel; }
	[[nodiscard]] bool IsRelative() const { return type == Type::Percent; }

	[[nodiscard]] float Resolve(float parentSize) const {
		switch (type) {
		case Type::Pixel:
			return value;
		case Type::Percent:
			return parentSize * (value / 100.0F);
			break;
		case Type::Auto:
		case Type::Grow:
		default:
			return 0.F;
			break;
		}
	}

	bool operator==(const StyleLength &other) const { return type == other.type && value == other.value; }
	bool operator!=(const StyleLength &other) const { return !(*this == other); }
};

struct StyleEdges {
	StyleLength top, right, bottom, left;

	constexpr static StyleEdges All(StyleLength val) {
		return { .top = val, .right = val, .bottom = val, .left = val };
	}
	constexpr static StyleEdges Axes(StyleLength vertical, StyleLength horizontal) {
		return { .top = vertical, .right = horizontal, .bottom = vertical, .left = horizontal };
	}
	constexpr static StyleEdges Zero() { return All(StyleLength::Pixel(0.F)); }

	bool operator==(const StyleEdges &other) const {
		return top == other.top && right == other.right && bottom == other.bottom && left == other.left;
	}
};

} // namespace Aquila::UI
