#pragma once

#include "Aquila/Graphics/Core/QuadBatcher.h"
#include "Aquila/UI/Rendering/DrawCmd.h"

namespace Aquila::UI::Rendering {

class DrawList {
  public:
	void DrawRect(Rect rect, vec4 color, vec4 radius = vec4(0.F), float borderWidth = 0.F, vec4 borderColor = vec4(0.F), int32 z = 0);
	void DrawImage(Rect rect, GFX::GfxTexture *tex, vec4 tint = vec4(1.F), int32 z = 0);
	void PushClip(Rect clipRect);
	void PopClip();

	// Call before Submit. Stable sort by zOrder — preserves tree order within same z.
	void Sort();

	// Translates every DrawCmd into Renderer2D calls.
	// Must be called between Renderer2D::Begin and End.
	void Submit(Graphics::QuadBatcher &r2d, GFX::GfxCommandList &cmd);

	void Clear();
	[[nodiscard]] bool IsEmpty() const;

	// Must be kept in sync with the owning Canvas size.
	// Used to restore full-screen scissor when the clip stack empties.
	void SetCanvasSize(uint32 width, uint32 height);

  private:
	[[nodiscard]] Option<Rect> ActiveClip() const;

	std::vector<DrawCmd> m_Commands;
	std::vector<Rect>    m_ClipStack;
	uint32               m_CanvasWidth  = 0;
	uint32               m_CanvasHeight = 0;
};

} // namespace Aquila::UI::Rendering
