#pragma once

#include "Aquila/UI/Widgets/TextInput.h"

namespace Aquila::UI::Core {

class NumberInput : public TextInput {
  public:
	NumberInput();

	[[nodiscard]] std::string_view get_type_name() const override { return "NumberInput"; }

	void set_value(F64 value);
	void set_range(F64 min, F64 max);
	void set_step(F64 step);
	void set_precision(int decimals);
	Signal<void(F64)> on_value_changed;

	[[nodiscard]] F64 get_value() const { return m_value; }

	void on_key_press(Platform::KeyCode key, int mods = 0) override;
	void on_focus_lost() override;

  private:
	void commit_text();
	void update_display_text();

	F64 m_value = 0.0;
	F64 m_min = -1e18;
	F64 m_max = 1e18;
	F64 m_step = 1.0;
	int m_precision = 3;
};

} // namespace Aquila::UI::Core
