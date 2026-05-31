#pragma once

#include "Aquila/UI/Core/View.h"

namespace Aquila::UI::Core {

class Toggle : public View {
  public:
	Toggle();
	explicit Toggle(bool on);

	[[nodiscard]] std::string_view GetTypeName() const override { return "Toggle"; }

	void SetOn(bool on);
	[[nodiscard]] bool IsOn() const { return m_On; }
	void SetOnChanged(Delegate<void(bool)> callback);

	void OnMouseRelease(Platform::MouseButton btn, vec2 pos) override;
	void OnDrawSelf(Rendering::DrawList &drawList) override;

  private:
	bool m_On = false;
	Delegate<void(bool)> m_OnChanged;
};

} // namespace Aquila::UI::Core
