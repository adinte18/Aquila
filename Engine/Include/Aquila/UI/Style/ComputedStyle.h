#pragma once

#include "Aquila/UI/Style/StyleDefaults.h"
#include "Aquila/UI/Style/StyleLength.h"
#include <string>
#include <vector>

namespace Aquila::UI {

struct ComputedStyle {
	vec4 backgroundColor = StyleDefaults::BackgroundColor;
	vec4 borderColor = StyleDefaults::BorderColor;
	vec4 borderRadius = StyleDefaults::BorderRadius;
	f32 borderWidth = StyleDefaults::BorderWidth;
	f32 opacity = StyleDefaults::Opacity;

	StyleLength width = StyleDefaults::Width;
	StyleLength height = StyleDefaults::Height;

	StyleLength minWidth = StyleDefaults::MinWidth;
	StyleLength maxWidth = StyleDefaults::MaxWidth;
	StyleLength minHeight = StyleDefaults::MinHeight;
	StyleLength maxHeight = StyleDefaults::MaxHeight;

	StyleEdges padding = StyleDefaults::Padding;
	f32 gap = 0.f;
	f32 aspectRatio = 0.f; // 0 = unset (width/height ratio when one axis is auto)

	FlexDirection flexDirection = StyleDefaults::FlexDir;
	JustifyContent justify = StyleDefaults::Justify;
	AlignItems align = StyleDefaults::Align;
	FlexWrap wrap = StyleDefaults::Wrap;
	f32 flexGrow = StyleDefaults::FlexGrow;

	Position position = StyleDefaults::Pos;
	StyleLength top = StyleDefaults::Top;
	StyleLength bottom = StyleDefaults::Bottom;
	StyleLength left = StyleDefaults::Left;
	StyleLength right = StyleDefaults::Right;

	int zIndex = StyleDefaults::ZIndex;
	Display display = StyleDefaults::Disp;
	Overflow overflow = StyleDefaults::Overflow;
	vec4 color = StyleDefaults::Color;
	vec4 accentColor = vec4(0.f); // transparent sentinel; if alpha==0 widgets fall back to style.color
	vec4 selectionColor = vec4(0.f); // transparent sentinel; if alpha==0 widgets derive from style.color
	vec4 placeholderColor = vec4(0.f); // transparent sentinel; if alpha==0 widgets derive from style.color

	f32 fontSize = StyleDefaults::FontSize; // 0 = unset / inherit from parent
	std::string fontFamily; // "" = unset / inherit from parent
	TextAlign textAlign = TextAlign::Left;

	std::vector<BoxShadow> boxShadows;

	f32 transitionDuration = StyleDefaults::TransitionDuration; // ms
	TransitionEasing transitionEasing = StyleDefaults::TransitionEase;

	bool operator==(const ComputedStyle &b) const = default;

	[[nodiscard]] vec4 EffectiveAccentColor() const { return accentColor.a > 0.f ? accentColor : color; }
	[[nodiscard]] vec4 EffectiveSelectionColor() const {
		return selectionColor.a > 0.f ? selectionColor : vec4(color.r, color.g, color.b, 0.3f);
	}
	[[nodiscard]] vec4 EffectivePlaceholderColor() const {
		return placeholderColor.a > 0.f ? placeholderColor : vec4(color.r, color.g, color.b, color.a * 0.45f);
	}
};

} // namespace Aquila::UI
