#include "Aquila/UI/Widgets/Separator.h"

namespace Aquila::UI::Core {

Separator::Separator() {
	add_class("separator");
	apply_orientation();
}

Separator::Separator(bool vertical) : m_vertical(vertical) {
	add_class("separator");
	apply_orientation();
}

void Separator::set_vertical(bool vertical) {
	if (vertical == m_vertical) {
		return;
	}
	m_vertical = vertical;
	apply_orientation();
}

void Separator::apply_xml_attribute(std::string_view name, std::string_view value, void *loader_ctx) {
	if (name == "vertical") {
		set_vertical(value == "true");
		return;
	}
	View::apply_xml_attribute(name, value, loader_ctx);
}

void Separator::apply_orientation() {
	if (m_vertical) {
		add_class("separator-v");
	} else {
		remove_class("separator-v");
	}
}

} // namespace Aquila::UI::Core
