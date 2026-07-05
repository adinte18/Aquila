#include "UI/Panels/ViewportPanel.h"

#include "Aquila/Foundation/Macros.h"
#include "Aquila/UI/Widgets/DockPanel.h"
#include "Aquila/UI/Widgets/Image.h"

namespace Editor {

using namespace Aquila;

ViewportPanel::ViewportPanel(GFX::GfxTexture &initialTexture) : m_InitialTexture(initialTexture) {}

void ViewportPanel::Build(UI::Core::DockPanel *panel, UI::Core::View *) {
	m_Image = panel->FindById<UI::Core::Image>("viewport");
	if (m_Image == nullptr) {
		AQUILA_LOG_ERROR("ViewportPanel: 'viewport' image not found in layout");
		return;
	}

	m_Image->SetTexture(&m_InitialTexture);
	m_Image->SetPassThroughScroll(true);
}

void ViewportPanel::SetTexture(GFX::GfxTexture *texture) {
	if (m_Image) {
		m_Image->SetTexture(texture);
	}
}

} // namespace Editor
