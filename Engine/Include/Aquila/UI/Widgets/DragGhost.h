#pragma once

#include "Aquila/UI/Widgets/IconLabel.h"

namespace Aquila::UI::Core {

class DragGhost : public IconLabel {
  public:
	DragGhost();

	[[nodiscard]] std::string_view get_type_name() const override { return "DragGhost"; }

	void show(std::string label, Vec2 pos, GFX::GfxTexture *icon = nullptr);
	void move_to(Vec2 pos);
	void hide();

  private:
	void set_position(Vec2 pos);
};

} // namespace Aquila::UI::Core
