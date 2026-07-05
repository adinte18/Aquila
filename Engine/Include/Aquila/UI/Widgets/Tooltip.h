#pragma once

#include "Aquila/UI/Widgets/FloatingOverlay.h"
#include "Aquila/UI/Widgets/Label.h"

namespace Aquila::UI::Core {

class Tooltip : public FloatingOverlay {
  public:
	Tooltip();

	[[nodiscard]] std::string_view get_type_name() const override { return "Tooltip"; }

	void show_at(Vec2 canvas_pos, std::string text);
	void hide();

  private:
	Label *m_label = nullptr;
};

} // namespace Aquila::UI::Core
