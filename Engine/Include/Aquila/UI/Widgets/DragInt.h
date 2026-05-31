#pragma once

#include "Aquila/UI/Widgets/DragFloat.h"

namespace Aquila::UI::Core {

class DragInt : public DragFloat {
  public:
	DragInt();

	[[nodiscard]] std::string_view GetTypeName() const override { return "DragInt"; }

	void SetIntValue(int value) { SetValue(static_cast<float>(value)); }
	[[nodiscard]] int GetIntValue() const { return static_cast<int>(std::round(GetValue())); }
	void SetIntRange(int min, int max) { SetRange(static_cast<float>(min), static_cast<float>(max)); }

  protected:
	[[nodiscard]] std::string FormatValue() const override;
	void OnValueCommitted() override;
};

} // namespace Aquila::UI::Core
