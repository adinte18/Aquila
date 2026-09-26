#pragma once

#include "Aquila/UI/Core/View.h"

namespace Editor {

class ViewportCanvas : public Aquila::UI::Core::View {
  public:
	explicit ViewportCanvas(Int16 z_index);

	virtual void draw(Aquila::UI::Rendering::DrawList &draw_list) const = 0;

  protected:
	void fit(const Rect &viewport);
	void redraw();
	[[nodiscard]] Aquila::UI::Text::FontAtlas *font() const;
	[[nodiscard]] F32 font_size() const;

  private:
	Aquila::UI::Core::View *m_layer = nullptr;
};

}
