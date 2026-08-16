#include "Aquila/UI/Widgets/Label.h"
#include "Aquila/Foundation/Text/Utf8.h"
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
	if ((font == nullptr) || m_text.empty()) {
		return {};
	}

	const float bake_size = font->get_bake_size();
	const float render_size = (override_font_size > 0.F) ? override_font_size : get_computed_style().font_size;
	const float scale = (bake_size > 0.F && render_size > 0.F) ? (render_size / bake_size) : 1.F;

	Vec2 dims = font->measure_text(m_text, render_size);

	const Foundation::Utf8::Decoded first = Foundation::Utf8::decode(m_text, 0);
	if (const Text::GlyphInfo *glyph = font->get_glyph(first.codepoint)) {
		dims.x -= glyph->bearing.x * scale;
	}
	return dims;
}

Vec2 Label::get_intrinsic_size() const {
	return measure();
}

bool Label::get_clay_text_run(ClayTextRun &out) const {
	if (get_display_style().white_space != WhiteSpace::Normal) {
		return false;
	}
	Text::FontAtlas *font = resolve_font();
	if (font == nullptr || m_text.empty()) {
		return false;
	}
	out.text = m_text;
	out.font = font;
	out.font_size = get_display_style().font_size;
	out.align = get_display_style().text_align;
	return true;
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
	const bool wrap = get_display_style().white_space == WhiteSpace::Normal;

	draw_list.DrawText(world_rect, m_text, font, color, font_size, get_display_style().text_align, z, wrap);
}

} // namespace Aquila::UI::Core
