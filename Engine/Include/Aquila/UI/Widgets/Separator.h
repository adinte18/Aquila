#pragma once

#include "Aquila/UI/Core/View.h"

namespace Aquila::UI::Core {

class Separator : public View {
  public:
	Separator();
	explicit Separator(bool vertical);

	[[nodiscard]] std::string_view GetTypeName() const override { return "Separator"; }

	void SetVertical(bool vertical);
	void ApplyXmlAttribute(std::string_view name, std::string_view value, void *loaderCtx = nullptr) override;

  private:
	bool m_Vertical = false;

	void ApplyOrientation();
};

} // namespace Aquila::UI::Core
