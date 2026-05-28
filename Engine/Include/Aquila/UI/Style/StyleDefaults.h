#pragma once

#include "Aquila/UI/Style/StyleLength.h"

namespace Aquila::UI::StyleDefaults {

AQUILA_INLINE constexpr vec4 BackgroundColor = vec4(0, 0, 0, 0);
AQUILA_INLINE constexpr vec4 BorderColor = vec4(0, 0, 0, 0);
AQUILA_INLINE constexpr vec4 BorderRadius = vec4(0.f);
AQUILA_INLINE constexpr f32 BorderWidth = 0.f;
AQUILA_INLINE constexpr f32 Opacity = 1.f;

AQUILA_INLINE constexpr StyleLength Width = StyleLength::Auto();
AQUILA_INLINE constexpr StyleLength Height = StyleLength::Auto();

AQUILA_INLINE constexpr StyleLength Min = StyleLength::Pixel(0);
AQUILA_INLINE constexpr StyleLength Max = StyleLength::Grow();

AQUILA_INLINE constexpr StyleLength MinWidth = StyleLength::Pixel(0);
AQUILA_INLINE constexpr StyleLength MaxWidth = StyleLength::Grow();
AQUILA_INLINE constexpr StyleLength MinHeight = StyleLength::Pixel(0);
AQUILA_INLINE constexpr StyleLength MaxHeight = StyleLength::Grow();

AQUILA_INLINE constexpr StyleEdges Padding = StyleEdges::All(StyleLength::Pixel(0));

AQUILA_INLINE constexpr FlexDirection FlexDir = FlexDirection::Row;
AQUILA_INLINE constexpr JustifyContent Justify = JustifyContent::Start;
AQUILA_INLINE constexpr AlignItems Align = AlignItems::Stretch;
AQUILA_INLINE constexpr FlexWrap Wrap = FlexWrap::NoWrap;
AQUILA_INLINE constexpr f32 FlexGrow = 0.f;

AQUILA_INLINE constexpr Position Pos = Position::Static;

AQUILA_INLINE constexpr StyleLength Top = StyleLength::Auto();
AQUILA_INLINE constexpr StyleLength Bottom = StyleLength::Auto();
AQUILA_INLINE constexpr StyleLength Left = StyleLength::Auto();
AQUILA_INLINE constexpr StyleLength Right = StyleLength::Auto();

AQUILA_INLINE constexpr int ZIndex = 0;
AQUILA_INLINE constexpr Display Disp = Display::Flex;
AQUILA_INLINE constexpr Overflow Overflow = Overflow::Visible;
AQUILA_INLINE constexpr vec4 Color = vec4(1, 1, 1, 1);

AQUILA_INLINE constexpr f32 TransitionDuration = 0.f;
AQUILA_INLINE constexpr TransitionEasing TransitionEase = TransitionEasing::Ease;

AQUILA_INLINE constexpr f32 FontSize = 16.f;

} // namespace Aquila::UI::StyleDefaults
