#pragma once

#include "Aquila/UI/Core/View.h"
#include <vector>

namespace Aquila::UI::Core {

class LayoutEngine {
  public:
	LayoutEngine(Uint32 width, Uint32 height);

	void set_dimensions(Uint32 width, Uint32 height);

	void run_layout(View *root, Vec2 mouse_pos, bool mouse_down, Vec2 scroll_delta, float delta_time);

	// Adjusts the scroll offset of target's nearest scrolling ancestor so target is visible.
	// Must be called after RunLayout so rects and Clay scroll state are current.
	void scroll_into_view(View *target);

	[[nodiscard]] bool did_layout_resize() const { return m_size_changed; }

  private:
	void layout_pass(View *node);
	void update_rects(View *node, Vec2 parent_clay_pos = {}, Vec2 accumulated_offset = {});

	void *m_clay_ctx = nullptr;
	std::vector<Uint8> m_clay_memory;
	Uint32 m_width;
	Uint32 m_height;
	bool m_size_changed = false;
};

} // namespace Aquila::UI::Core
