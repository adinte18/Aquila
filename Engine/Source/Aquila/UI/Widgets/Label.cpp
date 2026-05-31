#include "Aquila/UI/Widgets/Label.h"
#include "Aquila/UI/Rendering/DrawCmd.h"
#include "Aquila/UI/Style/StyleTypes.h"

namespace Aquila::UI::Core {

Label::Label(std::string text, Text::FontAtlas *font) : m_Text(std::move(text)), m_Font(font) {}

void Label::SetText(std::string text) {
	if (text == m_Text) {
		return;
	}
	m_Text = std::move(text);
	InvalidateLayout();
}

void Label::SetFont(Text::FontAtlas *font) {
	if (font == m_Font) {
		return;
	}
	m_Font = font;
	InvalidateLayout();
}

Text::FontAtlas *Label::ResolveFont() const {
	if (Text::FontAtlas *css = GetResolvedFont()) {
		return css;
	}
	return m_Font;
}

vec2 Label::Measure(float overrideFontSize) const {
	Text::FontAtlas *font = ResolveFont();
	if (!font || m_Text.empty()) {
		return {};
	}

	const float bakeSize = font->GetBakeSize();
	const float renderSize = (overrideFontSize > 0.f) ? overrideFontSize : GetComputedStyle().fontSize;
	const float scale = (bakeSize > 0.f && renderSize > 0.f) ? (renderSize / bakeSize) : 1.f;

	float width = 0.f;
	float firstBearingX = 0.f;
	bool first = true;
	for (unsigned char ch : m_Text) {
		const Text::GlyphInfo *glyph = font->GetGlyph(static_cast<uint32>(ch));
		if (glyph) {
			if (first) {
				firstBearingX = glyph->bearing.x;
				first = false;
			}
			width += glyph->advance * scale;
		}
	}
	width -= firstBearingX * scale;
	const float height = font->GetLineHeight() * scale;
	return { width, height };
}

vec2 Label::GetIntrinsicSize() const {
	return Measure();
}

void Label::OnDrawSelf(Rendering::DrawList &drawList) {
	View::OnDrawSelf(drawList);

	Text::FontAtlas *font = ResolveFont();
	if (m_Text.empty() || font == nullptr) {
		return;
	}

	const Rect worldRect = { .position = GetAbsolutePosition(), .size = GetLayoutRect().size };
	const vec4 color = GetDisplayStyle().color;
	const float fontSize = GetDisplayStyle().fontSize;
	const int32 z = GetStackingZ() * 4 + 3;

	drawList.DrawText(worldRect, m_Text, font, color, fontSize, TextAlign::Center, z);
}

} // namespace Aquila::UI::Core
