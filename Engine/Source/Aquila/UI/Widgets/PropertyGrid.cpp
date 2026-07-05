#include "Aquila/UI/Widgets/PropertyGrid.h"

namespace Aquila::UI::Core {

PropertyGrid::PropertyGrid(float label_width) : m_label_width(label_width) {
	add_class("property-grid");
}

View *PropertyGrid::add_row(std::string label, Unique<View> widget) {
	auto row = create_unique<View>();
	row->add_class("property-row");

	auto lbl = create_unique<Label>(std::move(label));
	{
		StyleProperties lp;
		lp.width = StyleLength::pixel(m_label_width);
		lbl->set_style(lp);
		lbl->add_class("property-label");
	}
	row->add_child(std::move(lbl));

	widget->add_class("property-value");
	View *widget_raw = row->add_child(std::move(widget));

	add_child(std::move(row));
	return widget_raw;
}

void PropertyGrid::add_separator() {
	auto sep = create_unique<View>();
	sep->add_class("property-separator");
	add_child(std::move(sep));
}

} // namespace Aquila::UI::Core
