#include "Aquila/UI/Widgets/DockPanel.h"
#include "Aquila/UI/Core/LayoutLoader.h"

namespace Aquila::UI::Core {

DockPanel::DockPanel(std::string title) : m_Title(std::move(title)) {
	AddClass("dock-panel");
}

void DockPanel::SetTitle(std::string title) {
	m_Title = std::move(title);
}

void DockPanel::ApplyXmlAttribute(std::string_view name, std::string_view value, void *loaderCtx) {
	if (name == "src" || name == "icon") {
		if (auto *loader = static_cast<LayoutLoader *>(loaderCtx)) {
			if (GFX::GfxTexture *tex = loader->ResolveTexture(std::string(value))) {
				m_TabIcon = tex;
			}
		}
		return;
	}
	View::ApplyXmlAttribute(name, value, loaderCtx);
}

} // namespace Aquila::UI::Core
