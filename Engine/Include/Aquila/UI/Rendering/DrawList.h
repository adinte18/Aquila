#pragma once

#include "Aquila/Graphics/Core/QuadBatcher.h"
#include "Aquila/UI/Rendering/DrawCmd.h"
#include "Aquila/UI/Text/FontAtlas.h"
#include "Aquila/UI/Style/StyleTypes.h"

namespace Aquila::UI::Rendering {

class DrawList {
  public:
	void DrawRect(Rect rect, vec4 color, vec4 radius = vec4(0.F), float borderWidth = 0.F, vec4 borderColor = vec4(0.F),
				  int32 z = 0);
	void DrawShadow(Rect widgetRect, vec2 offset, float blur, float spread, vec4 color, vec4 radius, int32 z = 0);
	void DrawImage(Rect rect, GFX::GfxTexture *tex, vec4 tint = vec4(1.F), vec2 uvMin = vec2(0.F),
				   vec2 uvMax = vec2(1.F), int32 z = 0);
	void DrawText(Rect bounds, std::string_view text, Text::FontAtlas *font, vec4 color, float fontSize = 0.f,
				  TextAlign align = TextAlign::Left, int32 z = 0);
	void PushClip(Rect clipRect);
	void PopClip();

	void Sort();

	void Submit(Graphics::QuadBatcher &r2d, GFX::GfxCommandList &cmd);

	void AppendCmd(const DrawCmd &cmd);

	[[nodiscard]] std::vector<DrawCmd> TakeCommands();

	void Clear();
	[[nodiscard]] bool IsEmpty() const;

	// TODO : DELETE, debug only
	const std::vector<DrawCmd> &GetCommands() const { return m_Commands; }

	void SetCanvasSize(uint32 width, uint32 height);

  private:
	[[nodiscard]] Option<Rect> ActiveClip() const;

	std::vector<DrawCmd> m_Commands;
	std::vector<Rect> m_ClipStack;
	uint32 m_CanvasWidth = 0;
	uint32 m_CanvasHeight = 0;
};

} // namespace Aquila::UI::Rendering
