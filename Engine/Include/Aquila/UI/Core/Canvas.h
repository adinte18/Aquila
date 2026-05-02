#pragma once

#include "Aquila/UI/Core/View.h"
#include "Aquila/UI/Style/StyleSheet.h"

namespace Aquila::UI::Core {

using namespace Aquila::UI::Rendering;

class Canvas {
  public:
	Canvas(uint32 width, uint32 height);

	void OnEvent(Application::Events::Event &event);
	void Update(float deltaTime);
	void Render(Graphics::QuadBatcher &r2d, GFX::GfxCommandList &cmd);
	void Resize(uint32 width, uint32 height);

	StyleSheet &GetStyleSheet();
	View *GetRoot();

  private:
	void StylePass(View *node, const ComputedStyle *parentComputed);
	void ClayLayoutPass(View *node);
	void ClayUpdateRects(View *node, vec2 parentAbsPos = {});
	void DrawPass(View *node, vec2 origin);

	Unique<View> m_Root;
	StyleSheet m_StyleSheet;
	DrawList m_DrawList;
	View *m_HoveredView = nullptr;
	View *m_FocusedView = nullptr;
	uint32 m_Width, m_Height;

	void *m_ClayCtx = nullptr;
	std::vector<uint8_t> m_ClayMemory;
};
} // namespace Aquila::UI::Core
