#include "Aquila/UI/Core/TextInputState.h"
#include "Aquila/Application/Events/InputEvent.h"
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
			text.erase(cursor - 1, 1);
			--cursor;
			selectAnchor = cursor;
		}
		return true;
	case KeyCode::Delete:
		if (HasSelection()) {
			DeleteSelection();
		} else if (cursor < text.size()) {
			text.erase(cursor, 1);
		}
		return true;
	case KeyCode::Left:
		if (HasSelection() && !shift) {
			cursor = SelectionMin();
			selectAnchor = cursor;
		} else if (cursor > 0) {
			--cursor;
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
			++cursor;
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
	if (codepoint < 32 || codepoint > 126) {
		return false;
	}
	if (HasSelection()) {
		DeleteSelection();
	}
	text.insert(cursor, 1, static_cast<char>(codepoint));
	++cursor;
	selectAnchor = cursor;
	return true;
}

float TextInputState::MeasureToPos(const Text::FontAtlas &font, float scale, size_t pos) const {
	float x = 0.f;
	const size_t end = std::min(pos, text.size());
	for (size_t i = 0; i < end; ++i) {
		const Text::GlyphInfo *g = font.GetGlyph(static_cast<uint32>(static_cast<unsigned char>(text[i])));
		if (g) {
			x += g->advance * scale;
		}
	}
	return x;
}

size_t TextInputState::HitTestPos(const Text::FontAtlas &font, float scale, float localX) const {
	float acc = 0.f;
	for (size_t i = 0; i < text.size(); ++i) {
		const Text::GlyphInfo *g = font.GetGlyph(static_cast<uint32>(static_cast<unsigned char>(text[i])));
		if (!g) {
			continue;
		}
		if (localX < acc + g->advance * scale * 0.5f) {
			return i;
		}
		acc += g->advance * scale;
	}
	return text.size();
}

} // namespace Aquila::UI::Core
