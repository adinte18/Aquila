#pragma once

#include "Aquila/UI/Core/View.h"

namespace Aquila::UI::Core {

class Checkbox : public View {
  public:
	Checkbox();
	explicit Checkbox(bool checked);

	[[nodiscard]] std::string_view GetTypeName() const override { return "Checkbox"; }

	void SetChecked(bool checked);
	void SetOnChanged(Delegate<void(bool)> callback);
	[[nodiscard]] bool IsChecked() const { return m_Checked; }

	void OnMouseEnter() override;
	void OnMouseLeave() override;
	void OnMousePress(Platform::MouseButton btn, vec2 pos) override;
	void OnMouseRelease(Platform::MouseButton btn, vec2 pos) override;
	void OnDrawSelf(Rendering::DrawList &drawList) override;

  private:
	bool m_Checked = false;
	Delegate<void(bool)> m_OnChanged;
};

} // namespace Aquila::UI::Core
