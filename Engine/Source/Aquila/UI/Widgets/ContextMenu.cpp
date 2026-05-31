#include "Aquila/UI/Widgets/ContextMenu.h"

namespace Aquila::UI::Core {

ContextMenu::ContextMenu() : FloatingOverlay(4) {
	StyleProperties sp;
	sp.display = Display::None;
	sp.flexDirection = FlexDirection::Column;
	SetStyle(sp);

	FloatingConfig fc;
	fc.attachTo = FloatingAttachTo::Root;
	fc.elementPoint = FloatingAttachPoint::LeftTop;
	fc.parentPoint = FloatingAttachPoint::LeftTop;
	fc.offset = { 0.f, 0.f };
	fc.zIndex = 5;
	SetFloating(fc);
}

void ContextMenu::AddItem(std::string text, Delegate<void()> callback) {
	m_Items.push_back({ std::move(text), std::move(callback) });
}

void ContextMenu::ClearItems() {
	for (View *v : m_ItemViews) {
		RemoveChild(v);
	}
	m_ItemViews.clear();
	m_Items.clear();
	InvalidateLayout();
}

void ContextMenu::Rebuild() {
	for (View *v : m_ItemViews) {
		RemoveChild(v);
	}
	m_ItemViews.clear();

	for (auto &item : m_Items) {
		auto btn = CreateUnique<Button>();
		btn->SetText(item.text);
		btn->AddClass("context-item");
		btn->SetOnClick([this, cb = item.callback] {
			cb();
			Close();
		});
		m_ItemViews.push_back(AddChild(std::move(btn)));
	}

	InvalidateLayout();
}

void ContextMenu::OpenAt(vec2 pos) {
	FloatingConfig fc = GetFloating();
	fc.offset = pos;
	SetFloating(fc);

	Rebuild();
	Open();
}

} // namespace Aquila::UI::Core
