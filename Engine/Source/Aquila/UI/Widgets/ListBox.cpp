#include "Aquila/UI/Widgets/ListBox.h"

namespace Aquila::UI::Core {

ListBox::ListBox() {
	AddClass("list-box");

	StyleProperties sp;
	sp.flexDirection = FlexDirection::Column;
	sp.width = StyleLength::Grow();
	sp.height = StyleLength::Grow();
	MergeStyle(sp);

	auto scroll = CreateUnique<ScrollView>();
	scroll->AddClass("list-box-scroll");
	m_Scroll = static_cast<ScrollView *>(AddChild(std::move(scroll)));
}

void ListBox::AddItem(std::string id, std::string display) {
	auto btn = CreateUnique<Button>(display);
	btn->AddClass("list-item");
	btn->onClick.Connect([this, id] { SelectItem(id); });

	Button *btnPtr = static_cast<Button *>(m_Scroll->AddContent(std::move(btn)));
	m_Items.push_back({ std::move(id), std::move(display), btnPtr });
}

void ListBox::RemoveItem(const std::string &id) {
	auto it = std::ranges::find_if(m_Items, [&](const Item &item) { return item.id == id; });
	if (it == m_Items.end()) {
		return;
	}

	if (it->button->GetParent()) {
		it->button->GetParent()->RemoveChild(it->button);
	}

	if (m_SelectedId == it->id) {
		m_SelectedId.clear();
	}
	m_Items.erase(it);
}

void ListBox::ClearItems() {
	for (auto &item : m_Items) {
		View *parent = item.button->GetParent();
		if (parent) {
			parent->RemoveChild(item.button);
		}
	}
	m_Items.clear();
	m_SelectedId.clear();
}

void ListBox::SetSelectedId(const std::string &id) {
	m_SelectedId = id;
	UpdateSelectionStyles();
}


void ListBox::SelectItem(const std::string &id) {
	if (m_SelectedId == id) {
		return;
	}
	m_SelectedId = id;
	UpdateSelectionStyles();
	onSelectionChanged(m_SelectedId);
}

void ListBox::UpdateSelectionStyles() {
	for (auto &item : m_Items) {
		if (item.id == m_SelectedId) {
			item.button->AddClass("selected");
		} else {
			item.button->RemoveClass("selected");
		}
	}
}

} // namespace Aquila::UI::Core
