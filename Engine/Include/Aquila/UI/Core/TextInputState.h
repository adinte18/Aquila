#pragma once

#include "Aquila/Platform/Input.h"
#include "Aquila/UI/Text/FontAtlas.h"
#include <string>

namespace Aquila::UI::Core {

struct TextInputState {
	std::string text;
	size_t cursor = 0;
	size_t selectAnchor = 0;

	[[nodiscard]] bool HasSelection() const { return cursor != selectAnchor; }
	[[nodiscard]] size_t SelectionMin() const { return std::min(cursor, selectAnchor); }
	[[nodiscard]] size_t SelectionMax() const { return std::max(cursor, selectAnchor); }

	void SetText(std::string t) {
		text = std::move(t);
		cursor = text.size();
		selectAnchor = cursor;
	}

	void SelectAll() {
		selectAnchor = 0;
		cursor = text.size();
	}

	void DeleteSelection() {
		const size_t lo = SelectionMin();
		const size_t hi = SelectionMax();
		text.erase(lo, hi - lo);
		cursor = lo;
		selectAnchor = lo;
	}

	bool HandleKeyPress(Platform::KeyCode key, int mods);
	bool HandleCharInput(uint32 codepoint);

	[[nodiscard]] float MeasureToPos(const Text::FontAtlas &font, float scale, size_t pos) const;

	[[nodiscard]] size_t HitTestPos(const Text::FontAtlas &font, float scale, float localX) const;
};

} // namespace Aquila::UI::Core
