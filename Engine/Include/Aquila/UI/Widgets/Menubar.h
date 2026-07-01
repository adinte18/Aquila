#pragma once

#include "Aquila/UI/Core/View.h"
#include "Aquila/UI/Widgets/Button.h"
#include "Aquila/UI/Widgets/FloatingOverlay.h"
#include "Aquila/UI/Widgets/Separator.h"

namespace Aquila::UI::Core {

class MenuDropdown;
class MenuBar;

class MenuDropdown : public FloatingOverlay {
  public:
	explicit MenuDropdown(MenuBar *owner);

	[[nodiscard]] std::string_view GetTypeName() const override { return "MenuDropdown"; }

	void AddItem(std::string text, Delegate<void()> callback);
	void AddSeparator();
	void ClearItems();
	void OpenBelow(vec2 buttonAbsPos, float buttonHeight);

  private:
	struct Item {
		std::string text;
		Delegate<void()> callback;
		bool isSeparator = false;
	};

	void Rebuild();

	MenuBar *m_Owner = nullptr;
	std::vector<Item> m_Items;
	std::vector<View *> m_ItemViews;
};

class MenuBar : public View {
  public:
	MenuBar();

	[[nodiscard]] std::string_view GetTypeName() const override { return "MenuBar"; }

	MenuDropdown *AddMenu(std::string title);
	void CloseAll();
	void OpenDropdown(MenuDropdown *dropdown, vec2 buttonAbsPos, float buttonHeight);
	void SetOverlayRoot(View *root);

  private:
	struct Entry {
		Button *button = nullptr;
		MenuDropdown *dropdown = nullptr;
	};

	std::vector<Entry> m_Entries;
	MenuDropdown *m_OpenDropdown = nullptr;
	View *m_OverlayRoot = nullptr;
};

} // namespace Aquila::UI::Core
