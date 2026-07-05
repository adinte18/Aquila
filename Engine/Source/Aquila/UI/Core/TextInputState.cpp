#include "Aquila/UI/Core/TextInputState.h"
#include "Aquila/Application/Events/InputEvent.h"
#include "Aquila/Foundation/Text/Utf8.h"
#include "Aquila/UI/Core/Clipboard.h"

namespace Aquila::UI::Core {

using KeyCode = Platform::KeyCode;
using namespace Application::Events;

bool TextInputState::HandleKeyPress(Platform::KeyCode key, int mods) {
	const bool ctrl = (mods & ModControl) != 0;
	const bool shift = (mods & ModShift) != 0;

	if (ctrl) {
		switch (key) {
		case KeyCode::A:
			SelectAll();
			return true;
		case KeyCode::C:
			if (HasSelection()) {
				Clipboard::Set(text.substr(SelectionMin(), SelectionMax() - SelectionMin()));
			}
			return true;
		case KeyCode::X:
			if (HasSelection()) {
				Clipboard::Set(text.substr(SelectionMin(), SelectionMax() - SelectionMin()));
				DeleteSelection();
			}
			return true;
		case KeyCode::V: {
			const std::string clip = Clipboard::Get();
			if (!clip.empty()) {
				if (HasSelection()) {
					DeleteSelection();
				}
				text.insert(cursor, clip);
				cursor += clip.size();
				selectAnchor = cursor;
			}
			return true;
		}
		case KeyCode::Backspace:
			text.clear();
			cursor = 0;
			selectAnchor = 0;
			return true;
		default:
			return false;
		}
	}

	switch (key) {
	case KeyCode::Backspace:
		if (HasSelection()) {
			DeleteSelection();
		} else if (cursor > 0) {
			const size_t prev = Foundation::Utf8::PrevIndex(text, cursor);
			text.erase(prev, cursor - prev);
			cursor = prev;
			selectAnchor = cursor;
		}
		return true;
	case KeyCode::Delete:
		if (HasSelection()) {
			DeleteSelection();
		} else if (cursor < text.size()) {
			const size_t next = Foundation::Utf8::NextIndex(text, cursor);
			text.erase(cursor, next - cursor);
		}
		return true;
	case KeyCode::Left:
		if (HasSelection() && !shift) {
			cursor = SelectionMin();
			selectAnchor = cursor;
		} else if (cursor > 0) {
			cursor = Foundation::Utf8::PrevIndex(text, cursor);
			if (!shift) {
				selectAnchor = cursor;
			}
		}
		return true;
	case KeyCode::Right:
		if (HasSelection() && !shift) {
			cursor = SelectionMax();
			selectAnchor = cursor;
		} else if (cursor < text.size()) {
			cursor = Foundation::Utf8::NextIndex(text, cursor);
			if (!shift) {
				selectAnchor = cursor;
			}
		}
		return true;
	case KeyCode::Home:
		cursor = 0;
		if (!shift) {
			selectAnchor = cursor;
		}
		return true;
	case KeyCode::End:
		cursor = text.size();
		if (!shift) {
			selectAnchor = cursor;
		}
		return true;
	case KeyCode::Escape:
		selectAnchor = cursor;
		return true;
	default:
		return false;
	}
}

bool TextInputState::HandleCharInput(uint32 codepoint) {
	if (codepoint < 32 || codepoint == 127) {
		return false;
	}
	if (HasSelection()) {
		DeleteSelection();
	}
	char encoded[4];
	const uint32 count = Foundation::Utf8::Encode(codepoint, encoded);
	text.insert(cursor, encoded, count);
	cursor += count;
	selectAnchor = cursor;
	return true;
}

float TextInputState::MeasureToPos(const Text::FontAtlas &font, float scale, size_t pos) const {
	float x = 0.f;
	const size_t end = std::min(pos, text.size());
	for (size_t i = 0; i < end;) {
		const Foundation::Utf8::Decoded d = Foundation::Utf8::Decode(text, i);
		const Text::GlyphInfo *g = font.GetGlyph(d.codepoint);
		if (g) {
			x += g->advance * scale;
		}
		i += (d.size > 0 ? d.size : 1u);
	}
	return x;
}

size_t TextInputState::HitTestPos(const Text::FontAtlas &font, float scale, float localX) const {
	float acc = 0.f;
	for (size_t i = 0; i < text.size();) {
		const Foundation::Utf8::Decoded d = Foundation::Utf8::Decode(text, i);
		const size_t step = (d.size > 0 ? d.size : 1u);
		const Text::GlyphInfo *g = font.GetGlyph(d.codepoint);
		if (g) {
			if (localX < acc + g->advance * scale * 0.5f) {
				return i;
			}
			acc += g->advance * scale;
		}
		i += step;
	}
	return text.size();
}

} // namespace Aquila::UI::Core
