#pragma once

#include "Aquila/UI/Core/View.h"
#include <string>

namespace Aquila::GFX {
class GfxTexture;
}

namespace Aquila::UI::Core {

class DockPanel : public View {
  public:
	explicit DockPanel(std::string title = "");

	[[nodiscard]] std::string_view get_type_name() const override { return "DockPanel"; }
	[[nodiscard]] const std::string &get_title() const { return m_title; }
	void set_title(std::string title);

	[[nodiscard]] GFX::GfxTexture *get_tab_icon() const { return m_tab_icon; }
	void set_tab_icon(GFX::GfxTexture *icon) { m_tab_icon = icon; }

	void apply_xml_text_content(std::string_view text) override { set_title(std::string(text)); }
	void apply_xml_attribute(std::string_view name, std::string_view value, void *loader_ctx = nullptr) override;

  private:
	std::string m_title;
	GFX::GfxTexture *m_tab_icon = nullptr;
};

} // namespace Aquila::UI::Core
