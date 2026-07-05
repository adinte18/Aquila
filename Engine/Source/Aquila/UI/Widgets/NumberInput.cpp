#include "Aquila/UI/Widgets/NumberInput.h"

namespace Aquila::UI::Core {

NumberInput::NumberInput() {
	update_display_text();
}

void NumberInput::set_value(double value) {
	m_value = std::clamp(value, m_min, m_max);
	update_display_text();
}

void NumberInput::set_range(double min, double max) {
	m_min = min;
	m_max = max;
	m_value = std::clamp(m_value, m_min, m_max);
	update_display_text();
}

void NumberInput::set_step(double step) {
	m_step = step;
}

void NumberInput::set_precision(int decimals) {
	m_precision = std::max(0, decimals);
	update_display_text();
}


void NumberInput::on_key_press(Platform::KeyCode key, int mods) {
	if (key == Platform::KeyCode::Up) {
		set_value(m_value + m_step);
		on_value_changed(m_value);
		return;
	}
	if (key == Platform::KeyCode::Down) {
		set_value(m_value - m_step);
		on_value_changed(m_value);
		return;
	}
	if (key == Platform::KeyCode::Enter) {
		commit_text();
		return;
	}
	TextInput::on_key_press(key, mods);
}

void NumberInput::on_focus_lost() {
	commit_text();
	TextInput::on_focus_lost();
}

void NumberInput::commit_text() {
	try {
		const double parsed = std::stod(m_state.text);
		m_value = std::clamp(parsed, m_min, m_max);
		on_value_changed(m_value);
	} catch (...) {
	}
	update_display_text();
}

void NumberInput::update_display_text() {
	std::ostringstream ss;
	ss.precision(m_precision);
	ss << std::fixed << m_value;
	TextInput::set_text(ss.str());
}

} // namespace Aquila::UI::Core
