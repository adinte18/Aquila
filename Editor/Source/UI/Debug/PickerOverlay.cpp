#include "UI/Debug/PickerOverlay.h"

#include "Aquila/UI/Rendering/DrawList.h"
#include "Aquila/UI/Style/ComputedStyle.h"

namespace Editor {

using namespace Aquila;
using namespace Aquila::UI::Core;

PickerOverlay::PickerOverlay() {
	add_class("picker-overlay");
	m_should_skip_hit_test = true;

	Aquila::UI::FloatingConfig fc;
	fc.attach_to = Aquila::UI::FloatingAttachTo::Root;
	fc.element_point = Aquila::UI::FloatingAttachPoint::LeftTop;
	fc.parent_point = Aquila::UI::FloatingAttachPoint::LeftTop;
	fc.z_index = 900;
	set_floating(fc);
}

void PickerOverlay::set_target(const Rect &rect) {
	m_active = true;
	m_target = rect;
	queue_redraw();
}

void PickerOverlay::clear() {
	if (!m_active) {
		return;
	}
	m_active = false;
	queue_redraw();
}

void PickerOverlay::on_draw_self(Aquila::UI::Rendering::DrawList &draw_list) {
	if (!m_active) {
		return;
	}
	const Vec4 accent = get_computed_style().color;
	const Vec4 fill = { accent.r, accent.g, accent.b, accent.a * 0.18f };
	draw_list.draw_rect(m_target, fill, Vec4(0.F), 1.5f, accent, 0);
}

} // namespace Editor
