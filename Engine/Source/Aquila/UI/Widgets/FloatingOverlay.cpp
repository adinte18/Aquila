#include "Aquila/UI/Widgets/FloatingOverlay.h"
#include "Aquila/UI/Widgets/Button.h"

namespace Aquila::UI::Core {

FloatingOverlay::FloatingOverlay(int16_t backdropZTier) {
	auto bd = CreateUnique<Button>();
	bd->AddClass("floating-backdrop");
	FloatingConfig bfc;
	bfc.attachTo = FloatingAttachTo::Root;
	bfc.elementPoint = FloatingAttachPoint::LeftTop;
	bfc.parentPoint = FloatingAttachPoint::LeftTop;
	bfc.zIndex = backdropZTier;
	bd->SetFloating(bfc);
	bd->SetOnClick([this] {
		if (m_DismissOnClickAway) {
			Close();
		}
	});
	m_Backdrop = static_cast<Button *>(AddChild(std::move(bd)));
}

void FloatingOverlay::Open() {
	if (m_Open) {
		return;
	}
	m_Open = true;
	ApplyDisplayState();
}

void FloatingOverlay::Close() {
	if (!m_Open) {
		return;
	}
	m_Open = false;
	ApplyDisplayState();
}

void FloatingOverlay::Toggle() {
	m_Open ? Close() : Open();
}

void FloatingOverlay::SetDismissOnClickAway(bool v) {
	m_DismissOnClickAway = v;
	if (m_Backdrop && m_Open) {
		StyleProperties p;
		p.display = v ? Display::Flex : Display::None;
		m_Backdrop->MergeStyle(p);
	}
}

void FloatingOverlay::ApplyDisplayState() {
	const Display overlayDisplay = m_Open ? Display::Flex : Display::None;
	const Display bdDisplay = (m_Open && m_DismissOnClickAway) ? Display::Flex : Display::None;

	{
		StyleProperties p;
		p.display = overlayDisplay;
		MergeStyle(p);
	}
	if (m_Backdrop) {
		StyleProperties p;
		p.display = bdDisplay;
		m_Backdrop->MergeStyle(p);
	}
}

View *FloatingOverlay::HitTestAbsolute(vec2 canvasPos) {
	if (GetComputedStyle().display == Display::None) {
		return nullptr;
	}

	// Test non-backdrop children first; they have priority over the backdrop.
	for (int i = static_cast<int>(GetChildren().size()) - 1; i >= 0; --i) {
		View *child = GetChildren()[i].get();
		if (child == m_Backdrop) {
			continue;
		}
		if (View *hit = child->HitTestAbsolute(canvasPos)) {
			return hit;
		}
	}

	const Rect r = GetAbsoluteRect();
	if (r.Contains(canvasPos)) {
		return this;
	}

	if (m_Backdrop) {
		return m_Backdrop->HitTestAbsolute(canvasPos);
	}
	return nullptr;
}

} // namespace Aquila::UI::Core
