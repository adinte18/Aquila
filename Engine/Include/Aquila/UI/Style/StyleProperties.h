#pragma once

#include "Aquila/UI/Style/StyleLength.h"
#include <vector>

namespace Aquila::UI {

struct StyleProperties {
	Option<vec4> backgroundColor;
	Option<vec4> borderColor;
	Option<f32> borderWidth;
	Option<vec4> borderRadius;
	Option<f32> opacity;
	Option<Overflow> overflow;
	Option<Display> display;

	Option<StyleLength> width, height;
	Option<StyleLength> min, max;
	Option<StyleLength> minWidth, maxWidth;
	Option<StyleLength> minHeight, maxHeight;
	Option<StyleEdges> padding;
	Option<StyleLength> paddingLeft, paddingRight, paddingTop, paddingBottom;
	Option<f32> gap;

	Option<FlexDirection> flexDirection;
	Option<JustifyContent> justifyContent;
	Option<AlignItems> alignItems;
	Option<f32> flexGrow;
	Option<FlexWrap> flexWrap;

	Option<Position> position;
	Option<StyleLength> top, right, bottom, left;
	Option<int32> zIndex;

	Option<vec4> color;
	Option<vec4> accentColor;
	Option<f32> fontSize;
	Option<std::string> fontFamily;
	Option<TextAlign> textAlign;

	Option<std::vector<BoxShadow>> boxShadows;

	Option<f32> transitionDuration; // milliseconds
	Option<TransitionEasing> transitionEasing;
};

} // namespace Aquila::UI
