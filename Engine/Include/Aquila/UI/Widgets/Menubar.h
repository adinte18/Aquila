#pragma once

#include "Aquila/UI/Core/View.h"
#include "Aquila/UI/Widgets/Button.h"
#include "Aquila/UI/Widgets/PopupMenu.h"

namespace Aquila::UI::Core {

class MenuBar : public View {
  public:
	MenuBar();

	[[nodiscard]] std::string_view get_type_name() const override { return "MenuBar"; }

	PopupMenu *add_menu(std::string title);
	void close_all();
	void open_dropdown(PopupMenu *dropdown, Vec2 button_abs_pos, float button_height);
	void set_overlay_root(View *root);

  private:
	struct Entry {
		Button *button = nullptr;
		PopupMenu *dropdown = nullptr;
	};

	std::vector<Entry> m_entries;
	PopupMenu *m_open_dropdown = nullptr;
	View *m_overlay_root = nullptr;
};

} // namespace Aquila::UI::Core
