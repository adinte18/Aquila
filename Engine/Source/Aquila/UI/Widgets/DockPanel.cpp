#include "Aquila/UI/Widgets/DockPanel.h"
#include "Aquila/UI/Core/LayoutLoader.h"

namespace Aquila::UI::Core {

DockPanel::DockPanel(std::string title) : m_title(std::move(title)) {
	add_class("dock-panel");
}

void DockPanel::set_title(std::string title) {
	m_title = std::move(title);
}

void DockPanel::apply_xml_attribute(std::string_view name, std::string_view value, void *loader_ctx) {
	if (name == "src" || name == "icon") {
		if (auto *loader = static_cast<LayoutLoader *>(loader_ctx)) {
			if (GFX::GfxTexture *tex = loader->resolve_texture(std::string(value))) {
				m_tab_icon = tex;
			}
		}
		return;
	}
	View::apply_xml_attribute(name, value, loader_ctx);
}

} // namespace Aquila::UI::Core
