#pragma once

#include "Aquila/UI/Core/View.h"

namespace Aquila::UI::Core {

class Control : public View {
  public:
	[[nodiscard]] std::string_view get_type_name() const override { return "Control"; }

	void set_focusable(bool focusable) { m_focusable = focusable; }
	[[nodiscard]] bool is_focusable() const { return m_focusable; }

	void set_tab_index(int index) { m_tab_index = index; }
	[[nodiscard]] int get_tab_index() const { return m_tab_index; }

  protected:
	bool m_focusable = true;
	int m_tab_index = -1;
};

} // namespace Aquila::UI::Core
