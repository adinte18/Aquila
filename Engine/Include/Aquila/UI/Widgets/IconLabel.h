#pragma once

#include "Aquila/UI/Widgets/Label.h"
#include "Aquila/UI/Widgets/Image.h"

namespace Aquila::UI::Core {

class IconLabel : public View {
  public:
	IconLabel();
	explicit IconLabel(std::string text, Text::FontAtlas *font = nullptr);

	[[nodiscard]] std::string_view get_type_name() const override { return "IconLabel"; }

	void set_text(std::string text);
	void set_font(Text::FontAtlas *font) override;

	void set_icon_texture(GFX::GfxTexture *texture);
	void set_icon_tint(Vec4 tint);

	[[nodiscard]] Label *get_label() const { return m_label; }
	[[nodiscard]] Image *get_icon() const { return m_icon; }

	void on_style_resolved() override;
	void apply_xml_text_content(std::string_view text) override { set_text(std::string(text)); }
	void apply_xml_attribute(std::string_view name, std::string_view value, void *loader_ctx = nullptr) override;

  private:
	void update_icon_visibility();

	Image *m_icon = nullptr;
	Label *m_label = nullptr;
};

} // namespace Aquila::UI::Core
