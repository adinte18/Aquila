#include "Aquila/UI/Widgets/VecField.h"

namespace Aquila::UI::Core {

LabeledDragFloat::LabeledDragFloat(const char *label) {
	AddClass("labeled-drag");

	auto lbl = CreateUnique<Label>(label);
	lbl->AddClass("vec-label");
	m_Label = static_cast<Label *>(AddChild(std::move(lbl)));

	auto drag = CreateUnique<DragFloat>();
	drag->AddClass("vec-drag");
	m_Drag = static_cast<DragFloat *>(AddChild(std::move(drag)));
}

Vec2Field::Vec2Field() {
	AddClass("vec2-field");

	auto xf = CreateUnique<LabeledDragFloat>("X");
	auto yf = CreateUnique<LabeledDragFloat>("Y");

	RegisterComponent(0, xf->GetDrag());
	RegisterComponent(1, yf->GetDrag());

	AddChild(std::move(xf));
	AddChild(std::move(yf));
}

Vec3Field::Vec3Field() {
	auto makeDrag = [](const char *prefix, const char *axisClass) {
		auto drag = CreateUnique<DragFloat>();
		drag->SetPrefix(prefix);
		drag->AddClass("vec-drag");
		drag->AddClass(axisClass);
		return drag;
	};

	auto xDrag = makeDrag("X ", "vec-drag-x");
	auto yDrag = makeDrag("Y ", "vec-drag-y");
	auto zDrag = makeDrag("Z ", "vec-drag-z");

	RegisterComponent(0, static_cast<DragFloat *>(AddChild(std::move(xDrag))));
	RegisterComponent(1, static_cast<DragFloat *>(AddChild(std::move(yDrag))));
	RegisterComponent(2, static_cast<DragFloat *>(AddChild(std::move(zDrag))));
}

Vec4Field::Vec4Field() {
	AddClass("vec4-field");

	auto xf = CreateUnique<LabeledDragFloat>("X");
	auto yf = CreateUnique<LabeledDragFloat>("Y");
	auto zf = CreateUnique<LabeledDragFloat>("Z");
	auto wf = CreateUnique<LabeledDragFloat>("W");

	RegisterComponent(0, xf->GetDrag());
	RegisterComponent(1, yf->GetDrag());
	RegisterComponent(2, zf->GetDrag());
	RegisterComponent(3, wf->GetDrag());

	AddChild(std::move(xf));
	AddChild(std::move(yf));
	AddChild(std::move(zf));
	AddChild(std::move(wf));
}

} // namespace Aquila::UI::Core
