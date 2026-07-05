#pragma once

#include "Aquila/UI/Core/View.h"

namespace Aquila::UI::Core {

class ProgressBar : public View {
  public:
	ProgressBar();

	[[nodiscard]] std::string_view get_type_name() const override { return "ProgressBar"; }

	void set_value(float value);
	[[nodiscard]] float get_value() const { return m_value; }

	void apply_xml_attribute(std::string_view name, std::string_view value, void *loader_ctx = nullptr) override;

  private:
	float m_value = 0.F;
	View *m_fill = nullptr;
};

} // namespace Aquila::UI::Core
