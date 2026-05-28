#include "Aquila/UI/Widgets/Popup.h"

namespace Aquila::UI::Core {

Popup::Popup() {
	StyleProperties sp;
	sp.display = Display::None;
	sp.zIndex = 100;
	SetStyle(sp);

	FloatingConfig fc;
	fc.attachTo = FloatingAttachTo::Parent;
	fc.elementPoint = FloatingAttachPoint::LeftTop;
	fc.parentPoint = FloatingAttachPoint::LeftBottom;
	fc.offset = { 0.f, 4.f };
	fc.zIndex = 100;
	SetFloating(fc);

	auto bd = CreateUnique<Button>();
	StyleProperties bp;
	bp.display = Display::None;
	bp.width = StyleLength::Pixel(9999.f);
	bp.height = StyleLength::Pixel(9999.f);
	bp.backgroundColor = vec4(0.f);
	bp.zIndex = 49;
	bp.padding = StyleEdges{};
	bd->SetStyle(bp);
	FloatingConfig bfc;
	bfc.attachTo = FloatingAttachTo::Root;
	bfc.elementPoint = FloatingAttachPoint::LeftTop;
	bfc.parentPoint = FloatingAttachPoint::LeftTop;
	bfc.zIndex = 49;
	bd->SetFloating(bfc);
	bd->SetOnClick([this] {
		if (m_DismissOnClickAway) {
			Close();
		}
	});
	m_Backdrop = static_cast<Button *>(AddChild(std::move(bd)));
}

void Popup::Open() {
	if (m_Open) {
		return;
	}
	m_Open = true;
	ApplyDisplayState();
}

void Popup::Close() {
	if (!m_Open) {
		return;
	}
	m_Open = false;
	ApplyDisplayState();
}

void Popup::Toggle() {
	m_Open ? Close() : Open();
}

void Popup::SetDismissOnClickAway(bool v) {
	m_DismissOnClickAway = v;
	if (m_Backdrop) {
		StyleProperties p;
		p.display = (m_Open && v) ? Display::Flex : Display::None;
		m_Backdrop->MergeStyle(p);
	}
}

void Popup::ApplyDisplayState() {
	const Display popupDisplay = m_Open ? Display::Flex : Display::None;
	const Display bdDisplay = (m_Open && m_DismissOnClickAway) ? Display::Flex : Display::None;
	{
		StyleProperties p;
		p.display = popupDisplay;
		MergeStyle(p);
	}
	if (m_Backdrop) {
		StyleProperties p;
		p.display = bdDisplay;
		m_Backdrop->MergeStyle(p);
	}
}

View *Popup::HitTestAbsolute(vec2 canvasPos) {
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
