#include "Aquila/UI/Widgets/TabView.h"

namespace Aquila::UI::Core {

TabView::TabView() {
	auto bar = CreateUnique<View>();
	bar->AddClass("tab-bar");
	m_TabBar = AddChild(std::move(bar));

	auto panels = CreateUnique<View>();
	panels->AddClass("tab-panels");
	m_Panels = AddChild(std::move(panels));
}

View *TabView::AddTab(std::string title, GFX::GfxTexture *icon) {
	const int idx = static_cast<int>(m_Tabs.size());

	auto btn = CreateUnique<Button>();
	btn->SetText(title);
	if (icon != nullptr) {
		btn->SetIcon(icon);
	}
	btn->AddClass("tab-button");
	btn->onClick.Connect([this, idx] { SetActiveTab(idx); });
	Button *btnRaw = static_cast<Button *>(m_TabBar->AddChild(std::move(btn)));

	auto panel = CreateUnique<View>();
	panel->AddClass("tab-panel");
	View *panelRaw = m_Panels->AddChild(std::move(panel));

	m_Tabs.push_back({ btnRaw, panelRaw });

	if (m_ActiveTab < 0) {
		SetActiveTab(0);
	}

	return panelRaw;
}

void TabView::SetActiveTab(int index) {
	if (index < 0 || index >= static_cast<int>(m_Tabs.size())) {
		return;
	}
	m_ActiveTab = index;
	ApplyActiveTab();
	onTabChanged(m_ActiveTab);
}


void TabView::ApplyActiveTab() {
	for (int i = 0; i < static_cast<int>(m_Tabs.size()); ++i) {
		const bool active = (i == m_ActiveTab);

		m_Tabs[i].panel->SetHidden(!active);

		if (active) {
			m_Tabs[i].button->AddClass("tab-active");
		} else {
			m_Tabs[i].button->RemoveClass("tab-active");
		}
	}
}

} // namespace Aquila::UI::Core
