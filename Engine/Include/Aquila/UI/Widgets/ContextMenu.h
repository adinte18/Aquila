#pragma once

#include "Aquila/UI/Widgets/FloatingOverlay.h"
#include "Aquila/UI/Widgets/Button.h"

namespace Aquila::UI::Core {

class ContextMenu : public FloatingOverlay {
  public:
	ContextMenu();

	[[nodiscard]] std::string_view get_type_name() const override { return "ContextMenu"; }

	void add_item(std::string text, Delegate<void()> callback);
	void clear_items();
	void open_at(Vec2 canvas_pos);

  private:
	struct Item {
		std::string text;
		Delegate<void()> callback;
	};

	void rebuild();

	std::vector<Item> m_items;
	std::vector<View *> m_item_views;
};

} // namespace Aquila::UI::Core
