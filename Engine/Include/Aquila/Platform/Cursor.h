#pragma once

#include "Aquila/Foundation/PrimitiveTypes.h"

namespace Aquila::Platform {

enum class CursorType : Uint8 {
	Arrow,
	Text,
	Hand,
	Crosshair,
	ResizeHorizontal,
	ResizeVertical,
	ResizeDiagonalTLBR,
	ResizeDiagonalBLTR,
	ResizeAll,
	NotAllowed,
	Count
};

} // namespace Aquila::Platform
