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

	[[nodiscard]] std::string_view get_type_name() const override { return "MenuDropdown"; }

	void add_item(std::string text, Delegate<void()> callback);
	void add_separator();
	void clear_items();
	void open_below(Vec2 button_abs_pos, float button_height);

  private:
	struct Item {
		std::string text;
		Delegate<void()> callback;
		bool is_separator = false;
	};

	void rebuild();

	MenuBar *m_owner = nullptr;
	std::vector<Item> m_items;
	std::vector<View *> m_item_views;
};

class MenuBar : public View {
  public:
	MenuBar();

	[[nodiscard]] std::string_view get_type_name() const override { return "MenuBar"; }

	MenuDropdown *add_menu(std::string title);
	void close_all();
	void open_dropdown(MenuDropdown *dropdown, Vec2 button_abs_pos, float button_height);
	void set_overlay_root(View *root);

  private:
	struct Entry {
		Button *button = nullptr;
		MenuDropdown *dropdown = nullptr;
	};

	std::vector<Entry> m_entries;
	MenuDropdown *m_open_dropdown = nullptr;
	View *m_overlay_root = nullptr;
};

} // namespace Aquila::UI::Core
