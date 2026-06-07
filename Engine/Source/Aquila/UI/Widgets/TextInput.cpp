#include "Aquila/UI/Widgets/TextInput.h"
#include "Aquila/UI/Rendering/DrawCmd.h"
#include "Aquila/Application/Events/InputEvent.h"
#include "Aquila/Platform/Input.h"

namespace Aquila::UI::Core {

static void InitTextInput(TextInput *self) {
	self->SetInputLeaf(true);

	StyleProperties sp;
	sp.overflow = Overflow::Hidden;
	self->SetStyle(sp);
}

TextInput::TextInput() {
	InitTextInput(this);
}

TextInput::TextInput(std::string placeholder) : m_Placeholder(std::move(placeholder)) {
	InitTextInput(this);
}

void TextInput::SetText(const std::string &text) {
	m_State.SetText(text);
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
	if (Text::FontAtlas *css = GetResolvedFont()) {
		return css;
	}
	return m_Font;
}

void TextInput::ClampScrollOffset(Text::FontAtlas *font, float scale, float visibleWidth) {
	const float cursorX = m_State.MeasureToPos(*font, scale, m_State.cursor);
	if (cursorX - m_ScrollOffsetX < 0.f) {
		m_ScrollOffsetX = cursorX;
	} else if (cursorX - m_ScrollOffsetX > visibleWidth) {
		m_ScrollOffsetX = cursorX - visibleWidth;
	}
	m_ScrollOffsetX = std::max(0.f, m_ScrollOffsetX);
}

void TextInput::OnMousePress(Platform::MouseButton btn, vec2 pos) {
	View::OnMousePress(btn, pos);
	if (btn != Platform::MouseButton::Left) {
		return;
	}

	Text::FontAtlas *font = ResolveFont();
	if (!font) {
		return;
	}

	const float fontSize = GetDisplayStyle().fontSize;
	const float bakeSize = font->GetBakeSize();
	const float scale = (bakeSize > 0.f && fontSize > 0.f) ? (fontSize / bakeSize) : 1.f;

	constexpr float kPadX = 4.f;
	const float visibleWidth = GetLayoutRect().size.x - kPadX * 2.f;
	const float localX = pos.x - GetAbsolutePosition().x - kPadX + m_ScrollOffsetX;
	const size_t hit = m_State.HitTestPos(*font, scale, localX);

	const bool shift = Platform::Input::IsKeyPressed(Platform::KeyCode::LeftShift) ||
		Platform::Input::IsKeyPressed(Platform::KeyCode::RightShift);

	if (shift) {
		m_State.cursor = hit;
	} else {
		m_State.cursor = hit;
		m_State.selectAnchor = hit;
	}
	ClampScrollOffset(font, scale, visibleWidth);
	QueueRedraw();
}

void TextInput::OnMouseMove(vec2 pos) {
	if (!m_IsPressed) {
		return;
	}
	Text::FontAtlas *font = ResolveFont();
	if (!font) {
		return;
	}

	const float fontSize = GetDisplayStyle().fontSize;
	const float bakeSize = font->GetBakeSize();
	const float scale = (bakeSize > 0.f && fontSize > 0.f) ? (fontSize / bakeSize) : 1.f;

	constexpr float kPadX = 4.f;
	const float visibleWidth = GetLayoutRect().size.x - kPadX * 2.f;
	const float localX = pos.x - GetAbsolutePosition().x - kPadX + m_ScrollOffsetX;
	const size_t hit = m_State.HitTestPos(*font, scale, localX);
	if (hit != m_State.cursor) {
		m_State.cursor = hit;
		ClampScrollOffset(font, scale, visibleWidth);
		QueueRedraw();
	}
}

void TextInput::OnKeyPress(Platform::KeyCode key, int mods) {
	const bool handled = m_State.HandleKeyPress(key, mods);
	if (handled) {
		if (m_OnChanged) {
			m_OnChanged(m_State.text);
		}
		if (Text::FontAtlas *font = ResolveFont()) {
			const float fontSize = GetDisplayStyle().fontSize;
			const float bakeSize = font->GetBakeSize();
			const float scale = (bakeSize > 0.f && fontSize > 0.f) ? (fontSize / bakeSize) : 1.f;
			constexpr float kPadX = 4.f;
			const float visibleWidth = GetLayoutRect().size.x - kPadX * 2.f;
			ClampScrollOffset(font, scale, visibleWidth);
		}
		QueueRedraw();
		return;
	}

	if (key == Platform::KeyCode::Enter) {
		if (m_OnSubmit) {
			m_OnSubmit(m_State.text);
		}
	}
}

void TextInput::OnCharInput(uint32 codepoint) {
	if (m_State.HandleCharInput(codepoint)) {
		if (m_OnChanged) {
			m_OnChanged(m_State.text);
		}
		QueueRedraw();
	}
}

void TextInput::OnFocusGained() {
	View::OnFocusGained();
	QueueRedraw();
}

void TextInput::OnFocusLost() {
	View::OnFocusLost();
	m_State.selectAnchor = m_State.cursor;
	m_ScrollOffsetX = 0.f;
	QueueRedraw();
}

void TextInput::OnDrawSelf(Rendering::DrawList &drawList) {
	View::OnDrawSelf(drawList);

	using namespace Rendering;
	const Rect rect = GetAbsoluteRect();
	const auto &style = GetDisplayStyle();
	const int32 z = 0;
	const float fontSize = style.fontSize > 0.f ? style.fontSize : 14.f;

	Text::FontAtlas *font = ResolveFont();
	if (!font) {
		return;
	}

	const float bakeSize = font->GetBakeSize();
	const float scale = (bakeSize > 0.f) ? (fontSize / bakeSize) : 1.f;
	const float lineH = font->GetLineHeight() * scale;
	const float textY = rect.position.y + (rect.size.y - lineH) * 0.5f;
	constexpr float kPadX = 4.f;

	const Rect textRect = {
		.position = { rect.position.x + kPadX - m_ScrollOffsetX, textY },
		.size = { rect.size.x - kPadX * 2.f + m_ScrollOffsetX, lineH },
	};

	if (m_IsFocused && m_State.HasSelection()) {
		const float x0 = textRect.position.x + m_State.MeasureToPos(*font, scale, m_State.SelectionMin());
		const float x1 = textRect.position.x + m_State.MeasureToPos(*font, scale, m_State.SelectionMax());
		const Rect selRect = { .position = { x0, textY }, .size = { x1 - x0, lineH } };
		const vec4 selColor = vec4(style.color.r, style.color.g, style.color.b, 0.3f);
		drawList.DrawRect(selRect, selColor, vec4(2.f), 0.f, vec4(0.f), z);
	}

	if (!m_State.text.empty()) {
		drawList.DrawText(textRect, m_State.text, font, style.color, fontSize, TextAlign::Left, z + 1);
	} else if (!m_Placeholder.empty() && !m_IsFocused) {
		const vec4 muted = vec4(style.color.r, style.color.g, style.color.b, style.color.a * 0.45f);
		drawList.DrawText(textRect, m_Placeholder, font, muted, fontSize, TextAlign::Left, z + 1);
	}

	if (m_IsFocused && !m_State.HasSelection()) {
		const float cx = textRect.position.x + m_State.MeasureToPos(*font, scale, m_State.cursor);
		const float cy = rect.position.y + (rect.size.y - lineH) * 0.5f;
		const Rect cursor = { .position = { cx - 0.75f, cy }, .size = { 1.5f, lineH } };
		drawList.DrawRect(cursor, style.color, vec4(0.f), 0.f, vec4(0.f), z);
	}
}

} // namespace Aquila::UI::Core
