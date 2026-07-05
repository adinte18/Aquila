#include "Aquila/UI/Widgets/Popup.h"

namespace Aquila::UI::Core {

Popup::Popup() : FloatingOverlay(49) {
	add_class("popup");
	set_dismiss_on_click_away(true);
	FloatingConfig fc;
	fc.attach_to = FloatingAttachTo::Parent;
	fc.element_point = FloatingAttachPoint::LeftTop;
	fc.parent_point = FloatingAttachPoint::LeftBottom;
	fc.offset = { 0.F, 4.F };
	fc.z_index = 100;
	set_floating(fc);
}
} // namespace Aquila::UI::Core
