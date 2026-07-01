#pragma once

#include "Aquila/Foundation/Math/Rect.h"
#include "Aquila/Foundation/PrimitiveTypes.h"
#include "Aquila/UI/Style/StyleTypes.h"

#include "Aquila/GFX/GfxTexture.h"

#include <string>
#include <variant>

namespace Aquila::UI::Text {
class FontAtlas;
}

namespace Aquila::UI::Rendering {

struct RectCmd {
	Rect rect;
	vec4 color = vec4(1.f);
	vec4 radius = vec4(0.f);
	float borderWidth = 0.f;
	vec4 borderColor = vec4(0.f);
	float rotation = 0.f;
	int32 zOrder = 0;
};

struct ShadowCmd {
	Rect rect; // the widget rect the shadow is cast from
	vec4 color = vec4(0.f);
	vec4 radius = vec4(0.f);
	vec2 offset = { 0.f, 0.f };
	vec2 originalHalfSize = { 0.f, 0.f }; // widget half-size + spread, drives the SDF
	float blur = 0.f;
	int32 zOrder = 0;
};

struct ImageCmd {
	Rect rect;
	GFX::GfxTexture *texture = nullptr;
	vec4 tint = vec4(0.f);
	vec2 uvMin = { 0.f, 0.f };
	vec2 uvMax = { 1.f, 1.f };
	int32 zOrder = 0;
};

struct TextCmd {
	Rect rect; // layout bounds for alignment
	vec4 color = vec4(1.f);
	std::string text;
	Text::FontAtlas *font = nullptr;
	float fontSize = 0.f;
	TextAlign align = TextAlign::Left;
	int32 zOrder = 0;
};

struct ClipPushCmd {
	Rect rect;
	int32 zOrder = 0;
};

struct ClipPopCmd {
	Rect rect; // rect to restore to; empty means the full canvas
	int32 zOrder = 0;
};

using DrawCmd = std::variant<RectCmd, ShadowCmd, ImageCmd, TextCmd, ClipPushCmd, ClipPopCmd>;

[[nodiscard]] inline int32 DrawCmdZOrder(const DrawCmd &cmd) {
	return std::visit([](const auto &c) { return c.zOrder; }, cmd);
}

} // namespace Aquila::UI::Rendering
