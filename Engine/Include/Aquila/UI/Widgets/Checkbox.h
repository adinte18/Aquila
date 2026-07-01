#pragma once

#include "Aquila/UI/Core/View.h"

namespace Aquila::UI::Core {

class Checkbox : public View {
  public:
	Checkbox();
	explicit Checkbox(bool checked);

	[[nodiscard]] std::string_view GetTypeName() const override { return "Checkbox"; }

	void SetChecked(bool checked);
	[[nodiscard]] bool IsChecked() const { return m_Checked; }

	void SetValue(bool checked) { SetChecked(checked); }
	[[nodiscard]] bool GetValue() const { return m_Checked; }

	Signal<void(bool)> onChanged;

	void OnMouseRelease(Platform::MouseButton btn, vec2 pos) override;
	void OnDrawSelf(Rendering::DrawList &drawList) override;

  private:
	bool m_Checked = false;
};

} // namespace Aquila::UI::Core
