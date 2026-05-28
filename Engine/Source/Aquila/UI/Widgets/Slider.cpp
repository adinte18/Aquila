#include "Aquila/UI/Widgets/Slider.h"
#include <algorithm>
#include <cmath>

namespace Aquila::UI::Core {

static constexpr float kTrackHeight = 4.f;
static constexpr float kHandleSize = 12.f;
static constexpr float kHandlePad = kHandleSize * 0.5f;

Slider::Slider() {
	SetCapturesInput(true);
}

void Slider::SetValue(float value) {
	float clamped = std::clamp(value, m_Min, m_Max);
	if (m_Step > 0.f) {
		clamped = std::round((clamped - m_Min) / m_Step) * m_Step + m_Min;
	}
	if (clamped == m_Value) {
		return;
	}
	m_Value = clamped;
	QueueRedraw();
}

void Slider::SetRange(float min, float max) {
	m_Min = min;
	m_Max = max;
	SetValue(m_Value); // re-clamp
}

void Slider::SetStep(float step) {
	m_Step = step;
	SetValue(m_Value); // re-snap
}

void Slider::SetOnChanged(Delegate<void(float)> callback) {
	m_OnChanged = std::move(callback);
}

float Slider::ValueFromX(float x) const {
	const Rect rect = { .position = GetAbsolutePosition(), .size = GetLayoutRect().size };
	const float left = rect.position.x + kHandlePad;
	const float right = rect.position.x + rect.size.x - kHandlePad;
	const float t = std::clamp((x - left) / (right - left), 0.f, 1.f);
	float raw = m_Min + t * (m_Max - m_Min);
	if (m_Step > 0.f) {
		raw = std::round((raw - m_Min) / m_Step) * m_Step + m_Min;
	}
	return std::clamp(raw, m_Min, m_Max);
}

void Slider::OnMouseEnter() {
	View::OnMouseEnter();
}
void Slider::OnMouseLeave() {
	View::OnMouseLeave();
}

void Slider::OnMousePress(Platform::MouseButton btn, vec2 pos) {
	if (btn == Platform::MouseButton::Left) {
		const float newVal = ValueFromX(pos.x);
		if (newVal != m_Value) {
			m_Value = newVal;
			if (m_OnChanged) {
				m_OnChanged(m_Value);
			}
			QueueRedraw();
		}
	}
	View::OnMousePress(btn, pos);
}

void Slider::OnMouseRelease(Platform::MouseButton btn, vec2 pos) {
	View::OnMouseRelease(btn, pos);
}

void Slider::OnMouseMove(vec2 pos) {
	const float newVal = ValueFromX(pos.x);
	if (newVal != m_Value) {
		m_Value = newVal;
		if (m_OnChanged) {
			m_OnChanged(m_Value);
		}
		QueueRedraw();
	}
}

void Slider::OnDrawSelf(Rendering::DrawList &drawList) {
	View::OnDrawSelf(drawList);

	const Rect rect = { .position = GetAbsolutePosition(), .size = GetLayoutRect().size };
	const auto &style = GetDisplayStyle();
	const int32 z = style.zIndex * 4;
	const float cy = rect.position.y + rect.size.y * 0.5f;

	const float t = (m_Max > m_Min) ? (m_Value - m_Min) / (m_Max - m_Min) : 0.f;

	if (m_TrackTex) {
		const float trackH = rect.size.y;
		const Rect track = {
			.position = { rect.position.x, rect.position.y },
			.size = { rect.size.x, trackH },
		};
		drawList.DrawImage(track, m_TrackTex, vec4(1.f), vec2(0.f), vec2(1.f), z + 1);

		const float hx = rect.position.x + t * rect.size.x - kHandleSize * 0.5f;
		const float hy = cy - kHandleSize * 0.5f;
		const Rect handle = { .position = { hx, hy }, .size = { kHandleSize, kHandleSize } };
		drawList.DrawRect(handle, vec4(1.f), vec4(kHandleSize * 0.5f), 1.5f, vec4(0.f, 0.f, 0.f, 0.7f), z + 2);
	} else {
		const Rect track = {
			.position = { rect.position.x + kHandlePad, cy - kTrackHeight * 0.5f },
			.size = { rect.size.x - kHandlePad * 2.f, kTrackHeight },
		};
		const vec4 trackColor = vec4(style.color.r, style.color.g, style.color.b, style.color.a * 0.3f);
		drawList.DrawRect(track, trackColor, vec4(kTrackHeight * 0.5f), 0.f, vec4(0.f), z + 1);

		const float fillW = track.size.x * t;
		if (fillW > 0.f) {
			const Rect fill = { .position = track.position, .size = { fillW, kTrackHeight } };
			drawList.DrawRect(fill, style.color, vec4(kTrackHeight * 0.5f), 0.f, vec4(0.f), z + 1);
		}

		const float hx = track.position.x + fillW - kHandleSize * 0.5f;
		const float hy = cy - kHandleSize * 0.5f;
		const Rect handle = { .position = { hx, hy }, .size = { kHandleSize, kHandleSize } };
		drawList.DrawRect(handle, style.color, vec4(kHandleSize * 0.5f), 0.f, vec4(0.f), z + 2);
	}
}

} // namespace Aquila::UI::Core
