#include "Aquila/UI/Widgets/IconLabel.h"
#include "Aquila/UI/Core/View.h"
#include "Aquila/UI/Style/StyleProperties.h"
#include "Aquila/UI/Style/StyleTypes.h"

namespace Aquila::UI::Core {

IconLabel::IconLabel() {
	m_should_skip_hit_test = true;

	m_icon = dynamic_cast<Image *>(add_child(std::make_unique<Image>()));
	m_icon->add_class("icon");
	m_label = dynamic_cast<Label *>(add_child(std::make_unique<Label>(std::string())));
	m_spacer = add_child(std::make_unique<View>());
	m_spacer->add_class("icon-label-spacer");
	m_shortcut = dynamic_cast<Label *>(add_child(std::make_unique<Label>(std::string())));
	m_shortcut->add_class("shortcut");
	m_trailing = dynamic_cast<Image *>(add_child(std::make_unique<Image>()));
	m_trailing->add_class("trailing-icon");

	update_icon_visibility();
	update_right_content();
}

IconLabel::IconLabel(std::string text, Text::FontAtlas *font) : IconLabel() {
	m_label->set_text(std::move(text));
	m_label->set_font(font);
}

void IconLabel::set_text(std::string text) {
	m_label->set_text(std::move(text));
}

void IconLabel::set_shortcut(std::string shortcut) {
	m_shortcut->set_text(std::move(shortcut));
	update_right_content();
}

void IconLabel::set_trailing_icon(GFX::GfxTexture *texture) {
	m_trailing->set_texture(texture);
	update_right_content();
}

void IconLabel::set_font(Text::FontAtlas *font) {
	m_label->set_font(font);
}

void IconLabel::set_icon_texture(GFX::GfxTexture *texture) {
	m_icon->set_texture(texture);
	update_icon_visibility();
}

void IconLabel::set_icon_tint(Vec4 tint) {
	m_icon->set_tint(tint);
}

void IconLabel::set_reserve_icon_space(bool reserve) {
	if (m_reserve_icon_space == reserve) {
		return;
	}
	m_reserve_icon_space = reserve;
	update_icon_visibility();
}

void IconLabel::on_style_resolved() {
	View::on_style_resolved();
	if (Text::FontAtlas *font = get_resolved_font()) {
		m_label->set_font(font);
		m_shortcut->set_font(font);
	}
}

void IconLabel::apply_xml_attribute(std::string_view name, std::string_view value, IResourceResolver *resolver) {
	if (name == "src" || name == "uv" || name == "tint") {
		m_icon->apply_xml_attribute(name, value, resolver);
		update_icon_visibility();
		return;
	}
	View::apply_xml_attribute(name, value, resolver);
}

void IconLabel::update_icon_visibility() {
	const bool has_icon = m_icon->get_texture() != nullptr;
	StyleProperties sp;
	sp.display = (has_icon || m_reserve_icon_space) ? Display::Flex : Display::None;
	m_icon->merge_style(sp);
}

void IconLabel::update_right_content() {
	const bool has_shortcut = !m_shortcut->get_text().empty();
	const bool has_trailing = m_trailing->get_texture() != nullptr;
	const bool has_right = has_shortcut || has_trailing;

	StyleProperties shortcut_sp;
	shortcut_sp.display = has_shortcut ? Display::Flex : Display::None;
	m_shortcut->merge_style(shortcut_sp);

	StyleProperties trailing_sp;
	trailing_sp.display = has_trailing ? Display::Flex : Display::None;
	m_trailing->merge_style(trailing_sp);

	StyleProperties spacer_sp;
	spacer_sp.display = has_right ? Display::Flex : Display::None;
	m_spacer->merge_style(spacer_sp);

	StyleProperties self_sp;
	self_sp.width = has_right ? StyleLength::grow() : StyleLength::Auto();
	merge_style(self_sp);
}

} // namespace Aquila::UI::Core
