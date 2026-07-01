#include "Aquila/UI/Widgets/Toggle.h"

namespace Aquila::UI::Core {

Toggle::Toggle() {
	SetInputLeaf(true);
	AddClass("toggle");
}

Toggle::Toggle(bool on) : m_On(on) {
	SetInputLeaf(true);
	AddClass("toggle");
	if (on) {
		AddClass("toggle-on");
	}
}

void Toggle::SetOn(bool on) {
	if (on == m_On) {
		return;
	}
	m_On = on;
	SyncStateClass();
	QueueRedraw();
}

void Toggle::SyncStateClass() {
	if (m_On) {
		AddClass("toggle-on");
	} else {
		RemoveClass("toggle-on");
	}
}

void Toggle::OnMouseRelease(Platform::MouseButton btn, vec2 pos) {
	if (btn == Platform::MouseButton::Left && m_IsHovered) {
		m_On = !m_On;
		SyncStateClass();
		onChanged(m_On);
		QueueRedraw();
	}
	View::OnMouseRelease(btn, pos);
}

void Toggle::OnDrawSelf(Rendering::DrawList &drawList) {
	View::OnDrawSelf(drawList);

	using namespace Rendering;
	const Rect rect = GetAbsoluteRect();
	const auto &style = GetDisplayStyle();
	const int32 z = 0;

	const vec4 trackColor = style.EffectiveAccentColor();
	const vec4 thumbColor = style.color;

	// Track
	const float trackH = rect.size.y * 0.55f;
	const float trackY = rect.position.y + (rect.size.y - trackH) * 0.5f;
	const Rect track = { .position = { rect.position.x, trackY }, .size = { rect.size.x, trackH } };
	drawList.DrawRect(track, trackColor, vec4(trackH * 0.5f), 0.f, vec4(0.f), z + 2);

	// Thumb — drawn on top of track
	const float thumbDiam = rect.size.y - 4.f;
	const float thumbY = rect.position.y + 2.f;
	const float thumbX = m_On ? rect.position.x + rect.size.x - thumbDiam - 2.f : rect.position.x + 2.f;
	const Rect thumb = { .position = { thumbX, thumbY }, .size = { thumbDiam, thumbDiam } };
	drawList.DrawRect(thumb, thumbColor, vec4(thumbDiam * 0.5f), 0.f, vec4(0.f), z + 3);
}

} // namespace Aquila::UI::Core
