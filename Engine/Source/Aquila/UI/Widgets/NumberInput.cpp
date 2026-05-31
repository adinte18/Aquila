#include "Aquila/UI/Widgets/NumberInput.h"

namespace Aquila::UI::Core {

NumberInput::NumberInput() {
	UpdateDisplayText();
}

void NumberInput::SetValue(double value) {
	m_Value = std::clamp(value, m_Min, m_Max);
	UpdateDisplayText();
}

void NumberInput::SetRange(double min, double max) {
	m_Min = min;
	m_Max = max;
	m_Value = std::clamp(m_Value, m_Min, m_Max);
	UpdateDisplayText();
}

void NumberInput::SetStep(double step) {
	m_Step = step;
}

void NumberInput::SetPrecision(int decimals) {
	m_Precision = std::max(0, decimals);
	UpdateDisplayText();
}

void NumberInput::SetOnValueChanged(Delegate<void(double)> callback) {
	m_OnValueChanged = std::move(callback);
}

void NumberInput::OnKeyPress(Platform::KeyCode key, int mods) {
	if (key == Platform::KeyCode::Up) {
		SetValue(m_Value + m_Step);
		if (m_OnValueChanged) {
			m_OnValueChanged(m_Value);
		}
		return;
	}
	if (key == Platform::KeyCode::Down) {
		SetValue(m_Value - m_Step);
		if (m_OnValueChanged) {
			m_OnValueChanged(m_Value);
		}
		return;
	}
	if (key == Platform::KeyCode::Enter) {
		CommitText();
		return;
	}
	TextInput::OnKeyPress(key, mods);
}

void NumberInput::OnFocusLost() {
	CommitText();
	TextInput::OnFocusLost();
}

void NumberInput::CommitText() {
	try {
		const double parsed = std::stod(m_State.text);
		m_Value = std::clamp(parsed, m_Min, m_Max);
		if (m_OnValueChanged) {
			m_OnValueChanged(m_Value);
		}
	} catch (...) {
	}
	UpdateDisplayText();
}

void NumberInput::UpdateDisplayText() {
	std::ostringstream ss;
	ss.precision(m_Precision);
	ss << std::fixed << m_Value;
	TextInput::SetText(ss.str());
}

} // namespace Aquila::UI::Core
