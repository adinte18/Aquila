#pragma once

#include "Aquila/UI/Widgets/BaseField.h"

namespace Aquila::UI::Core {

class Toggle : public BaseField<bool> {
  public:
	Toggle();
	explicit Toggle(bool on);

	[[nodiscard]] std::string_view GetTypeName() const override { return "Toggle"; }

	void SetOn(bool on) { SetValue(on); }
	[[nodiscard]] bool IsOn() const { return GetValue(); }

	void OnMouseRelease(Platform::MouseButton btn, vec2 pos) override;
	void OnDrawSelf(Rendering::DrawList &drawList) override;

  protected:
	void OnValueUpdated() override;
};

} // namespace Aquila::UI::Core
