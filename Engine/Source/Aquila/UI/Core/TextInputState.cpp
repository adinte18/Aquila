#include "Aquila/UI/Core/TextInputState.h"
#include "Aquila/Application/Events/InputEvent.h"
#include "Aquila/Foundation/Text/Utf8.h"
#include "Aquila/UI/Core/Clipboard.h"

namespace Aquila::UI::Core {

using KeyCode = Platform::KeyCode;
using namespace Application::Events;

bool TextInputState::handle_key_press(Platform::KeyCode key, int mods) {
	const bool ctrl = (mods & MODIFIER_CONTROL) != 0;
	const bool shift = (mods & MODIFIER_SHIFT) != 0;

	if (ctrl) {
		switch (key) {
		case KeyCode::A:
			select_all();
			return true;
		case KeyCode::C:
			if (has_selection()) {
				Clipboard::set(text.substr(selection_min(), selection_max() - selection_min()));
			}
			return true;
		case KeyCode::X:
			if (has_selection()) {
				Clipboard::set(text.substr(selection_min(), selection_max() - selection_min()));
				delete_selection();
			}
			return true;
		case KeyCode::V: {
			const std::string clip = Clipboard::get();
			if (!clip.empty()) {
				if (has_selection()) {
					delete_selection();
				}
				text.insert(cursor, clip);
				cursor += clip.size();
				select_anchor = cursor;
			}
			return true;
		}
		case KeyCode::Backspace:
			text.clear();
			cursor = 0;
			select_anchor = 0;
			return true;
		default:
			return false;
		}
	}

	switch (key) {
	case KeyCode::Backspace:
		if (has_selection()) {
			delete_selection();
		} else if (cursor > 0) {
			const size_t prev = Foundation::Utf8::prev_index(text, cursor);
			text.erase(prev, cursor - prev);
			cursor = prev;
			select_anchor = cursor;
		}
		return true;
	case KeyCode::Delete:
		if (has_selection()) {
			delete_selection();
		} else if (cursor < text.size()) {
			const size_t next = Foundation::Utf8::next_index(text, cursor);
			text.erase(cursor, next - cursor);
		}
		return true;
	case KeyCode::Left:
		if (has_selection() && !shift) {
			cursor = selection_min();
			select_anchor = cursor;
		} else if (cursor > 0) {
			cursor = Foundation::Utf8::prev_index(text, cursor);
			if (!shift) {
				select_anchor = cursor;
			}
		}
		return true;
	case KeyCode::Right:
		if (has_selection() && !shift) {
			cursor = selection_max();
			select_anchor = cursor;
		} else if (cursor < text.size()) {
			cursor = Foundation::Utf8::next_index(text, cursor);
			if (!shift) {
				select_anchor = cursor;
			}
		}
		return true;
	case KeyCode::Home:
		cursor = 0;
		if (!shift) {
			select_anchor = cursor;
		}
		return true;
	case KeyCode::End:
		cursor = text.size();
		if (!shift) {
			select_anchor = cursor;
		}
		return true;
	case KeyCode::Escape:
		select_anchor = cursor;
		return true;
	default:
		return false;
	}
}

bool TextInputState::handle_char_input(Uint32 codepoint) {
	if (codepoint < 32 || codepoint == 127) {
		return false;
	}
	if (has_selection()) {
		delete_selection();
	}
	char encoded[4];
	const Uint32 count = Foundation::Utf8::encode(codepoint, encoded);
	text.insert(cursor, encoded, count);
	cursor += count;
	select_anchor = cursor;
	return true;
}

float TextInputState::measure_to_pos(const Text::FontAtlas &font, float scale, size_t pos) const {
	float x = 0.F;
	const size_t end = std::min(pos, text.size());
	for (size_t i = 0; i < end;) {
		const Foundation::Utf8::Decoded d = Foundation::Utf8::decode(text, i);
		const Text::GlyphInfo *g = font.get_glyph(d.codepoint);
		if (g) {
			x += g->advance * scale;
		}
		i += (d.size > 0 ? d.size : 1u);
	}
	return x;
}

size_t TextInputState::hit_test_pos(const Text::FontAtlas &font, float scale, float local_x) const {
	float acc = 0.F;
	for (size_t i = 0; i < text.size();) {
		const Foundation::Utf8::Decoded d = Foundation::Utf8::decode(text, i);
		const size_t step = (d.size > 0 ? d.size : 1u);
		const Text::GlyphInfo *g = font.get_glyph(d.codepoint);
		if (g) {
			if (local_x < acc + g->advance * scale * 0.5f) {
				return i;
			}
			acc += g->advance * scale;
		}
		i += step;
	}
	return text.size();
}

} // namespace Aquila::UI::Core
