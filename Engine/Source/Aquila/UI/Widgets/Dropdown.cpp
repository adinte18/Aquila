#include "Aquila/UI/Widgets/Dropdown.h"

namespace Aquila::UI::Core {

Dropdown::Dropdown() {
	auto header = std::make_unique<Button>();
	header->add_class("dropdown-header");
	header->on_click.connect([this] { m_popup->toggle(); });
	m_header = static_cast<Button *>(add_child(std::move(header)));

	auto popup = std::make_unique<Popup>();
	popup->add_class("dropdown-popup");
	m_popup = static_cast<Popup *>(add_child(std::move(popup)));

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
