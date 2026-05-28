#pragma once

#include "Aquila/UI/Core/View.h"
#include "Aquila/UI/Widgets/Button.h"
#include <string>
#include <vector>

namespace Aquila::UI::Core {

class ContextMenu : public View {
  public:
	ContextMenu();

	[[nodiscard]] std::string_view GetTypeName() const override { return "ContextMenu"; }

	void AddItem(std::string text, Delegate<void()> callback);
	void ClearItems();
	void OpenAt(vec2 canvasPos);
	void Close();
	[[nodiscard]] bool IsOpen() const { return m_Open; }

	View *HitTestAbsolute(vec2 canvasPos) override;

  private:
	struct Item {
		std::string text;
		Delegate<void()> callback;
	};

	void Rebuild();
	void ApplyDisplayState();

	bool m_Open = false;
	std::vector<Item> m_Items;
	std::vector<View *> m_ItemViews;
	Button *m_Backdrop = nullptr;
};

} // namespace Aquila::UI::Core
