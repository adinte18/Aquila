#include "Aquila/UI/Widgets/PropertyGrid.h"

namespace Aquila::UI::Core {

PropertyGrid::PropertyGrid(float labelWidth) : m_LabelWidth(labelWidth) {
	StyleProperties sp;
	sp.flexDirection = FlexDirection::Column;
	sp.width = StyleLength::Grow();
	SetStyle(sp);
	AddClass("property-grid");
}

View *PropertyGrid::AddRow(std::string label, Unique<View> widget) {
	auto row = CreateUnique<View>();
	{
		StyleProperties rp;
		rp.flexDirection = FlexDirection::Row;
		rp.width = StyleLength::Grow();
		rp.alignItems = AlignItems::Center;
		row->SetStyle(rp);
		row->AddClass("property-row");
	}

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
	StyleProperties sp;
	sp.width = StyleLength::Grow();
	sp.height = StyleLength::Pixel(1.f);
	sp.backgroundColor = vec4(1.f, 1.f, 1.f, 0.08f);
	sep->SetStyle(sp);
	sep->AddClass("property-separator");
	AddChild(std::move(sep));
}

} // namespace Aquila::UI::Core
