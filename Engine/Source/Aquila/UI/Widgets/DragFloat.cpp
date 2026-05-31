#include "Aquila/UI/Widgets/DragFloat.h"
#include "Aquila/UI/Rendering/DrawCmd.h"

namespace Aquila::UI::Core {

DragFloat::DragFloat() {
	SetInputLeaf(true);
}

void DragFloat::SetValue(float value) {
	m_Value = std::clamp(value, m_Min, m_Max);
	if (m_Step > 0.f) {
		m_Value = std::round(m_Value / m_Step) * m_Step;
	}
	QueueRedraw();
}

void DragFloat::SetRange(float min, float max) {
	m_Min = min;
	m_Max = max;
	SetValue(m_Value);
}

void DragFloat::SetStep(float step) {
	m_Step = step;
}

void DragFloat::SetSpeed(float pixelsPerUnit) {
	m_Speed = pixelsPerUnit;
}

void DragFloat::SetPrecision(int decimals) {
	m_Precision = std::max(0, decimals);
	QueueRedraw();
}

void DragFloat::SetPrefix(std::string prefix) {
	m_Prefix = std::move(prefix);
	QueueRedraw();
}

void DragFloat::SetOnChanged(Delegate<void(float)> callback) {
	m_OnChanged = std::move(callback);
}

std::string DragFloat::FormatValue() const {
	std::ostringstream ss;
	ss.precision(m_Precision);
	ss << std::fixed << m_Value;
	return m_Prefix + ss.str();
}

Text::FontAtlas *DragFloat::ResolveFont() const {
	return GetResolvedFont();
}

void DragFloat::EnterEditMode() {
	m_Mode = Mode::Edit;
	m_EditState.SetText(FormatValue());
	m_EditState.SelectAll();
	QueueRedraw();
}

void DragFloat::CommitEdit() {
	try {
		const float parsed = std::stof(m_EditState.text);
		SetValue(parsed);
		OnValueCommitted();
		if (m_OnChanged) {
			m_OnChanged(m_Value);
		}
	} catch (...) {
		// Restore last valid value on parse failure.
	}
	m_Mode = Mode::Drag;
	QueueRedraw();
}

void DragFloat::CancelEdit() {
	m_Mode = Mode::Drag;
	QueueRedraw();
}

void DragFloat::OnMousePress(Platform::MouseButton btn, vec2 pos) {
	View::OnMousePress(btn, pos);
	if (btn != Platform::MouseButton::Left) {
		return;
	}
	if (m_Mode == Mode::Edit) {
		return;
	}
	m_DragStartValue = m_Value;
	m_DragStartX = pos.x;
	m_HasDragged = false;
}

void DragFloat::OnMouseRelease(Platform::MouseButton btn, vec2 pos) {
	if (btn == Platform::MouseButton::Left) {
		if (m_Mode == Mode::Drag && !m_HasDragged) {
			EnterEditMode();
		}
	}
	View::OnMouseRelease(btn, pos);
}

void DragFloat::OnMouseMove(vec2 pos) {
	if (m_Mode != Mode::Drag || !m_IsPressed) {
		return;
	}
	const float delta = (pos.x - m_DragStartX) * m_Speed;
	if (!m_HasDragged && std::abs(pos.x - m_DragStartX) > kDragThreshold) {
		m_HasDragged = true;
	}
	if (m_HasDragged) {
		SetValue(m_DragStartValue + delta);
		OnValueCommitted();
		if (m_OnChanged) {
			m_OnChanged(m_Value);
		}
	}
}

void DragFloat::OnKeyPress(Platform::KeyCode key, int mods) {
	if (m_Mode == Mode::Edit) {
		if (key == Platform::KeyCode::Enter || key == Platform::KeyCode::Tab) {
			CommitEdit();
			return;
		}
		if (key == Platform::KeyCode::Escape) {
			CancelEdit();
			return;
		}
		if (m_EditState.HandleKeyPress(key, mods)) {
			QueueRedraw();
		}
		return;
	}
	View::OnKeyPress(key, mods);
}

void DragFloat::OnCharInput(uint32 codepoint) {
	if (m_Mode == Mode::Edit) {
		if (m_EditState.HandleCharInput(codepoint)) {
			QueueRedraw();
		}
	}
}

void DragFloat::OnFocusLost() {
	if (m_Mode == Mode::Edit) {
		CommitEdit();
	}
	View::OnFocusLost();
}

void DragFloat::OnDrawSelf(Rendering::DrawList &drawList) {
	View::OnDrawSelf(drawList);

	using namespace Rendering;
	const Rect rect = GetAbsoluteRect();
	const auto &style = GetDisplayStyle();
	const int32 z = GetStackingZ() * 4;
	const float fontSize = style.fontSize > 0.f ? style.fontSize : 14.f;

	Text::FontAtlas *font = ResolveFont();
	if (!font) {
		return;
	}

	const float bakeSize = font->GetBakeSize();
	const float scale = (bakeSize > 0.f) ? (fontSize / bakeSize) : 1.f;
	const float lineH = font->GetLineHeight() * scale;
	const float textY = rect.position.y + (rect.size.y - lineH) * 0.5f;
	constexpr float kPadX = 4.f;
	const Rect textRect = {
		.position = { rect.position.x + kPadX, textY },
		.size = { rect.size.x - kPadX * 2.f, lineH },
	};

	if (m_Mode == Mode::Edit) {
		if (m_IsFocused && m_EditState.HasSelection()) {
			const float x0 = textRect.position.x + m_EditState.MeasureToPos(*font, scale, m_EditState.SelectionMin());
			const float x1 = textRect.position.x + m_EditState.MeasureToPos(*font, scale, m_EditState.SelectionMax());
			const Rect selRect = { .position = { x0, textY }, .size = { x1 - x0, lineH } };
			const vec4 selColor = vec4(style.color.r, style.color.g, style.color.b, 0.3f);
			drawList.DrawRect(selRect, selColor, vec4(2.f), 0.f, vec4(0.f), z);
		}

		if (!m_EditState.text.empty()) {
			drawList.DrawText(textRect, m_EditState.text, font, style.color, fontSize, TextAlign::Left, z + 1);
		}

		if (m_IsFocused && !m_EditState.HasSelection()) {
			const float cx = textRect.position.x + m_EditState.MeasureToPos(*font, scale, m_EditState.cursor);
			const Rect cursor = { .position = { cx - 0.75f, textY }, .size = { 1.5f, lineH } };
			drawList.DrawRect(cursor, style.color, vec4(0.f), 0.f, vec4(0.f), z);
		}
	} else {
		// Drag mode: show the formatted value, optionally a subtle drag indicator.
		const std::string display = FormatValue();
		drawList.DrawText(textRect, display, font, style.color, fontSize, TextAlign::Center, z + 1);

		// Small arrows hint that the field is draggable.
		constexpr float kArrowSize = 4.f;
		const float cy = rect.position.y + rect.size.y * 0.5f;
		const vec4 arrowColor = vec4(style.color.r, style.color.g, style.color.b, style.color.a * 0.4f);
		const Rect leftArrow = {
			.position = { rect.position.x + 3.f, cy - kArrowSize * 0.5f },
			.size = { kArrowSize, kArrowSize },
		};
		const Rect rightArrow = {
			.position = { rect.position.x + rect.size.x - kArrowSize - 3.f, cy - kArrowSize * 0.5f },
			.size = { kArrowSize, kArrowSize },
		};
		drawList.DrawRect(leftArrow, arrowColor, vec4(1.f), 0.f, vec4(0.f), z + 2);
		drawList.DrawRect(rightArrow, arrowColor, vec4(1.f), 0.f, vec4(0.f), z + 2);
	}
}

} // namespace Aquila::UI::Core
