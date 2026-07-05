#pragma once

#include "Aquila/UI/Core/View.h"
#include <vector>

namespace Aquila::UI::Core {

class LayoutEngine {
  public:
	LayoutEngine(uint32 width, uint32 height);

	void SetDimensions(uint32 width, uint32 height);

	void RunLayout(View *root, vec2 mousePos, bool mouseDown, vec2 scrollDelta, float deltaTime);

	// Adjusts the scroll offset of target's nearest scrolling ancestor so target is visible.
	// Must be called after RunLayout so rects and Clay scroll state are current.
	void ScrollIntoView(View *target);

  private:
	void LayoutPass(View *node);
	void UpdateRects(View *node, vec2 parentAbsPos = {});

	void *m_ClayCtx = nullptr;
	std::vector<uint8> m_ClayMemory;
	uint32 m_Width;
	uint32 m_Height;
};

} // namespace Aquila::UI::Core
