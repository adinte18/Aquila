#include "Aquila/UI/Widgets/FloatingOverlay.h"
#include "Aquila/UI/Core/Canvas.h"

namespace Aquila::UI::Core {

FloatingOverlay::FloatingOverlay(int16_t /*backdropZTier*/) {
	SetHidden(true);
}

void FloatingOverlay::Open() {
	if (m_Open) {
		return;
	}
	m_Open = true;
	ApplyDisplayState();
	if (m_DismissOnClickAway) {
		if (Canvas *canvas = GetCanvas()) {
			canvas->RegisterPopup(this, [this] { Close(); });
		}
	}
}

void FloatingOverlay::Close() {
	if (!m_Open) {
		return;
	}
	m_Open = false;
	ApplyDisplayState();
	if (Canvas *canvas = GetCanvas()) {
		canvas->UnregisterPopup(this);
	}
}

void FloatingOverlay::Toggle() {
	m_Open ? Close() : Open();
}

void FloatingOverlay::SetDismissOnClickAway(bool v) {
	m_DismissOnClickAway = v;
	if (!m_Open) {
		return;
	}
	if (Canvas *canvas = GetCanvas()) {
		if (v) {
			canvas->RegisterPopup(this, [this] { Close(); });
		} else {
			canvas->UnregisterPopup(this);
		}
	}
}

void FloatingOverlay::ApplyDisplayState() {
	SetHidden(!m_Open);
}

View *FloatingOverlay::HitTestAbsolute(vec2 canvasPos) {
	if (GetComputedStyle().display == Display::None) {
		return nullptr;
	}

	for (int i = static_cast<int>(GetChildren().size()) - 1; i >= 0; --i) {
		if (View *hit = GetChildren()[i]->HitTestAbsolute(canvasPos)) {
			return hit;
		}
	}

	const Rect r = GetAbsoluteRect();
	if (r.Contains(canvasPos)) {
		return this;
	}
	return nullptr;
}

} // namespace Aquila::UI::Core
