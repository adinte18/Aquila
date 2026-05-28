#include "Aquila/UI/Widgets/ContextMenu.h"
#include "Aquila/Application/Events/InputEvent.h"
#include "Aquila/Foundation/Macros.h"

namespace Aquila::UI::Core {

ContextMenu::ContextMenu() {
	StyleProperties sp;
	sp.display = Display::None;
	sp.flexDirection = FlexDirection::Column;
	sp.zIndex = 0;
	SetStyle(sp);

	FloatingConfig fc;
	fc.attachTo = FloatingAttachTo::Root;
	fc.elementPoint = FloatingAttachPoint::LeftTop;
	fc.parentPoint = FloatingAttachPoint::LeftTop;
	fc.offset = { 0.f, 0.f };
	fc.zIndex = 5;
	SetFloating(fc);

	// auto bd = CreateUnique<Button>();
	// StyleProperties bp;
	// bp.display = Display::None;
	// bp.width = StyleLength::Pixel(9999.f);
	// bp.height = StyleLength::Pixel(9999.f);
	// bp.backgroundColor = vec4(0.f);
	// bp.zIndex = 5;
	// bp.padding = StyleEdges{};
	// bd->SetStyle(bp);

	// FloatingConfig bfc;
	// bfc.attachTo = FloatingAttachTo::Root;
	// bfc.elementPoint = FloatingAttachPoint::LeftTop;
	// bfc.parentPoint = FloatingAttachPoint::LeftTop;
	// bfc.zIndex = 5;
	// bd->SetFloating(bfc);
	// bd->SetOnClick([this] { Close(); });
	// m_Backdrop = static_cast<Button *>(AddChild(std::move(bd)));
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
	m_Open = true;
	ApplyDisplayState();
}

void ContextMenu::Close() {
	if (!m_Open) {
		return;
	}
	m_Open = false;
	ApplyDisplayState();
}

void ContextMenu::ApplyDisplayState() {
	const Display d = m_Open ? Display::Flex : Display::None;
	{
		StyleProperties p;
		p.display = d;
		MergeStyle(p);
	}
	if (m_Backdrop) {
		StyleProperties p;
		p.display = d;
		m_Backdrop->MergeStyle(p);
	}
}

View *ContextMenu::HitTestAbsolute(vec2 canvasPos) {
	if (GetComputedStyle().display == Display::None) {
		return nullptr;
	}

	for (int i = static_cast<int>(GetChildren().size()) - 1; i >= 0; --i) {
		View *child = GetChildren()[i].get();
		if (child == m_Backdrop) {
			continue;
		}
		if (View *hit = child->HitTestAbsolute(canvasPos)) {
			return hit;
		}
	}

	const Rect r = { GetAbsolutePosition(), GetLayoutRect().size };
	if (r.Contains(canvasPos)) {
		return this;
	}

	if (m_Backdrop) {
		return m_Backdrop->HitTestAbsolute(canvasPos);
	}

	return nullptr;
}

} // namespace Aquila::UI::Core
