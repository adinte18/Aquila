#include "Aquila/UI/Core/Label.h"

namespace Aquila::UI::Core {

Label::Label(std::string text, Text::FontAtlas *font) : m_Text(std::move(text)), m_Font(font) {}

vec2 Label::Measure() const {
	if (!m_Font || m_Text.empty()) {
		return {};
	}

	float width = 0.f;
	float height = m_Font->GetLineHeight();

	for (unsigned char ch : m_Text) {
		const Text::GlyphInfo *glyph = m_Font->GetGlyph(static_cast<uint32>(ch));
		if (glyph) {
			width += glyph->advance;
		}
	}
	return { width, height };
}

void Label::OnDraw(Rendering::DrawList &drawList, vec2 origin) {
	View::OnDraw(drawList, origin);

	if (m_Text.empty() || m_Font == nullptr) {
		return;
	}

	const Rect &layout = GetLayoutRect();
	const Rect worldRect{ .position = layout.position + origin, .size = layout.size };

	const vec4 color = GetComputedStyle().color;
	const int32 z = GetComputedStyle().zIndex;

	drawList.DrawText(worldRect, m_Text, m_Font, color, z);
}

} // namespace Aquila::UI::Core
