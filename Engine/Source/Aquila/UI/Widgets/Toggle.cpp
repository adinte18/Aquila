#include "Aquila/UI/Widgets/Toggle.h"
#include "Aquila/UI/Rendering/DrawCmd.h"

namespace Aquila::UI::Core {

Toggle::Toggle() {
	SetInputLeaf(true);
}

Toggle::Toggle(bool on) : m_On(on) {
	SetInputLeaf(true);
}

void Toggle::SetOn(bool on) {
	if (on == m_On) {
		return;
	}
	m_On = on;
	QueueRedraw();
}

void Toggle::SetOnChanged(Delegate<void(bool)> callback) {
	m_OnChanged = std::move(callback);
}

void Toggle::OnMouseRelease(Platform::MouseButton btn, vec2 pos) {
	if (btn == Platform::MouseButton::Left && m_IsHovered) {
		m_On = !m_On;
		if (m_OnChanged) {
			m_OnChanged(m_On);
		}
		QueueRedraw();
	}
	View::OnMouseRelease(btn, pos);
}

void Toggle::OnDrawSelf(Rendering::DrawList &drawList) {
	View::OnDrawSelf(drawList);

	using namespace Rendering;
	const Rect rect = GetAbsoluteRect();
	const int32 z = GetStackingZ() * 4;

	// Track
	const float trackH = rect.size.y * 0.55f;
	const float trackY = rect.position.y + (rect.size.y - trackH) * 0.5f;
	const Rect track = { .position = { rect.position.x, trackY }, .size = { rect.size.x, trackH } };
	const vec4 trackColor = m_On ? vec4(0.27f, 0.72f, 0.47f, 1.f) : vec4(0.35f, 0.35f, 0.38f, 1.f);
	drawList.DrawRect(track, trackColor, vec4(trackH * 0.5f), 0.f, vec4(0.f), z + 2);

	// Thumb — drawn on top of track
	const float thumbDiam = rect.size.y - 4.f;
	const float thumbY = rect.position.y + 2.f;
	const float thumbX = m_On ? rect.position.x + rect.size.x - thumbDiam - 2.f : rect.position.x + 2.f;
	const Rect thumb = { .position = { thumbX, thumbY }, .size = { thumbDiam, thumbDiam } };
	drawList.DrawRect(thumb, vec4(1.f), vec4(thumbDiam * 0.5f), 0.f, vec4(0.f), z + 3);
}

} // namespace Aquila::UI::Core
