#pragma once

#include "Aquila/Foundation/Math/Rect.h"
#include "Aquila/Foundation/PrimitiveTypes.h"

#include "Aquila/GFX/GfxTexture.h"

#include <string>

namespace Aquila::UI::Text {
class FontAtlas;
}

namespace Aquila::UI::Rendering {
enum class UICommandType : uint8 { Rect, Image, Text, ClipPush, ClipPop };

struct DrawCmd {
	UICommandType type;
	Rect rect;
	vec4 color = vec4(1.F);
	vec4 radius = vec4(0.F);
	float borderWidth = 0.F;
	vec4 borderColor = vec4(0.F);
	GFX::GfxTexture *texture = nullptr;
	vec4 textureTint = vec4(0.F);
	int32 zOrder = 0;

	std::string text;
	Text::FontAtlas *font = nullptr;
};
} // namespace Aquila::UI::Rendering
