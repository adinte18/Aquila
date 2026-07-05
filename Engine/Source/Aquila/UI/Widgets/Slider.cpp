#include "Aquila/UI/Widgets/Slider.h"

namespace Aquila::UI::Core {

static constexpr float kTrackHeight = 4.f;
static constexpr float kHandleSize = 12.f;
static constexpr float kHandlePad = kHandleSize * 0.5f;

Slider::Slider() {
	SetInputLeaf(true);
	AddClass("slider");
}

float Slider::Coerce(const float &value) const {
	float clamped = std::clamp(value, m_Min, m_Max);
	if (m_Step > 0.f) {
		clamped = std::round((clamped - m_Min) / m_Step) * m_Step + m_Min;
	}
	return clamped;
}

void Slider::SetRange(float min, float max) {
	m_Min = min;
	m_Max = max;
	SetValueWithoutNotify(GetValue());
}

void Slider::SetStep(float step) {
	m_Step = step;
	SetValueWithoutNotify(GetValue());
}


float Slider::ValueFromX(float x) const {
	const Rect rect = GetAbsoluteRect();
	const float left = rect.position.x + kHandlePad;
	const float right = rect.position.x + rect.size.x - kHandlePad;
	const float t = std::clamp((x - left) / (right - left), 0.f, 1.f);
	float raw = m_Min + t * (m_Max - m_Min);
	if (m_Step > 0.f) {
		raw = std::round((raw - m_Min) / m_Step) * m_Step + m_Min;
	}
	return std::clamp(raw, m_Min, m_Max);
}

void Slider::OnMousePress(Platform::MouseButton btn, vec2 pos) {
	if (btn == Platform::MouseButton::Left) {
		SetValue(ValueFromX(pos.x));
	}
	View::OnMousePress(btn, pos);
}

void Slider::OnMouseMove(vec2 pos) {
	SetValue(ValueFromX(pos.x));
}

void Slider::OnDrawSelf(Rendering::DrawList &drawList) {
	View::OnDrawSelf(drawList);

	const Rect rect = GetAbsoluteRect();
	using namespace Rendering;
	const auto &style = GetDisplayStyle();
	const int32 z = 0;
	const float cy = rect.position.y + rect.size.y * 0.5f;

	const float t = (m_Max > m_Min) ? (GetValue() - m_Min) / (m_Max - m_Min) : 0.f;

	const vec4 accentColor = style.EffectiveAccentColor();

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
		drawList.DrawRect(handle, accentColor, vec4(kHandleSize * 0.5f), style.borderWidth, style.borderColor, z + 2);
	} else {
		const Rect track = {
			.position = { rect.position.x + kHandlePad, cy - kTrackHeight * 0.5f },
			.size = { rect.size.x - kHandlePad * 2.f, kTrackHeight },
		};
		const vec4 trackColor = vec4(accentColor.r, accentColor.g, accentColor.b, accentColor.a * 0.3f);
		drawList.DrawRect(track, trackColor, vec4(kTrackHeight * 0.5f), 0.f, vec4(0.f), z + 1);

		const float fillW = track.size.x * t;
		if (fillW > 0.f) {
			const Rect fill = { .position = track.position, .size = { fillW, kTrackHeight } };
			drawList.DrawRect(fill, accentColor, vec4(kTrackHeight * 0.5f), 0.f, vec4(0.f), z + 1);
		}

		const float hx = track.position.x + fillW - kHandleSize * 0.5f;
		const float hy = cy - kHandleSize * 0.5f;
		const Rect handle = { .position = { hx, hy }, .size = { kHandleSize, kHandleSize } };
		drawList.DrawRect(handle, accentColor, vec4(kHandleSize * 0.5f), 0.f, vec4(0.f), z + 2);
	}
}

} // namespace Aquila::UI::Core
