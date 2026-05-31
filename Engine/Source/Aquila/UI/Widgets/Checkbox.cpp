#include "Aquila/UI/Widgets/Checkbox.h"

namespace Aquila::UI::Core {

Checkbox::Checkbox() {
	SetInputLeaf(true);
}

Checkbox::Checkbox(bool checked) : m_Checked(checked) {
	SetInputLeaf(true);
}

void Checkbox::SetChecked(bool checked) {
	if (checked == m_Checked) {
		return;
	}
	m_Checked = checked;
	QueueRedraw();
}

void Checkbox::SetOnChanged(Delegate<void(bool)> callback) {
	m_OnChanged = std::move(callback);
}

void Checkbox::OnMouseRelease(Platform::MouseButton btn, vec2 pos) {
	if (btn == Platform::MouseButton::Left && m_IsHovered) {
		m_Checked = !m_Checked;
		if (m_OnChanged) {
			m_OnChanged(m_Checked);
		}
		QueueRedraw();
	}
	View::OnMouseRelease(btn, pos);
}

void Checkbox::OnDrawSelf(Rendering::DrawList &drawList) {
	View::OnDrawSelf(drawList); // draws background + border from style

	if (!m_Checked) {
		return;
	}

	const Rect rect = GetAbsoluteRect();
	const auto &style = GetDisplayStyle();
	constexpr float pad = 4.f;
	const int32 z = GetStackingZ() * 4 + 2;
	const Rect fill = {
		.position = rect.position + vec2(pad),
		.size = rect.size - vec2(pad * 2.f),
	};
	drawList.DrawRect(fill, style.color, style.borderRadius, 0.f, vec4(0.f), z);
}

} // namespace Aquila::UI::Core
