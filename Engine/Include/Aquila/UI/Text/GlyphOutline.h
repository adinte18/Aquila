#pragma once

#include "Aquila/Foundation/Math/Geometry/Bezier.h"

namespace Aquila::UI::Text {

struct GlyphOutline {
	std::vector<Math::Geometry::Bezier::QuadraticBezier> curves;
	Vec2 em_min{ 0.F, 0.F };
	Vec2 em_max{ 0.F, 0.F };
};

} // namespace Aquila::UI::Text
