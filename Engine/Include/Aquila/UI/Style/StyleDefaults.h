#pragma once

#include "Aquila/UI/Style/StyleLength.h"

namespace Aquila::UI::StyleDefaults {

AQUILA_INLINE constexpr Vec4 BACKGROUND_COLOR = Vec4(0, 0, 0, 0);
AQUILA_INLINE constexpr Vec4 BORDER_COLOR = Vec4(0, 0, 0, 0);
AQUILA_INLINE constexpr Vec4 BORDER_RADIUS = Vec4(0.F);
AQUILA_INLINE constexpr F32 BORDER_WIDTH = 0.F;
AQUILA_INLINE constexpr BorderStyle BORDER_STYLE = BorderStyle::Solid;
AQUILA_INLINE constexpr F32 OPACITY = 1.F;

AQUILA_INLINE constexpr StyleLength WIDTH = StyleLength::Auto();
AQUILA_INLINE constexpr StyleLength HEIGHT = StyleLength::Auto();

AQUILA_INLINE constexpr StyleLength MIN = StyleLength::pixel(0);
AQUILA_INLINE constexpr StyleLength MAX = StyleLength::grow();

AQUILA_INLINE constexpr StyleLength MIN_WIDTH = StyleLength::pixel(0);
AQUILA_INLINE constexpr StyleLength MAX_WIDTH = StyleLength::grow();
AQUILA_INLINE constexpr StyleLength MIN_HEIGHT = StyleLength::pixel(0);
AQUILA_INLINE constexpr StyleLength MAX_HEIGHT = StyleLength::grow();

AQUILA_INLINE constexpr StyleEdges PADDING = StyleEdges::all(StyleLength::pixel(0));

AQUILA_INLINE constexpr FlexDirection FLEX_DIR = FlexDirection::Row;
AQUILA_INLINE constexpr JustifyContent JUSTIFY = JustifyContent::Start;
AQUILA_INLINE constexpr AlignItems ALIGN = AlignItems::Stretch;
AQUILA_INLINE constexpr FlexWrap WRAP = FlexWrap::NoWrap;
AQUILA_INLINE constexpr F32 FLEX_GROW = 0.F;

AQUILA_INLINE constexpr Position POS = Position::Static;

AQUILA_INLINE constexpr StyleLength TOP = StyleLength::Auto();
AQUILA_INLINE constexpr StyleLength BOTTOM = StyleLength::Auto();
AQUILA_INLINE constexpr StyleLength LEFT = StyleLength::Auto();
AQUILA_INLINE constexpr StyleLength RIGHT = StyleLength::Auto();

AQUILA_INLINE constexpr int Z_INDEX = 0;
AQUILA_INLINE constexpr Display DISP = Display::Flex;
AQUILA_INLINE constexpr Overflow OVERFLOW_DEFAULT = Overflow::Visible;
AQUILA_INLINE constexpr Vec4 COLOR = Vec4(1, 1, 1, 1);

AQUILA_INLINE constexpr F32 TRANSITION_DURATION = 0.F;
AQUILA_INLINE constexpr TransitionEasing TRANSITION_EASE = TransitionEasing::Ease;

AQUILA_INLINE constexpr F32 FONT_SIZE = 16.F;

} // namespace Aquila::UI::StyleDefaults
