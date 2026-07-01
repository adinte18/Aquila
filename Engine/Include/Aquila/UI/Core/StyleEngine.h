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
		bool layoutAffected = false; // a layout-relevant property changed
	};

	StyleSheet &GetStyleSheet() { return m_StyleSheet; }

	void Invalidate(View *view);
	void Remove(View *view);
	[[nodiscard]] bool HasPending() const { return !m_DirtyViews.IsEmpty(); }

	ResolveResult Resolve(uint32 viewportWidth, uint32 viewportHeight, bool layoutAlreadyDirty);

  private:
	StyleSheet m_StyleSheet;
	Foundation::DirtySet<View *> m_DirtyViews;
	Foundation::ComputedCache<View *, ComputedStyle> m_StyleCache;
};

} // namespace Aquila::UI::Core
