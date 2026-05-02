#pragma once

#include "Aquila/Foundation/PrimitiveTypes.h"

namespace Aquila::UI {

enum class Type : uint8 { Pixel, Percent, Auto, Grow };

enum class FlexDirection : uint8 { Row, Column, RowReverse, ColumnReverse };
enum class JustifyContent : uint8 { Start, End, Center, SpaceBetween, SpaceAround, SpaceEvenly };
enum class AlignItems : uint8 { Start, End, Center, Stretch };
enum class FlexWrap : uint8 { NoWrap, Wrap };

enum class DisplayType : uint8 { Flex, None };
enum class OverflowType : uint8 { Visible, Hidden, Scroll };

enum class PositionType : uint8 { Static, Relative, Absolute };
} // namespace Aquila::UI
