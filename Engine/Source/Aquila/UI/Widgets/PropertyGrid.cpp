#include "Aquila/UI/Widgets/PropertyGrid.h"

namespace Aquila::UI::Core {

PropertyGrid::PropertyGrid(float labelWidth) : m_LabelWidth(labelWidth) {
	AddClass("property-grid");
}

View *PropertyGrid::AddRow(std::string label, Unique<View> widget) {
	auto row = CreateUnique<View>();
	row->AddClass("property-row");

	auto lbl = CreateUnique<Label>(std::move(label));
	{
		StyleProperties lp;
		lp.width = StyleLength::Pixel(m_LabelWidth);
		lbl->SetStyle(lp);
		lbl->AddClass("property-label");
	}
	row->AddChild(std::move(lbl));

	widget->AddClass("property-value");
	{
		StyleProperties wp;
		wp.flexGrow = 1.f;
		widget->MergeStyle(wp);
	}
	View *widgetRaw = row->AddChild(std::move(widget));

	AddChild(std::move(row));
	return widgetRaw;
}

void PropertyGrid::AddSeparator() {
	auto sep = CreateUnique<View>();
	sep->AddClass("property-separator");
	AddChild(std::move(sep));
}

} // namespace Aquila::UI::Core
