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

	[[nodiscard]] Vec2 measure(const std::string &text);

	// A tooltip is display-only; it must never capture hover (which would flicker it).
	View *hit_test_absolute(Vec2 /*canvas_pos*/) override { return nullptr; }

  private:
	Label *m_label = nullptr;
};

} // namespace Aquila::UI::Core
