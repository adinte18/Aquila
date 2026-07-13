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
	void set_shortcut(std::string shortcut);
	void set_icon_texture(GFX::GfxTexture *texture);
	void set_trailing_icon(GFX::GfxTexture *texture);
	void set_icon_tint(Vec4 tint);

	// When true, the icon slot keeps its layout box (and CSS size) even with no texture,
	// so text aligns across items that have icons and items that don't. Default false.
	void set_reserve_icon_space(bool reserve);

	[[nodiscard]] Label *get_label() const { return m_label; }
	[[nodiscard]] Label *get_shortcut() const { return m_shortcut; }
	[[nodiscard]] Image *get_icon() const { return m_icon; }

	void on_style_resolved() override;
	void apply_xml_text_content(std::string_view text) override { set_text(std::string(text)); }
	void apply_xml_attribute(std::string_view name, std::string_view value,
							 IResourceResolver *resolver = nullptr) override;

  private:
	void update_icon_visibility();
	void update_right_content();

	Image *m_icon = nullptr;
	Label *m_label = nullptr;
	View *m_spacer = nullptr;
	Label *m_shortcut = nullptr;
	Image *m_trailing = nullptr;
	bool m_reserve_icon_space = false;
};

} // namespace Aquila::UI::Core
