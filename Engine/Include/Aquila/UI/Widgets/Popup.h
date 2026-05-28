#pragma once

#include "Aquila/UI/Core/View.h"
#include "Aquila/UI/Widgets/Button.h"

namespace Aquila::UI::Core {

// Floating overlay container with built-in click-away dismissal.
//
// Default floating: drops below its parent (LeftBottom → LeftTop), zIndex 100.
// Override with SetFloating() after construction.
// SetDismissOnClickAway(false) disables the backdrop entirely.
class Popup : public View {
  public:
	Popup();

	[[nodiscard]] std::string_view GetTypeName() const override { return "Popup"; }

	void Open();
	void Close();
	void Toggle();
	[[nodiscard]] bool IsOpen() const { return m_Open; }

	void SetDismissOnClickAway(bool v);

	View *HitTestAbsolute(vec2 canvasPos) override;

  private:
	void ApplyDisplayState();

	bool m_Open = false;
	bool m_DismissOnClickAway = true;
	Button *m_Backdrop = nullptr;
};

} // namespace Aquila::UI::Core
