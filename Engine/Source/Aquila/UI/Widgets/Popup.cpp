#include "Aquila/UI/Widgets/Popup.h"

namespace Aquila::UI::Core {

Popup::Popup() : FloatingOverlay(49) {
	AddClass("popup");
	SetDismissOnClickAway(true);
	FloatingConfig fc;
	fc.attachTo = FloatingAttachTo::Parent;
	fc.elementPoint = FloatingAttachPoint::LeftTop;
	fc.parentPoint = FloatingAttachPoint::LeftBottom;
	fc.offset = { 0.f, 4.f };
	fc.zIndex = 100;
	SetFloating(fc);
}
} // namespace Aquila::UI::Core
