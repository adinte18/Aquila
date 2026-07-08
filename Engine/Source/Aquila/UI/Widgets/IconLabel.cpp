#include "Aquila/UI/Widgets/IconLabel.h"
#include "Aquila/UI/Style/StyleProperties.h"
#include "Aquila/UI/Style/StyleTypes.h"

namespace Aquila::UI::Core {

IconLabel::IconLabel() {
	m_should_skip_hit_test = true;

	m_icon = dynamic_cast<Image *>(add_child(std::make_unique<Image>()));
	m_icon->add_class("icon");
	m_label = dynamic_cast<Label *>(add_child(std::make_unique<Label>(std::string())));

	update_icon_visibility();
}

IconLabel::IconLabel(std::string text, Text::FontAtlas *font) : IconLabel() {
	m_label->set_text(std::move(text));
	m_label->set_font(font);
}

void IconLabel::set_text(std::string text) {
	m_label->set_text(std::move(text));
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

void IconLabel::on_style_resolved() {
	View::on_style_resolved();
	if (Text::FontAtlas *font = get_resolved_font()) {
		m_label->set_font(font);
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
	StyleProperties sp;
	sp.display = (m_icon->get_texture() != nullptr) ? Display::Flex : Display::None;
	m_icon->merge_style(sp);
}

} // namespace Aquila::UI::Core
