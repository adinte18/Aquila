#include "Aquila/UI/Widgets/Tooltip.h"

namespace Aquila::UI::Core {

Tooltip::Tooltip() : FloatingOverlay(48) {
	AddClass("tooltip");
	SetDismissOnClickAway(false);

	auto label = CreateUnique<Label>("");
	label->AddClass("tooltip-label");
	m_Label = static_cast<Label *>(AddChild(std::move(label)));
}

void Tooltip::ShowAt(vec2 canvasPos, std::string text) {
	m_Label->SetText(std::move(text));

	FloatingConfig fc;
	fc.attachTo = FloatingAttachTo::Root;
	fc.elementPoint = FloatingAttachPoint::LeftTop;
	fc.parentPoint = FloatingAttachPoint::LeftTop;
	fc.offset = canvasPos;
	fc.zIndex = 100;
	SetFloating(fc);

	Open();
}

void Tooltip::Hide() {
	Close();
}

} // namespace Aquila::UI::Core
