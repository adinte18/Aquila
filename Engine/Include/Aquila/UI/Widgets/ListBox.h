#pragma once

#include "Aquila/UI/Core/View.h"
#include "Aquila/UI/Widgets/ScrollView.h"
#include "Aquila/UI/Widgets/Button.h"

namespace Aquila::UI::Core {

class ListBox : public View {
  public:
	ListBox();

	[[nodiscard]] std::string_view GetTypeName() const override { return "ListBox"; }

	void AddItem(std::string id, std::string display);
	void RemoveItem(const std::string &id);
	void ClearItems();

	void SetSelectedId(const std::string &id);
	[[nodiscard]] const std::string &GetSelectedId() const { return m_SelectedId; }

	Signal<void(const std::string &)> onSelectionChanged;

  private:
	void SelectItem(const std::string &id);
	void UpdateSelectionStyles();

	struct Item {
		std::string id;
		std::string display;
		Button *button = nullptr;
	};

	ScrollView *m_Scroll = nullptr;
	std::vector<Item> m_Items;
	std::string m_SelectedId;
};

} // namespace Aquila::UI::Core
