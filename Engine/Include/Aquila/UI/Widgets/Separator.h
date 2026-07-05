#pragma once

#include "Aquila/UI/Core/View.h"

namespace Aquila::UI::Core {

class Separator : public View {
  public:
	Separator();
	explicit Separator(bool vertical);

	[[nodiscard]] std::string_view get_type_name() const override { return "Separator"; }

	void set_vertical(bool vertical);
	void apply_xml_attribute(std::string_view name, std::string_view value, void *loader_ctx = nullptr) override;

  private:
	bool m_vertical = false;

	void apply_orientation();
};

} // namespace Aquila::UI::Core
