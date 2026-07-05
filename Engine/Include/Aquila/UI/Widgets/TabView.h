#pragma once

#include "Aquila/UI/Core/View.h"
#include "Aquila/UI/Widgets/Button.h"
#include <string>
#include <vector>

namespace Aquila::UI::Core {

class TabView : public View {
  public:
	TabView();

	[[nodiscard]] std::string_view GetTypeName() const override { return "TabView"; }

	View *AddTab(std::string title, GFX::GfxTexture *icon = nullptr);

	void SetActiveTab(int index);
	[[nodiscard]] int GetActiveTab() const { return m_ActiveTab; }
	Signal<void(int)> onTabChanged;

  private:
	void ApplyActiveTab();

	struct Tab {
		Button *button = nullptr;
		View *panel = nullptr;
	};

	View *m_TabBar = nullptr;
	View *m_Panels = nullptr;
	std::vector<Tab> m_Tabs;
	int m_ActiveTab = -1;
};

} // namespace Aquila::UI::Core
