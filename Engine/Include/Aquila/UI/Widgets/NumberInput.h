#pragma once

#include "Aquila/UI/Widgets/TextInput.h"

namespace Aquila::UI::Core {

class NumberInput : public TextInput {
  public:
	NumberInput();

	[[nodiscard]] std::string_view GetTypeName() const override { return "NumberInput"; }

	void SetValue(f64 value);
	void SetRange(f64 min, f64 max);
	void SetStep(f64 step);
	void SetPrecision(int decimals);
	Signal<void(f64)> onValueChanged;

	[[nodiscard]] f64 GetValue() const { return m_Value; }

	void OnKeyPress(Platform::KeyCode key, int mods = 0) override;
	void OnFocusLost() override;

  private:
	void CommitText();
	void UpdateDisplayText();

	f64 m_Value = 0.0;
	f64 m_Min = -1e18;
	f64 m_Max = 1e18;
	f64 m_Step = 1.0;
	int m_Precision = 3;
};

} // namespace Aquila::UI::Core
