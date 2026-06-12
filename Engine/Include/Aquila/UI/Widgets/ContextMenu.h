#pragma once

#include "Aquila/UI/Widgets/FloatingOverlay.h"
#include "Aquila/UI/Widgets/Button.h"

namespace Aquila::UI::Core {

class ContextMenu : public FloatingOverlay {
  public:
	ContextMenu();

	[[nodiscard]] std::string_view GetTypeName() const override { return "ContextMenu"; }

	void AddItem(std::string text, Delegate<void()> callback);
	void ClearItems();
	void OpenAt(vec2 canvasPos);

  private:
	struct Item {
		std::string text;
		Delegate<void()> callback;
	};

	void Rebuild();

	std::vector<Item> m_Items;
	std::vector<View *> m_ItemViews;
};

} // namespace Aquila::UI::Core
