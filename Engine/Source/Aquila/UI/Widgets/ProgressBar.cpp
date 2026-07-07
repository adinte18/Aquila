#include "Aquila/UI/Widgets/ProgressBar.h"

namespace Aquila::UI::Core {

ProgressBar::ProgressBar() {
	add_class("progress-bar");

	auto fill = std::make_unique<View>();
	fill->add_class("progress-fill");
	m_fill = add_child(std::move(fill));

	set_value(0.F);
}

void ProgressBar::set_value(float value) {
	m_value = std::clamp(value, 0.F, 1.F);

	StyleProperties sp;
	sp.width = StyleLength::percent(m_value * 100.F);
	sp.height = StyleLength::grow();
	m_fill->merge_style(sp);
}

void ProgressBar::apply_xml_attribute(std::string_view name, std::string_view value, void *loader_ctx) {
	if (name == "value") {
		set_value(std::stof(std::string(value)));
		return;
	}
	View::apply_xml_attribute(name, value, loader_ctx);
}

} // namespace Aquila::UI::Core
