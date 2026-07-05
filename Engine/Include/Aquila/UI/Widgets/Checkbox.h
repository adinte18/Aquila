#pragma once

#include "Aquila/UI/Widgets/BaseField.h"

namespace Aquila::UI::Core {

class Checkbox : public BaseField<bool> {
  public:
	Checkbox();
	explicit Checkbox(bool checked);

	[[nodiscard]] std::string_view GetTypeName() const override { return "Checkbox"; }

	void SetChecked(bool checked) { SetValue(checked); }
	[[nodiscard]] bool IsChecked() const { return GetValue(); }

	void OnMouseRelease(Platform::MouseButton btn, vec2 pos) override;
	void OnDrawSelf(Rendering::DrawList &drawList) override;

  protected:
	void OnValueUpdated() override;
};

} // namespace Aquila::UI::Core
