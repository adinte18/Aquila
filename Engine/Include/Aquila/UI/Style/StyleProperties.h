#pragma once

#include "Aquila/UI/Style/StyleLength.h"

namespace Aquila::UI {
struct StyleProperties {
	Option<vec4> backgroundColor;
	Option<vec4> borderColor;
	Option<float> borderWidth;
	Option<vec4> borderRadius;
	Option<float> opacity;
	Option<OverflowType> overflow; // maps to CLAY_SCROLL
	Option<DisplayType> display;

	Option<StyleLength> width, height;
	Option<StyleLength> min, max; // if min is set or max is set, maxWidth/Height is set to the that value
	Option<StyleLength> minWidth, maxWidth;
	Option<StyleLength> minHeight, maxHeight;
	Option<StyleEdges> padding; // margin handled by Clay gap/spacing
	Option<FlexDirection> flexDirection;
	Option<JustifyContent> justifyContent;
	Option<AlignItems> alignItems;
	Option<float> flexGrow; // maps to CLAY_SIZING_GROW(weight)

	Option<PositionType> position;
	Option<StyleLength> top, right, bottom, left;
	Option<int32> zIndex;
	Option<FlexWrap> flexWrap;

	Option<vec4> color;
	Option<float> fontSize;
};
} // namespace Aquila::UI
