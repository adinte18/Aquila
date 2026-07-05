#pragma once

#include "Aquila/UI/Widgets/DragFloat.h"

namespace Aquila::UI::Core {

class DragInt : public DragFloat {
  public:
	DragInt();

	[[nodiscard]] std::string_view get_type_name() const override { return "DragInt"; }

	void set_int_value(int value) { set_value(static_cast<float>(value)); }
	[[nodiscard]] int get_int_value() const { return static_cast<int>(std::round(get_value())); }
	void set_int_range(int min, int max) { set_range(static_cast<float>(min), static_cast<float>(max)); }

  protected:
	[[nodiscard]] std::string format_value() const override;
	void on_value_committed() override;
};

} // namespace Aquila::UI::Core
