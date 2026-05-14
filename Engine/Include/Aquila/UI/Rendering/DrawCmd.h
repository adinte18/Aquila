#pragma once

#include "Aquila/Foundation/Math/Rect.h"
#include "Aquila/Foundation/PrimitiveTypes.h"

#include "Aquila/GFX/GfxTexture.h"

#include <string>

namespace Aquila::UI::Text {
class FontAtlas;
}

namespace Aquila::UI::Rendering {
enum class UICommandType : uint8 { Rect, Shadow, Image, Text, ClipPush, ClipPop };

struct DrawCmd {
	UICommandType type;
	Rect rect;
	vec4 color = vec4(1.F);
	vec4 radius = vec4(0.F);
	float borderWidth = 0.F;
	vec4 borderColor = vec4(0.F);
	GFX::GfxTexture *texture = nullptr;
	vec4 textureTint = vec4(0.F);
	vec2 uvMin = { 0.F, 0.F };
	vec2 uvMax = { 1.F, 1.F };
	int32 zOrder = 0;

	// Text
	std::string text;
	Text::FontAtlas *font = nullptr;
	float fontSize = 0.f;

	// Shadow (UICommandType::Shadow)
	// rect = widget rect (used to compute SDF parameters)
	// color = shadow color
	// borderColor.xy = shadow offset
	// borderWidth = blur radius
	// borderColor.zw = (widget.w/2 + spread, widget.h/2 + spread)
	// radius = widget corner radii
};
} // namespace Aquila::UI::Rendering
