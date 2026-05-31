#include "Aquila/UI/Widgets/DockPanel.h"

namespace Aquila::UI::Core {

DockPanel::DockPanel(std::string title) : m_Title(std::move(title)) {
	StyleProperties sp;
	sp.flexDirection = FlexDirection::Column;
	sp.width = StyleLength::Grow();
	sp.height = StyleLength::Grow();
	sp.flexGrow = 1.f;
	sp.display = Display::None;
	SetStyle(sp);
	AddClass("dock-panel");
}

void DockPanel::SetTitle(std::string title) {
	m_Title = std::move(title);
}

} // namespace Aquila::UI::Core
