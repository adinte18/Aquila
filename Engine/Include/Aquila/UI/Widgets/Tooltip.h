#pragma once

#include "Aquila/UI/Widgets/FloatingOverlay.h"
#include "Aquila/UI/Widgets/Label.h"

namespace Aquila::UI::Core {

class Tooltip : public FloatingOverlay {
  public:
	Tooltip();

	[[nodiscard]] std::string_view GetTypeName() const override { return "Tooltip"; }

	void ShowAt(vec2 canvasPos, std::string text);
	void Hide();

  private:
	Label *m_Label = nullptr;
};

} // namespace Aquila::UI::Core
