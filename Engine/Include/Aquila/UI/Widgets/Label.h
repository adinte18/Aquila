#pragma once

#include "Aquila/UI/Core/View.h"
#include "Aquila/UI/Text/FontAtlas.h"

namespace Aquila::UI::Core {

class Label : public View {
  public:
	Label();
	explicit Label(std::string text, Text::FontAtlas *font = nullptr);

	[[nodiscard]] std::string_view get_type_name() const override { return "Label"; }

	void set_text(std::string text);
	void set_font(Text::FontAtlas *font) override;

	[[nodiscard]] Vec2 measure(float override_font_size = 0.F) const;

	[[nodiscard]] Text::FontAtlas *resolve_font() const;

	Vec2 get_intrinsic_size() const override;

	[[nodiscard]] const std::string &get_text() const { return m_text; }
	[[nodiscard]] Text::FontAtlas *get_font() const { return m_font; }

	void on_draw_self(Rendering::DrawList &draw_list) override;
	void apply_xml_text_content(std::string_view text) override { set_text(std::string(text)); }

  private:
	std::string m_text;
	Text::FontAtlas *m_font = nullptr;
};

} // namespace Aquila::UI::Core
