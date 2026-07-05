#include "Aquila/UI/Widgets/Label.h"
#include "Aquila/UI/Rendering/DrawCmd.h"
#include "Aquila/UI/Style/StyleTypes.h"

namespace Aquila::UI::Core {

Label::Label(std::string text, Text::FontAtlas *font) : m_text(std::move(text)), m_font(font) {
	m_should_skip_hit_test = true;
}

void Label::set_text(std::string text) {
	if (text == m_text) {
		return;
	}
	m_text = std::move(text);
	invalidate_layout();
}

void Label::set_font(Text::FontAtlas *font) {
	if (font == m_font) {
		return;
	}
	m_font = font;
	invalidate_layout();
}

Text::FontAtlas *Label::resolve_font() const {
	if (Text::FontAtlas *css = get_resolved_font()) {
		return css;
	}
	return m_font;
}

Vec2 Label::measure(float override_font_size) const {
	Text::FontAtlas *font = resolve_font();
	if (!font || m_text.empty()) {
		return {};
	}

	const float bake_size = font->get_bake_size();
	const float render_size = (override_font_size > 0.F) ? override_font_size : get_computed_style().font_size;
	const float scale = (bake_size > 0.F && render_size > 0.F) ? (render_size / bake_size) : 1.F;

	float width = 0.F;
	float first_bearing_x = 0.F;
	bool first = true;
	for (unsigned char ch : m_text) {
		const Text::GlyphInfo *glyph = font->get_glyph(static_cast<Uint32>(ch));
		if (glyph) {
			if (first) {
				first_bearing_x = glyph->bearing.x;
				first = false;
			}
			width += glyph->advance * scale;
		}
	}
	width -= first_bearing_x * scale;
	const float height = font->get_line_height() * scale;
	return { width, height };
}

Vec2 Label::get_intrinsic_size() const {
	return measure();
}

void Label::on_draw_self(Rendering::DrawList &draw_list) {
	View::on_draw_self(draw_list);

	Text::FontAtlas *font = resolve_font();
	if (m_text.empty() || font == nullptr) {
		return;
	}

	const Rect world_rect = { .position = get_absolute_position(), .size = get_layout_rect().size };
	const Vec4 color = get_display_style().color;
	const float font_size = get_display_style().font_size;
	const Int32 z = 3;

	draw_list.DrawText(world_rect, m_text, font, color, font_size, get_display_style().text_align, z);
}

} // namespace Aquila::UI::Core
