#include "Aquila/UI/Widgets/Tooltip.h"

namespace Aquila::UI::Core {

Tooltip::Tooltip() : FloatingOverlay(48) {
	add_class("tooltip");
	set_dismiss_on_click_away(false);

	auto label = std::make_unique<Label>("");
	label->add_class("tooltip-label");
	m_label = static_cast<Label *>(add_child(std::move(label)));
}

void Tooltip::show_at(Vec2 canvas_pos, std::string text) {
	m_label->set_text(std::move(text));

	FloatingConfig fc;
	fc.attach_to = FloatingAttachTo::Root;
	fc.element_point = FloatingAttachPoint::LeftTop;
	fc.parent_point = FloatingAttachPoint::LeftTop;
	fc.offset = canvas_pos;
	fc.z_index = 100;
	set_floating(fc);

	open();
}

void Tooltip::hide() {
	close();
}

} // namespace Aquila::UI::Core
