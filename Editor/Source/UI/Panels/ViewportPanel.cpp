#include "UI/Panels/ViewportPanel.h"

#include "Aquila/Foundation/Macros.h"
#include "Aquila/UI/Widgets/DockPanel.h"
#include "Aquila/UI/Widgets/Image.h"

namespace Editor {

using namespace Aquila;

ViewportPanel::ViewportPanel(GFX::GfxTexture &initialTexture) : m_InitialTexture(initialTexture) {}

void ViewportPanel::Build(UI::Core::DockPanel *panel, UI::Core::View *) {
	auto imgUniq = CreateUnique<UI::Core::Image>();
	auto *img = static_cast<UI::Core::Image *>(panel->AddChild(std::move(imgUniq)));
	img->SetId("viewport");
	img->SetTexture(&m_InitialTexture);
	img->SetPassThroughScroll(true);

	UI::StyleProperties sp;
	sp.width = UI::StyleLength::Grow();
	sp.height = UI::StyleLength::Grow();
	sp.flexGrow = 1.f;
	img->SetStyle(sp);

	m_Image = img;
}

void ViewportPanel::SetTexture(GFX::GfxTexture *texture) {
	if (m_Image) {
		m_Image->SetTexture(texture);
	}
}

} // namespace Editor
