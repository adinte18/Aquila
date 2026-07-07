#include "Aquila/UI/Widgets/Button.h"
#include "Aquila/UI/Core/LayoutLoader.h"
#include "Aquila/Foundation/Macros.h"

namespace Aquila::UI::Core {

Button::Button() {
	set_input_leaf(true);
}

Button::Button(std::string text, Text::FontAtlas *font) {
	set_input_leaf(true);
	m_content = dynamic_cast<IconLabel *>(add_child(std::make_unique<IconLabel>(std::move(text), font)));
}

void Button::ensure_content() {
	if (m_content == nullptr) {
		m_content = static_cast<IconLabel *>(add_child(std::make_unique<IconLabel>()));
	}
}

void Button::set_text(std::string text) {
	ensure_content();
	m_content->set_text(std::move(text));
}

void Button::set_font(Text::FontAtlas *font) {
	if (m_content == nullptr) {
		return;
	}
	m_content->set_font(font);
}

void Button::set_icon(GFX::GfxTexture *texture) {
	ensure_content();
	m_content->set_icon_texture(texture);
}

void Button::on_mouse_release(Platform::MouseButton btn, Vec2 pos) {
	View::on_mouse_release(btn, pos);
	if (btn == Platform::MouseButton::Left && m_is_hovered) {
		on_click();
	}
}

void Button::on_style_resolved() {
	View::on_style_resolved();
	if (m_content == nullptr) {
		return;
	}
	if (Text::FontAtlas *font = get_resolved_font()) {
		m_content->set_font(font);
	}
}

void Button::apply_xml_attribute(std::string_view name, std::string_view value, void *loader_ctx) {
	if (name == "src" || name == "icon" || name == "bank" || name == "uv" || name == "tint") {
		ensure_content();
		m_content->apply_xml_attribute(name, value, loader_ctx);
		return;
	}
	if (name == "on-click") {
		if (auto *loader = static_cast<LayoutLoader *>(loader_ctx)) {
			if (Delegate<void()> command = loader->resolve_command(std::string(value))) {
				on_click.connect(std::move(command));
			} else {
				AQUILA_LOG_WARNING("Button: on-click references unknown command '{}'", value);
			}
		}
		return;
	}
	View::apply_xml_attribute(name, value, loader_ctx);
}

} // namespace Aquila::UI::Core
