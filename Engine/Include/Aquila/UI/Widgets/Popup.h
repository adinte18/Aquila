#pragma once

#include "Aquila/UI/Widgets/FloatingOverlay.h"

namespace Aquila::UI::Core {

class Popup : public FloatingOverlay {
  public:
	Popup();

	[[nodiscard]] std::string_view GetTypeName() const override { return "Popup"; }
};

} // namespace Aquila::UI::Core
