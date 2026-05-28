#pragma once

#include "Aquila/Foundation/PrimitiveTypes.h"

namespace Aquila::UI {

enum class LengthUnit : uint8 { Pixel, Percent, Auto, Grow };

enum class FlexDirection : uint8 { Row, Column, RowReverse, ColumnReverse };
enum class JustifyContent : uint8 { Start, End, Center };
enum class AlignItems : uint8 { Start, End, Center, Stretch };
enum class FlexWrap : uint8 { NoWrap, Wrap };

enum class Display : uint8 { Flex, None };
enum class Overflow : uint8 { Visible, Hidden, Scroll };
enum class Position : uint8 { Static, Relative, Absolute };

enum class TransitionEasing : uint8 { Linear, Ease, EaseIn, EaseOut, EaseInOut };

struct BoxShadow {
	vec2 offset = { 0.f, 0.f };
	float blur = 0.f;
	float spread = 0.f;
	vec4 color = { 0.f, 0.f, 0.f, 0.75f };
	bool inset = false;

	bool operator==(const BoxShadow &) const = default;
};

enum class TextAlign : uint8 { Left, Center, Right };

enum class FloatingAttachTo : uint8 { Parent, Root };
enum class FloatingAttachPoint : uint8 {
	LeftTop,
	LeftCenter,
	LeftBottom,
	CenterTop,
	Center,
	CenterBottom,
	RightTop,
	RightCenter,
	RightBottom,
};

struct FloatingConfig {
	vec2 offset = {};
	int16_t zIndex = 10;
	FloatingAttachTo attachTo = FloatingAttachTo::Parent;
	FloatingAttachPoint elementPoint = FloatingAttachPoint::LeftTop;
	FloatingAttachPoint parentPoint = FloatingAttachPoint::LeftBottom;
	bool operator==(const FloatingConfig &) const = default;
};

enum class FontSize : uint8 {
	Tiny = 9,
	XSmall = 11,
	Small = 13,
	Body = 16,
	BodyLarge = 18,
	Subtitle = 20,
	Heading = 24,
	HeadingLarge = 28,
	Title = 32,
	TitleLarge = 40,
	Display = 48,
	DisplayLarge = 64,
};

constexpr float FontSizeToPixels(FontSize size) {
	return static_cast<float>(static_cast<uint8>(size));
}

} // namespace Aquila::UI
