#include "Aquila/UI/Widgets/Dropdown.h"
#include "Aquila/UI/Text/FontAtlas.h"


namespace Aquila::UI::Core {

Dropdown::Dropdown() {
	auto header = std::make_unique<Button>();
	header->add_class("dropdown-header");
	header->on_click.connect([this] { toggle_popup(); });
	m_header = dynamic_cast<Button *>(add_child(std::move(header)));

	auto popup = std::make_unique<Popup>();
	popup->add_class("dropdown-popup");
	m_popup = dynamic_cast<Popup *>(add_child(std::move(popup)));

	update_header_text();
}

void Dropdown::add_option(std::string value, std::string display) {
	m_options.push_back({ std::move(value), std::move(display) });
	rebuild();
}

void Dropdown::clear_options() {
	m_options.clear();
	m_value.clear();
	rebuild();
	update_header_text();
}

void Dropdown::set_value(const std::string &value) {
	for (const auto &opt : m_options) {
		if (opt.value == value) {
			m_value = value;
			update_header_text();
			return;
		}
	}
}

void Dropdown::clear_selection() {
	m_value.clear();
	update_header_text();
}

void Dropdown::set_placeholder(std::string text) {
	m_placeholder = std::move(text);
	update_header_text();
}

void Dropdown::rebuild() {
	for (View *v : m_option_buttons) {
		m_popup->remove_child(v);
	}
	m_option_buttons.clear();

	for (const auto &opt : m_options) {
		auto btn = std::make_unique<Button>();
		btn->set_reserve_icon_space(true);
		btn->set_text(opt.label());
		btn->add_class("dropdown-option");
		btn->on_click.connect([this, value = opt.value] {
			select(value);
			m_popup->close();
		});
		m_option_buttons.push_back(m_popup->add_child(std::move(btn)));
	}
}

void Dropdown::select(const std::string &value) {
	m_value = value;
	update_header_text();
	on_changed(m_value);
}

Vec2 Dropdown::get_intrinsic_size() const {
	Text::FontAtlas *font = get_resolved_font();
	if (font == nullptr && m_header != nullptr) {
		font = m_header->get_resolved_font();
	}
	if (font == nullptr) {
		return { -1.F, -1.F };
	}

	const F32 font_size = get_display_style().font_size;

	F32 widest = 0.F;
	auto consider = [&](const std::string &text) {
		if (text.empty()) {
			return;
		}
		font->ensure_glyphs(text);
		widest = std::max(widest, font->measure_text(text, font_size).x);
	};

	consider(m_placeholder);
	for (const auto &opt : m_options) {
		consider(opt.label());
	}

	if (widest <= 0.F) {
		return { -1.F, -1.F };
	}

	F32 chrome = 0.F;
	if (m_header != nullptr) {
		const ComputedStyle &header_style = m_header->get_display_style();
		chrome += header_style.padding.left.resolve(0.F) + header_style.padding.right.resolve(0.F);
		chrome += header_style.border_width * 2.F;
	}

	return { widest + chrome, -1.F };
}

void Dropdown::toggle_popup() {
	// Match the open list to the trigger's width so the popup never ends up wider
	// (or narrower) than the collapsed control, overriding the shared min-width.
	StyleProperties sp;
	sp.min_width = StyleLength::pixel(m_header->get_layout_rect().size.x);
	m_popup->merge_style(sp);
	m_popup->toggle();
}

void Dropdown::update_header_text() {
	if (!m_value.empty()) {
		for (const auto &opt : m_options) {
			if (opt.value == m_value) {
				m_header->set_text(opt.label());
				return;
			}
		}
	}
	m_header->set_text(m_placeholder);
}

} // namespace Aquila::UI::Core
