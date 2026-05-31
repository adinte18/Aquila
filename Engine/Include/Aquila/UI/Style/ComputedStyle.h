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

	f32 fontSize = StyleDefaults::FontSize; // 0 = unset / inherit from parent
	std::string fontFamily;					// "" = unset / inherit from parent
	TextAlign textAlign = TextAlign::Left;

	std::vector<BoxShadow> boxShadows;

	f32 transitionDuration = StyleDefaults::TransitionDuration; // ms
	TransitionEasing transitionEasing = StyleDefaults::TransitionEase;

	bool operator==(const ComputedStyle &b) const {
		return backgroundColor == b.backgroundColor && borderColor == b.borderColor && borderRadius == b.borderRadius &&
			   borderWidth == b.borderWidth && opacity == b.opacity && width == b.width && height == b.height &&
			   minWidth == b.minWidth && maxWidth == b.maxWidth && minHeight == b.minHeight &&
			   maxHeight == b.maxHeight && padding == b.padding && gap == b.gap && flexDirection == b.flexDirection &&
			   justify == b.justify && align == b.align && wrap == b.wrap && flexGrow == b.flexGrow &&
			   position == b.position && top == b.top && bottom == b.bottom && left == b.left && right == b.right &&
			   zIndex == b.zIndex && display == b.display && overflow == b.overflow &&
			   color == b.color && accentColor == b.accentColor &&
			   fontSize == b.fontSize && fontFamily == b.fontFamily && textAlign == b.textAlign &&
			   boxShadows == b.boxShadows && transitionDuration == b.transitionDuration &&
			   transitionEasing == b.transitionEasing;
	}

	bool operator!=(const ComputedStyle &b) const { return !(*this == b); }
};

} // namespace Aquila::UI
