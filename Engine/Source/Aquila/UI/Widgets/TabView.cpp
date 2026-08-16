#include "Aquila/UI/Widgets/TabView.h"

namespace Aquila::UI::Core {

TabView::TabView() {
	auto bar = std::make_unique<View>();
	bar->add_class("tab-bar");
	m_tab_bar = add_child(std::move(bar));

	auto panels = std::make_unique<View>();
	panels->add_class("tab-panels");
	m_panels = add_child(std::move(panels));
}

View *TabView::add_tab(std::string title, GFX::GfxTexture *icon) {
	const int idx = static_cast<int>(m_tabs.size());

	auto btn = std::make_unique<Button>();
	btn->set_text(title);
	if (icon != nullptr) {
		btn->set_icon(icon);
	}
	btn->add_class("tab-button");
	btn->on_click.connect([this, idx] { set_active_tab(idx); });
	Button *btn_raw = dynamic_cast<Button *>(m_tab_bar->add_child(std::move(btn)));

	auto panel = std::make_unique<View>();
	panel->add_class("tab-panel");
	View *panel_raw = m_panels->add_child(std::move(panel));

	m_tabs.push_back({ btn_raw, panel_raw });

	if (m_active_tab < 0) {
		set_active_tab(0);
	}

	return panel_raw;
}

void TabView::set_active_tab(int index) {
	if (index < 0 || index >= static_cast<int>(m_tabs.size())) {
		return;
	}
	m_active_tab = index;
	apply_active_tab();
	on_tab_changed(m_active_tab);
}

void TabView::apply_active_tab() {
	for (int i = 0; i < static_cast<int>(m_tabs.size()); ++i) {
		const bool active = (i == m_active_tab);

		m_tabs[i].panel->set_hidden(!active);

		if (active) {
			m_tabs[i].button->add_class("tab-active");
		} else {
			m_tabs[i].button->remove_class("tab-active");
		}
	}
}

} // namespace Aquila::UI::Core
