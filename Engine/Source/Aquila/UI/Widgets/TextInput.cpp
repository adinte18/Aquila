#include "Aquila/UI/Widgets/TextInput.h"
#include "Aquila/UI/Core/Clipboard.h"
#include "Aquila/UI/Core/FontRegistry.h"
#include "Aquila/Application/Events/InputEvent.h"
#include "Aquila/Platform/Input.h"

namespace Aquila::UI::Core {

using KeyCode = Platform::KeyCode;

TextInput::TextInput() {
	SetCapturesInput(true);
}

TextInput::TextInput(std::string placeholder) : m_Placeholder(std::move(placeholder)) {
	SetCapturesInput(true);
}

void TextInput::SetText(const std::string &text) {
	m_Text = text;
	m_Cursor = m_Text.size();
	m_SelectAnchor = m_Cursor;
	QueueRedraw();
}

void TextInput::SetFont(Text::FontAtlas *font) {
	m_Font = font;
	QueueRedraw();
}

void TextInput::SetPlaceholder(std::string text) {
	m_Placeholder = std::move(text);
	QueueRedraw();
}

void TextInput::SetOnChanged(Delegate<void(const std::string &)> callback) {
	m_OnChanged = std::move(callback);
}

void TextInput::SetOnSubmit(Delegate<void(const std::string &)> callback) {
	m_OnSubmit = std::move(callback);
}

Text::FontAtlas *TextInput::ResolveFont() const {
	const std::string &family = GetComputedStyle().fontFamily;
	if (!family.empty()) {
		if (Text::FontAtlas *resolved = FontRegistry::Resolve(family)) {
			return resolved;
		}
	}
	return m_Font;
}

float TextInput::MeasureTextX(Text::FontAtlas *font, float scale, size_t pos) const {
	float x = 0.f;
	const size_t end = std::min(pos, m_Text.size());
	for (size_t i = 0; i < end; ++i) {
		const Text::GlyphInfo *g = font->GetGlyph(static_cast<uint32>(static_cast<unsigned char>(m_Text[i])));
		if (g) {
			x += g->advance * scale;
		}
	}
	return x;
}

size_t TextInput::HitTestPos(float localX) const {
	Text::FontAtlas *font = ResolveFont();
	if (!font) {
		return m_Text.size();
	}

	const float fontSize = GetDisplayStyle().fontSize;
	const float bakeSize = font->GetBakeSize();
	const float scale = (bakeSize > 0.f && fontSize > 0.f) ? (fontSize / bakeSize) : 1.f;
	constexpr float kPadX = 4.f;
	const float x = localX - kPadX;

	float acc = 0.f;
	for (size_t i = 0; i < m_Text.size(); ++i) {
		const Text::GlyphInfo *g = font->GetGlyph(static_cast<uint32>(static_cast<unsigned char>(m_Text[i])));
		if (!g) {
			continue;
		}
		if (x < acc + g->advance * scale * 0.5f) {
			return i;
		}
		acc += g->advance * scale;
	}
	return m_Text.size();
}

void TextInput::DeleteSelection() {
	const size_t lo = SelectionMin();
	const size_t hi = SelectionMax();
	m_Text.erase(lo, hi - lo);
	m_Cursor = lo;
	m_SelectAnchor = lo;
}

void TextInput::SelectAll() {
	m_SelectAnchor = 0;
	m_Cursor = m_Text.size();
}

void TextInput::OnMousePress(Platform::MouseButton btn, vec2 pos) {
	View::OnMousePress(btn, pos);
	if (btn != Platform::MouseButton::Left) {
		return;
	}

	const float localX = pos.x - GetAbsolutePosition().x;
	const size_t hit = HitTestPos(localX);
	const bool shift =
		Platform::Input::IsKeyPressed(KeyCode::LeftShift) || Platform::Input::IsKeyPressed(KeyCode::RightShift);

	if (shift) {
		m_Cursor = hit; // extend selection, keep anchor
	} else {
		m_Cursor = hit;
		m_SelectAnchor = hit;
	}
	QueueRedraw();
}

void TextInput::OnMouseMove(vec2 pos) {
	if (!m_IsPressed) {
		return;
	}
	const float localX = pos.x - GetAbsolutePosition().x;
	const size_t hit = HitTestPos(localX);
	if (hit != m_Cursor) {
		m_Cursor = hit;
		QueueRedraw();
	}
}

void TextInput::OnKeyPress(Platform::KeyCode key, int mods) {
	using namespace Application::Events;
	const bool ctrl = (mods & ModControl) != 0;
	const bool shift = (mods & ModShift) != 0;

	if (ctrl) {
		switch (key) {
		case KeyCode::A:
			SelectAll();
			QueueRedraw();
			break;
		case KeyCode::C:
			if (HasSelection()) {
				Clipboard::Set(m_Text.substr(SelectionMin(), SelectionMax() - SelectionMin()));
			}
			break;
		case KeyCode::X:
			if (HasSelection()) {
				Clipboard::Set(m_Text.substr(SelectionMin(), SelectionMax() - SelectionMin()));
				DeleteSelection();
				if (m_OnChanged) {
					m_OnChanged(m_Text);
				}
				QueueRedraw();
			}
			break;
		case KeyCode::V: {
			const std::string clip = Clipboard::Get();
			if (!clip.empty()) {
				if (HasSelection()) {
					DeleteSelection();
				}
				m_Text.insert(m_Cursor, clip);
				m_Cursor += clip.size();
				m_SelectAnchor = m_Cursor;
				if (m_OnChanged) {
					m_OnChanged(m_Text);
				}
				QueueRedraw();
			}
			break;
		}
		case KeyCode::Backspace:
			m_Text.clear();
			m_Cursor = 0;
			m_SelectAnchor = 0;
			if (m_OnChanged) {
				m_OnChanged(m_Text);
			}
			QueueRedraw();
			break;
		default:
			break;
		}
		return;
	}

	switch (key) {
	case KeyCode::Backspace:
		if (HasSelection()) {
			DeleteSelection();
			if (m_OnChanged) {
				m_OnChanged(m_Text);
			}
			QueueRedraw();
		} else if (m_Cursor > 0) {
			m_Text.erase(m_Cursor - 1, 1);
			--m_Cursor;
			m_SelectAnchor = m_Cursor;
			if (m_OnChanged) {
				m_OnChanged(m_Text);
			}
			QueueRedraw();
		}
		break;
	case KeyCode::Delete:
		if (HasSelection()) {
			DeleteSelection();
			if (m_OnChanged) {
				m_OnChanged(m_Text);
			}
			QueueRedraw();
		} else if (m_Cursor < m_Text.size()) {
			m_Text.erase(m_Cursor, 1);
			if (m_OnChanged) {
				m_OnChanged(m_Text);
			}
			QueueRedraw();
		}
		break;
	case KeyCode::Left:
		if (HasSelection() && !shift) {
			m_Cursor = SelectionMin();
			m_SelectAnchor = m_Cursor;
		} else if (m_Cursor > 0) {
			--m_Cursor;
			if (!shift) {
				m_SelectAnchor = m_Cursor;
			}
		}
		QueueRedraw();
		break;
	case KeyCode::Right:
		if (HasSelection() && !shift) {
			m_Cursor = SelectionMax();
			m_SelectAnchor = m_Cursor;
		} else if (m_Cursor < m_Text.size()) {
			++m_Cursor;
			if (!shift) {
				m_SelectAnchor = m_Cursor;
			}
		}
		QueueRedraw();
		break;
	case KeyCode::Enter:
		if (m_OnSubmit) {
			m_OnSubmit(m_Text);
		}
		break;
	case KeyCode::Escape:
		m_SelectAnchor = m_Cursor;
		QueueRedraw();
		break;
	default:
		break;
	}
}

void TextInput::OnCharInput(uint32 codepoint) {
	if (codepoint < 32 || codepoint > 126) {
		return;
	}
	if (HasSelection()) {
		DeleteSelection();
	}
	const char ch = static_cast<char>(codepoint);
	m_Text.insert(m_Cursor, 1, ch);
	++m_Cursor;
	m_SelectAnchor = m_Cursor;
	if (m_OnChanged) {
		m_OnChanged(m_Text);
	}
	QueueRedraw();
}

void TextInput::OnFocusGained() {
	View::OnFocusGained();
	QueueRedraw();
}

void TextInput::OnFocusLost() {
	View::OnFocusLost();
	m_SelectAnchor = m_Cursor; // clear selection on blur
	QueueRedraw();
}

void TextInput::OnDrawSelf(Rendering::DrawList &drawList) {
	View::OnDrawSelf(drawList);

	const Rect rect = { .position = GetAbsolutePosition(), .size = GetLayoutRect().size };
	const auto &style = GetDisplayStyle();
	const int32 z = style.zIndex * 4;
	const float fontSize = style.fontSize > 0.f ? style.fontSize : 14.f;

	Text::FontAtlas *font = ResolveFont();

	const bool showPlaceholder = m_Text.empty() && !m_IsFocused;
	const bool hasText = !m_Text.empty();

	if (font) {
		const float bakeSize = font->GetBakeSize();
		const float scale = (bakeSize > 0.f) ? (fontSize / bakeSize) : 1.f;
		const float lineH = font->GetLineHeight() * scale;
		const float textY = rect.position.y + (rect.size.y - lineH) * 0.5f;
		constexpr float kPadX = 4.f;
		const Rect textRect = {
			.position = { rect.position.x + kPadX, textY },
			.size = { rect.size.x - kPadX * 2.f, lineH },
		};

		// Selection highlight (behind text)
		if (m_IsFocused && HasSelection()) {
			const float selX0 = textRect.position.x + MeasureTextX(font, scale, SelectionMin());
			const float selX1 = textRect.position.x + MeasureTextX(font, scale, SelectionMax());
			const Rect selRect = {
				.position = { selX0, textY },
				.size = { selX1 - selX0, lineH },
			};
			const vec4 selColor = vec4(style.color.r, style.color.g, style.color.b, 0.3f);
			drawList.DrawRect(selRect, selColor, vec4(2.f), 0.f, vec4(0.f), z + 2);
		}

		if (hasText) {
			drawList.DrawText(textRect, m_Text, font, style.color, fontSize, TextAlign::Left, z + 3);
		} else if (showPlaceholder) {
			const vec4 muted = vec4(style.color.r, style.color.g, style.color.b, style.color.a * 0.45f);
			drawList.DrawText(textRect, m_Placeholder, font, muted, fontSize, TextAlign::Left, z + 3);
		}

		// Cursor (only when no selection, or always show at cursor end)
		if (m_IsFocused && !HasSelection()) {
			const float cx = textRect.position.x + MeasureTextX(font, scale, m_Cursor);
			const float cy = rect.position.y + (rect.size.y - lineH) * 0.5f;
			const Rect cursor = {
				.position = { cx - 0.75f, cy },
				.size = { 1.5f, lineH },
			};
			drawList.DrawRect(cursor, style.color, vec4(0.f), 0.f, vec4(0.f), z + 3);
		}
	}
}

} // namespace Aquila::UI::Core
