#pragma once

#include "Aquila/UI/Style/StyleDefaults.h"
#include "Aquila/UI/Style/StyleLength.h"

namespace Aquila::UI {

struct ComputedStyle {
	vec4 backgroundColor = StyleDefaults::BackgroundColor;
	vec4 borderColor = StyleDefaults::BorderColor;
	vec4 borderRadius = StyleDefaults::BorderRadius;
	float borderWidth = StyleDefaults::BorderWidth;
	float opacity = StyleDefaults::Opacity;

	StyleLength width = StyleDefaults::Width;
	StyleLength height = StyleDefaults::Height;

	StyleLength minWidth = StyleDefaults::MinWidth;
	StyleLength maxWidth = StyleDefaults::MaxWidth;
	StyleLength minHeight = StyleDefaults::MinHeight;
	StyleLength maxHeight = StyleDefaults::MaxHeight;

	StyleEdges padding = StyleDefaults::Padding;

	FlexDirection flexDirection = StyleDefaults::FlexDir;

	JustifyContent justify = StyleDefaults::Justify;
	AlignItems align = StyleDefaults::Align;
	FlexWrap wrap = StyleDefaults::Wrap;
	float flexGrow = StyleDefaults::FlexGrow;
	PositionType position = StyleDefaults::Position;

	StyleLength top = StyleDefaults::Top;
	StyleLength bottom = StyleDefaults::Bottom;
	StyleLength left = StyleDefaults::Left;
	StyleLength right = StyleDefaults::Right;

	int zIndex = StyleDefaults::zIndex;
	DisplayType display = StyleDefaults::Display;
	OverflowType overflow = StyleDefaults::Overflow;
	vec4 color = StyleDefaults::Color;
};
} // namespace Aquila::UI
