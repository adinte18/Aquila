#pragma once

#include "Aquila/UI/Core/View.h"

namespace Aquila::UI::Core {

class Control : public View {
  public:
	[[nodiscard]] std::string_view GetTypeName() const override { return "Control"; }

	void SetFocusable(bool focusable) { m_Focusable = focusable; }
	[[nodiscard]] bool IsFocusable() const { return m_Focusable; }

	void SetTabIndex(int index) { m_TabIndex = index; }
	[[nodiscard]] int GetTabIndex() const { return m_TabIndex; }

  protected:
	bool m_Focusable = true;
	int m_TabIndex = -1;
};

} // namespace Aquila::UI::Core
