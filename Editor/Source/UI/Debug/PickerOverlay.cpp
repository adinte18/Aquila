#include "UI/Debug/PickerOverlay.h"

#include "Aquila/UI/Rendering/DrawList.h"
#include "Aquila/UI/Style/ComputedStyle.h"

namespace Editor {

using namespace Aquila;
using namespace Aquila::UI::Core;

PickerOverlay::PickerOverlay() {
	AddClass("picker-overlay");
	m_ShouldSkipHitTest = true;

	Aquila::UI::FloatingConfig fc;
	fc.attachTo = Aquila::UI::FloatingAttachTo::Root;
	fc.elementPoint = Aquila::UI::FloatingAttachPoint::LeftTop;
	fc.parentPoint = Aquila::UI::FloatingAttachPoint::LeftTop;
	fc.zIndex = 900;
	SetFloating(fc);
}

void PickerOverlay::SetTarget(const Rect &rect) {
	m_Active = true;
	m_Target = rect;
	QueueRedraw();
}

void PickerOverlay::Clear() {
	if (!m_Active) {
		return;
	}
	m_Active = false;
	QueueRedraw();
}

void PickerOverlay::OnDrawSelf(Aquila::UI::Rendering::DrawList &drawList) {
	if (!m_Active) {
		return;
	}
	const vec4 accent = GetComputedStyle().color;
	const vec4 fill = { accent.r, accent.g, accent.b, accent.a * 0.18f };
	drawList.DrawRect(m_Target, fill, vec4(0.f), 1.5f, accent, 0);
}

} // namespace Editor
