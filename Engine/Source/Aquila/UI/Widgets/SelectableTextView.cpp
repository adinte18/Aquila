#include "Aquila/UI/Widgets/SelectableTextView.h"

#include "Aquila/Foundation/Math/Math.h"
#include "Aquila/Foundation/SharedConstants.h"
#include "Aquila/Platform/Events/InputEvent.h"
#include "Aquila/UI/Core/Clipboard.h"
#include "Aquila/UI/Rendering/DrawList.h"
#include "Aquila/UI/Text/FontAtlas.h"

namespace Aquila::UI::Core {

SelectableTextView::SelectableTextView() {
	add_class("selectable-text-view");
}

void SelectableTextView::add_line(std::string text, Vec4 color, Int32 tag_begin, Int32 tag_length, Vec4 tag_color) {
	m_lines.push_back(std::move(text));
	m_colors.push_back(color);
	m_tags.push_back({ tag_begin, tag_length, tag_color });
	invalidate_layout();
}

void SelectableTextView::clear() {
	m_lines.clear();
	m_colors.clear();
	m_tags.clear();
	m_selection = {};
	invalidate_layout();
}

void SelectableTextView::remove_front(Int32 count) {
	count = std::clamp(count, 0, get_line_count());
	if (count == 0) {
		return;
	}
	m_lines.erase(m_lines.begin(), m_lines.begin() + count);
	m_colors.erase(m_colors.begin(), m_colors.begin() + count);
	m_tags.erase(m_tags.begin(), m_tags.begin() + count);

	auto shift = [count](Text::TextPosition &position) {
		position.line -= count;
		if (position.line < 0) {
			position = { 0, 0 };
		}
	};
	shift(m_selection.anchor);
	shift(m_selection.focus);
	invalidate_layout();
}

void SelectableTextView::select_all() {
	m_selection = Text::select_all(m_lines);
	queue_redraw();
}

void SelectableTextView::clear_selection() {
	m_selection = {};
	queue_redraw();
}

std::string SelectableTextView::get_selected_text() const {
	return Text::extract_text(m_lines, m_selection);
}

F32 SelectableTextView::line_height() const {
	Text::FontAtlas *font = get_resolved_font();
	const F32 size = get_computed_style().font_size;
	const F32 scale = size > 0.F ? size : SharedConstants::FONT_DEFAULT_SIZE;
	return font != nullptr ? font->get_line_height() * scale : scale;
}

Vec2 SelectableTextView::get_intrinsic_size() const {
	return { -1.F, line_height() * static_cast<F32>(m_lines.size()) };
}

F32 SelectableTextView::width_of(const std::string &line, Int32 columns) const {
	Text::FontAtlas *font = get_resolved_font();
	if (font == nullptr || columns <= 0) {
		return 0.F;
	}
	const F32 size = get_display_style().font_size;
	return font->measure_text(std::string_view(line).substr(0, static_cast<Usize>(columns)), size).x;
}

Text::TextPosition SelectableTextView::position_at(Vec2 canvas_pos) const {
	if (m_lines.empty()) {
		return {};
	}
	const Rect rect = get_absolute_rect();
	const F32 height = line_height();
	const Int32 line = std::clamp(static_cast<Int32>(Math::floor((canvas_pos.y - rect.position.y) / height)), 0,
								  get_line_count() - 1);
	Text::FontAtlas *font = get_resolved_font();
	if (font == nullptr) {
		return { line, 0 };
	}
	const F32 size = get_display_style().font_size;
	const std::string &text = m_lines[static_cast<Usize>(line)];
	const Int32 column = Text::column_at(text, canvas_pos.x - rect.position.x, [font, size](std::string_view part) {
		return font->measure_text(part, size).x;
	});
	return { line, column };
}

Rect SelectableTextView::visible_rect() const {
	Rect visible = get_absolute_rect();
	for (const View *ancestor = get_parent(); ancestor != nullptr; ancestor = ancestor->get_parent()) {
		const UI::Overflow overflow = ancestor->get_display_style().overflow;
		if (overflow == UI::Overflow::Scroll || overflow == UI::Overflow::Hidden) {
			visible = visible.intersect(ancestor->get_absolute_rect());
		}
	}
	return visible;
}

void SelectableTextView::on_mouse_press(Platform::MouseButton btn, Vec2 pos) {
	View::on_mouse_press(btn, pos);
	if (btn != Platform::MouseButton::Left) {
		return;
	}
	request_focus();
	m_selecting = true;
	const Text::TextPosition hit = position_at(pos);
	m_selection = { hit, hit };
	queue_redraw();
}

void SelectableTextView::on_mouse_move(Vec2 pos) {
	if (!m_selecting || !is_pressed()) {
		return;
	}
	const Text::TextPosition hit = position_at(pos);
	if (hit != m_selection.focus) {
		m_selection.focus = hit;
		queue_redraw();
	}
}

void SelectableTextView::on_mouse_release(Platform::MouseButton btn, Vec2 pos) {
	View::on_mouse_release(btn, pos);
	m_selecting = false;
}

void SelectableTextView::on_key_press(Platform::KeyCode key, int mods) {
	if ((mods & Platform::Events::MODIFIER_CONTROL) == 0) {
		return;
	}
	if (key == Platform::KeyCode::C) {
		const std::string selected = get_selected_text();
		if (!selected.empty()) {
			Clipboard::set(selected);
		}
	} else if (key == Platform::KeyCode::A) {
		select_all();
	}
}

void SelectableTextView::on_draw_self(Rendering::DrawList &draw_list) {
	View::on_draw_self(draw_list);

	Text::FontAtlas *font = get_resolved_font();
	if (font == nullptr || m_lines.empty()) {
		return;
	}

	const Rect rect = get_absolute_rect();
	const Rect visible = visible_rect();
	const F32 height = line_height();
	const F32 size = get_display_style().font_size;
	const Vec4 default_color = get_display_style().color;
	const Vec4 selection_color = get_display_style().effective_selection_color();

	const Int32 first = std::max(0, static_cast<Int32>(Math::floor((visible.position.y - rect.position.y) / height)));
	const Int32 last = std::min(get_line_count() - 1,
								static_cast<Int32>(Math::ceil((visible.position.y + visible.size.y - rect.position.y) / height)));

	const Text::TextPosition selection_begin = m_selection.begin();
	const Text::TextPosition selection_end = m_selection.end();
	const bool has_selection = !m_selection.is_empty();

	for (Int32 index = first; index <= last; ++index) {
		const std::string &text = m_lines[static_cast<Usize>(index)];
		const F32 top = rect.position.y + static_cast<F32>(index) * height;

		if (has_selection && index >= selection_begin.line && index <= selection_end.line) {
			const Int32 from = index == selection_begin.line ? selection_begin.column : 0;
			const Int32 to = index == selection_end.line ? selection_end.column : static_cast<Int32>(text.size());
			const F32 start_x = width_of(text, from);
			F32 end_x = width_of(text, to);
			if (index != selection_end.line) {
				end_x += size * 0.5F;
			}
			if (end_x > start_x) {
				draw_list.draw_rect({ { rect.position.x + start_x, top }, { end_x - start_x, height } }, selection_color,
									Vec4(0.F), 0.F, Vec4(0.F), 2);
			}
		}

		const Vec4 color = m_colors[static_cast<Usize>(index)].a > 0.F ? m_colors[static_cast<Usize>(index)] : default_color;
		const Tag &tag = m_tags[static_cast<Usize>(index)];
		const Int32 tag_end = tag.begin + tag.length;
		if (tag.length <= 0 || tag_end > static_cast<Int32>(text.size())) {
			draw_list.draw_text({ { rect.position.x, top }, { rect.size.x, height } }, text, font, color, size,
								UI::TextAlign::Left, 3, false);
			continue;
		}

		const std::string before = text.substr(0, static_cast<Usize>(tag.begin));
		const std::string tagged = text.substr(static_cast<Usize>(tag.begin), static_cast<Usize>(tag.length));
		const std::string after = text.substr(static_cast<Usize>(tag_end));
		const F32 tag_x = width_of(text, tag.begin);
		const F32 after_x = width_of(text, tag_end);
		draw_list.draw_text({ { rect.position.x, top }, { rect.size.x, height } }, before, font, color, size,
							UI::TextAlign::Left, 3, false);
		draw_list.draw_text({ { rect.position.x + tag_x, top }, { rect.size.x - tag_x, height } }, tagged, font,
							tag.color, size, UI::TextAlign::Left, 3, false);
		draw_list.draw_text({ { rect.position.x + after_x, top }, { rect.size.x - after_x, height } }, after, font,
							color, size, UI::TextAlign::Left, 3, false);
	}
}

} // namespace Aquila::UI::Core
