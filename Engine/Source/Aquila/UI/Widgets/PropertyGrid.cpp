#include "Aquila/UI/Widgets/PropertyGrid.h"

namespace Aquila::UI::Core {

PropertyGrid::PropertyGrid(float label_width) : m_label_width(label_width) {
	add_class("property-grid");
}

void PropertyGrid::set_split(bool split) {
	m_split = split;
	set_class("property-grid-split", split);
}

View *PropertyGrid::add_row(std::string label, Unique<View> widget, RowLayout layout) {
	if (m_split) {
		return add_split_row(std::move(label), std::move(widget), layout);
	}

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

View *PropertyGrid::add_split_row(std::string label, Unique<View> widget, RowLayout layout) {
	auto row = std::make_unique<View>();
	row->add_class("property-split-row");
	row->set_class("property-split-row-top", layout == RowLayout::Stacked);

	auto lbl = std::make_unique<Label>(std::move(label));
	lbl->add_class("property-split-label");
	row->add_child(std::move(lbl));

	widget->add_class("property-split-value");
	widget->add_class("split-" + std::string(widget->get_type_name()));
	View *widget_raw = row->add_child(std::move(widget));
	add_child(std::move(row));
	return widget_raw;
}

View *PropertyGrid::add_check_row(std::string label, Unique<View> widget) {
	if (!m_split) {
		return add_row(std::move(label), std::move(widget));
	}

	auto row = std::make_unique<View>();
	row->add_class("property-split-row");

	auto spacer = std::make_unique<Label>(std::string{});
	spacer->add_class("property-split-label");
	row->add_child(std::move(spacer));

	widget->add_class("property-split-check");
	widget->add_class("split-" + std::string(widget->get_type_name()));
	View *widget_raw = row->add_child(std::move(widget));

	auto text = std::make_unique<Label>(std::move(label));
	text->add_class("property-split-check-text");
	row->add_child(std::move(text));

	add_child(std::move(row));
	return widget_raw;
}

void PropertyGrid::add_separator() {
	auto sep = std::make_unique<View>();
	sep->add_class("property-separator");
	add_child(std::move(sep));
}

} // namespace Aquila::UI::Core
