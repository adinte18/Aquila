#include "Aquila/UI/Widgets/Menubar.h"

namespace Aquila::UI::Core {

MenuDropdown::MenuDropdown(MenuBar *owner) : FloatingOverlay(10), m_Owner(owner) {
	AddClass("menu-dropdown");
	SetDismissOnClickAway(true);

	StyleProperties sp;
	sp.flexDirection = FlexDirection::Column;
	sp.minWidth = StyleLength::Pixel(140.f);
	MergeStyle(sp);

	FloatingConfig fc;
	fc.attachTo = FloatingAttachTo::Root;
	fc.elementPoint = FloatingAttachPoint::LeftTop;
	fc.parentPoint = FloatingAttachPoint::LeftTop;
	fc.zIndex = 50;
	SetFloating(fc);
}

void MenuDropdown::AddItem(std::string text, Delegate<void()> callback) {
	m_Items.push_back({ std::move(text), std::move(callback), false });
}

void MenuDropdown::AddSeparator() {
	m_Items.push_back({ {}, {}, true });
}

void MenuDropdown::ClearItems() {
	for (View *v : m_ItemViews) {
		RemoveChild(v);
	}
	m_ItemViews.clear();
	m_Items.clear();
}

void MenuDropdown::OpenBelow(vec2 buttonAbsPos, float buttonHeight) {
	FloatingConfig fc = GetFloating();
	fc.offset = { buttonAbsPos.x, buttonAbsPos.y + buttonHeight };
	SetFloating(fc);

	Rebuild();
	Open();
}

void MenuDropdown::Rebuild() {
	for (View *v : m_ItemViews) {
		RemoveChild(v);
	}
	m_ItemViews.clear();

	for (auto &item : m_Items) {
		if (item.isSeparator) {
			auto sep = CreateUnique<Separator>();
			sep->AddClass("menu-separator");
			m_ItemViews.push_back(AddChild(std::move(sep)));
			continue;
		}

		auto btn = CreateUnique<Button>();
		btn->SetText(item.text);
		btn->AddClass("menu-item");
		btn->onClick.Connect([this, cb = item.callback] {
			if (cb) {
				cb();
			}
			if (m_Owner) {
				m_Owner->CloseAll();
			}
		});
		m_ItemViews.push_back(AddChild(std::move(btn)));
	}

	InvalidateLayout();
}

MenuBar::MenuBar() {
	AddClass("menu-bar");

	StyleProperties sp;
	sp.flexDirection = FlexDirection::Row;
	sp.alignItems = AlignItems::Center;
	sp.width = StyleLength::Grow();
	MergeStyle(sp);
}

MenuDropdown *MenuBar::AddMenu(std::string title) {
	auto btn = CreateUnique<Button>();
	btn->SetText(title);
	btn->AddClass("menu-bar-item");

	auto dropdown = CreateUnique<MenuDropdown>(this);
	MenuDropdown *dropdownPtr = dropdown.get();

	btn->onClick.Connect([this, dropdownPtr] {
		if (m_OpenDropdown == dropdownPtr) {
			CloseAll();
			return;
		}
		for (auto &entry : m_Entries) {
			if (entry.dropdown == dropdownPtr) {
				Rect rect = entry.button->GetAbsoluteRect();
				OpenDropdown(dropdownPtr, rect.position, rect.size.y);
				return;
			}
		}
	});

	Button *btnPtr = static_cast<Button *>(AddChild(std::move(btn)));
	View *dropdownParent = m_OverlayRoot ? m_OverlayRoot : static_cast<View *>(this);
	dropdownParent->AddChild(std::move(dropdown));

	m_Entries.push_back({ btnPtr, dropdownPtr });
	return dropdownPtr;
}

void MenuBar::SetOverlayRoot(View *root) {
	m_OverlayRoot = root;
}

void MenuBar::CloseAll() {
	if (m_OpenDropdown) {
		m_OpenDropdown->Close();
		m_OpenDropdown = nullptr;
	}
}

void MenuBar::OpenDropdown(MenuDropdown *dropdown, vec2 buttonAbsPos, float buttonHeight) {
	if (m_OpenDropdown && m_OpenDropdown != dropdown) {
		m_OpenDropdown->Close();
	}
	m_OpenDropdown = dropdown;
	dropdown->OpenBelow(buttonAbsPos, buttonHeight);
}

} // namespace Aquila::UI::Core
