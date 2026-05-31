#pragma once

#include "Aquila/UI/Core/View.h"

namespace Aquila::UI::Core {

class Button;

class FloatingOverlay : public View {
  public:
	void Open();
	void Close();
	void Toggle();
	[[nodiscard]] bool IsOpen() const { return m_Open; }

	void SetDismissOnClickAway(bool v);

	View *HitTestAbsolute(vec2 canvasPos) override;

  protected:
	explicit FloatingOverlay(int16_t backdropZTier = 49);

	void ApplyDisplayState();

	bool m_Open = false;
	bool m_DismissOnClickAway = true;
	Button *m_Backdrop = nullptr;
};

} // namespace Aquila::UI::Core
