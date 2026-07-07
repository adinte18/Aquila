#include "Aquila/UI/Widgets/Collapsible.h"

namespace Aquila::UI::Core {

Collapsible::Collapsible(std::string title) {
	auto header = std::make_unique<Button>();
	header->set_text(std::move(title));
	header->add_class("collapsible-header");
	header->on_click.connect([this] {
		set_expanded(!m_expanded);
		on_toggled(m_expanded);
	});
	m_header = static_cast<Button *>(add_child(std::move(header)));

	auto content = std::make_unique<View>();
	content->add_class("collapsible-content");
	m_content = add_child(std::move(content));
	apply_state(); // apply initial collapsed/expanded display state
}

void Collapsible::set_title(std::string title) {
	m_header->set_text(std::move(title));
}

void Collapsible::apply_xml_attribute(std::string_view name, std::string_view value, void *loader_ctx) {
	if (name == "src" || name == "icon" || name == "bank" || name == "uv" || name == "tint") {
		m_header->apply_xml_attribute(name, value, loader_ctx);
		return;
	}
	View::apply_xml_attribute(name, value, loader_ctx);
}

void Collapsible::set_expanded(bool expanded) {
	if (expanded == m_expanded) {
		return;
	}
	m_expanded = expanded;
	apply_state();
}

View *Collapsible::add_content(Unique<View> child) {
	return m_content->add_child(std::move(child));
}

void Collapsible::apply_state() {
	m_content->set_hidden(!m_expanded);
}

} // namespace Aquila::UI::Core
