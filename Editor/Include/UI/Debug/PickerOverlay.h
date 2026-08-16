#pragma once

#include "Aquila/UI/Core/View.h"
#include "Aquila/Foundation/Math/Rect.h"

#include <string>

namespace Aquila::UI::Rendering {
class DrawList;
}
namespace Aquila::UI::Text {
class FontAtlas;
}

namespace Editor {

// Full-canvas, hit-test-transparent overlay that draws a highlight box around a
// picked view. Driven by the element picker; sits above all editor content.
class PickerOverlay : public Aquila::UI::Core::View {
  public:
	PickerOverlay();

	[[nodiscard]] std::string_view get_type_name() const override { return "PickerOverlay"; }

	void set_target(const Rect &rect, std::string label = "");
	void clear();

	void on_draw_self(Aquila::UI::Rendering::DrawList &draw_list) override;

  private:
	bool m_active = false;
	Rect m_target;
	std::string m_label;
	Aquila::UI::Text::FontAtlas *m_font = nullptr;
};

} // namespace Editor
