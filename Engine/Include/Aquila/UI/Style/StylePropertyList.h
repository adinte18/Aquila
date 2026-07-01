#pragma once

//   cssName    — the CSS property name (documentation / cross-reference)
//   spMember   — the Option<T> field in StyleProperties
//   csMember   — the resolved field in ComputedStyle
//   LAYOUT     — 1 if a change forces a re-layout
//   ANIM       — 1 if the property is interpolated by transitions (T must be
//   INHERIT    — 1 if the property inherits from the parent's computed value
// in this list — they remain explicit special cases in the consumers, applied

// clang-format off
#define AQ_STYLE_PROPERTY_LIST \
	AQ_STYLE_PROP("background-color",   backgroundColor,  backgroundColor,  0, 1, 0) \
	AQ_STYLE_PROP("border-color",       borderColor,      borderColor,      0, 1, 0) \
	AQ_STYLE_PROP("border-width",       borderWidth,      borderWidth,      0, 1, 0) \
	AQ_STYLE_PROP("border-radius",      borderRadius,     borderRadius,     0, 1, 0) \
	AQ_STYLE_PROP("opacity",            opacity,          opacity,          0, 1, 0) \
	AQ_STYLE_PROP("overflow",           overflow,         overflow,         1, 0, 0) \
	AQ_STYLE_PROP("display",            display,          display,          1, 0, 0) \
	AQ_STYLE_PROP("width",              width,            width,            1, 0, 0) \
	AQ_STYLE_PROP("height",             height,           height,           1, 0, 0) \
	AQ_STYLE_PROP("min-width",          minWidth,         minWidth,         1, 0, 0) \
	AQ_STYLE_PROP("max-width",          maxWidth,         maxWidth,         1, 0, 0) \
	AQ_STYLE_PROP("min-height",         minHeight,        minHeight,        1, 0, 0) \
	AQ_STYLE_PROP("max-height",         maxHeight,        maxHeight,        1, 0, 0) \
	AQ_STYLE_PROP("padding",            padding,          padding,          1, 0, 0) \
	AQ_STYLE_PROP("gap",                gap,              gap,              1, 0, 0) \
	AQ_STYLE_PROP("aspect-ratio",       aspectRatio,      aspectRatio,      1, 0, 0) \
	AQ_STYLE_PROP("flex-direction",     flexDirection,    flexDirection,    1, 0, 0) \
	AQ_STYLE_PROP("justify-content",    justifyContent,   justify,          1, 0, 0) \
	AQ_STYLE_PROP("align-items",        alignItems,       align,            1, 0, 0) \
	AQ_STYLE_PROP("flex-wrap",          flexWrap,         wrap,             1, 0, 0) \
	AQ_STYLE_PROP("flex-grow",          flexGrow,         flexGrow,         1, 0, 0) \
	AQ_STYLE_PROP("position",           position,         position,         1, 0, 0) \
	AQ_STYLE_PROP("top",                top,              top,              1, 0, 0) \
	AQ_STYLE_PROP("right",              right,            right,            1, 0, 0) \
	AQ_STYLE_PROP("bottom",             bottom,           bottom,           1, 0, 0) \
	AQ_STYLE_PROP("left",               left,             left,             1, 0, 0) \
	AQ_STYLE_PROP("z-index",            zIndex,           zIndex,           1, 0, 0) \
	AQ_STYLE_PROP("color",              color,            color,            0, 1, 1) \
	AQ_STYLE_PROP("accent-color",       accentColor,      accentColor,      0, 0, 0) \
	AQ_STYLE_PROP("selection-color",    selectionColor,   selectionColor,   0, 0, 0) \
	AQ_STYLE_PROP("placeholder-color",  placeholderColor, placeholderColor, 0, 0, 0) \
	AQ_STYLE_PROP("font-size",          fontSize,         fontSize,         1, 0, 1) \
	AQ_STYLE_PROP("font-family",        fontFamily,       fontFamily,       1, 0, 1) \
	AQ_STYLE_PROP("text-align",         textAlign,        textAlign,        0, 0, 0) \
	AQ_STYLE_PROP("box-shadow",         boxShadows,       boxShadows,       0, 0, 0) \
	AQ_STYLE_PROP("transition-duration",transitionDuration,transitionDuration,0,0,0) \
	AQ_STYLE_PROP("transition-easing",  transitionEasing, transitionEasing, 0, 0, 0)
// clang-format on

#define AQ_STYLE_WHEN_CAT_(a, b) a##b
#define AQ_STYLE_WHEN_CAT(a, b) AQ_STYLE_WHEN_CAT_(a, b)
#define AQ_STYLE_WHEN_0(...)
#define AQ_STYLE_WHEN_1(...) __VA_ARGS__
#define AQ_STYLE_WHEN(flag, ...) AQ_STYLE_WHEN_CAT(AQ_STYLE_WHEN_, flag)(__VA_ARGS__)
