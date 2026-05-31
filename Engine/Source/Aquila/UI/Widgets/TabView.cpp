#include "Aquila/UI/Widgets/TabView.h"

namespace Aquila::UI::Core {

TabView::TabView() {
	StyleProperties sp;
	sp.flexDirection = FlexDirection::Column;
	sp.width = StyleLength::Grow();
	sp.height = StyleLength::Grow();
	SetStyle(sp);

	auto bar = CreateUnique<View>();
	{
		StyleProperties bp;
		bp.flexDirection = FlexDirection::Row;
		bp.width = StyleLength::Grow();
		bar->SetStyle(bp);
		bar->AddClass("tab-bar");
	}
	m_TabBar = AddChild(std::move(bar));

	auto panels = CreateUnique<View>();
	{
		StyleProperties pp;
		pp.flexDirection = FlexDirection::Column;
		pp.width = StyleLength::Grow();
		pp.flexGrow = 1.f;
		panels->SetStyle(pp);
		panels->AddClass("tab-panels");
	}
	m_Panels = AddChild(std::move(panels));
}

View *TabView::AddTab(std::string title) {
	const int idx = static_cast<int>(m_Tabs.size());

	auto btn = CreateUnique<Button>();
	btn->SetText(title);
	btn->AddClass("tab-button");
	btn->SetOnClick([this, idx] { SetActiveTab(idx); });
	Button *btnRaw = static_cast<Button *>(m_TabBar->AddChild(std::move(btn)));

	auto panel = CreateUnique<View>();
	{
		StyleProperties pp;
		pp.flexDirection = FlexDirection::Column;
		pp.width = StyleLength::Grow();
		pp.flexGrow = 1.f;
		pp.display = Display::None;
		panel->SetStyle(pp);
		panel->AddClass("tab-panel");
	}
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
	if (m_OnTabChanged) {
		m_OnTabChanged(m_ActiveTab);
	}
}

void TabView::SetOnTabChanged(Delegate<void(int)> callback) {
	m_OnTabChanged = std::move(callback);
}

void TabView::ApplyActiveTab() {
	for (int i = 0; i < static_cast<int>(m_Tabs.size()); ++i) {
		const bool active = (i == m_ActiveTab);

		StyleProperties pp;
		pp.display = active ? Display::Flex : Display::None;
		m_Tabs[i].panel->MergeStyle(pp);

		if (active) {
			m_Tabs[i].button->AddClass("tab-active");
		} else {
			m_Tabs[i].button->RemoveClass("tab-active");
		}
	}
}

} // namespace Aquila::UI::Core
