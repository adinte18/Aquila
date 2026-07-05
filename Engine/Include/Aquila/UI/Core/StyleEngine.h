#pragma once

#include "Aquila/Foundation/Cache/ComputedCache.h"
#include "Aquila/Foundation/Invalidation/DirtySet.h"
#include "Aquila/UI/Core/View.h"
#include "Aquila/UI/Style/StyleSheet.h"

namespace Aquila::UI::Core {

class StyleEngine {
  public:
	struct ResolveResult {
		bool changed = false;        // at least one view's computed style changed
		bool layout_affected = false; // a layout-relevant property changed
	};

	StyleSheet &get_style_sheet() { return m_style_sheet; }

	void invalidate(View *view);
	void remove(View *view);
	[[nodiscard]] bool has_pending() const { return !m_dirty_views.is_empty(); }

	ResolveResult resolve(Uint32 viewport_width, Uint32 viewport_height, bool layout_already_dirty);

  private:
	StyleSheet m_style_sheet;
	Foundation::DirtySet<View *> m_dirty_views;
	Foundation::ComputedCache<View *, ComputedStyle> m_style_cache;
};

} // namespace Aquila::UI::Core
