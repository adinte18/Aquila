#include "Aquila/UI/Widgets/PropertyGrid.h"

namespace Aquila::UI::Core {

PropertyGrid::PropertyGrid(float label_width) : m_label_width(label_width) {
	add_class("property-grid");
}

View *PropertyGrid::add_row(std::string label, Unique<View> widget, RowLayout layout) {
	const bool stacked = layout == RowLayout::Stacked;

	auto row = std::make_unique<View>();
	row->add_class(stacked ? "property-row-stacked" : "property-row");

	auto lbl = std::make_unique<Label>(std::move(label));
	lbl->add_class("property-label");
	if (stacked) {
		lbl->add_class("property-label-stacked");
	} else {
		StyleProperties lp;
		lp.width = StyleLength::pixel(m_label_width);
		lbl->set_style(lp);
	}
	row->add_child(std::move(lbl));

	widget->add_class("property-value");
	View *widget_raw = row->add_child(std::move(widget));

	add_child(std::move(row));
	return widget_raw;
}

void PropertyGrid::add_separator() {
	auto sep = std::make_unique<View>();
	sep->add_class("property-separator");
	add_child(std::move(sep));
}

} // namespace Aquila::UI::Core
