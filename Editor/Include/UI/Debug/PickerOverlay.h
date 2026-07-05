#pragma once

#include "Aquila/UI/Core/View.h"
#include "Aquila/Foundation/Math/Rect.h"

namespace Aquila::UI::Rendering {
class DrawList;
}

namespace Editor {

// Full-canvas, hit-test-transparent overlay that draws a highlight box around a
// picked view. Driven by the element picker; sits above all editor content.
class PickerOverlay : public Aquila::UI::Core::View {
  public:
	PickerOverlay();

	[[nodiscard]] std::string_view GetTypeName() const override { return "PickerOverlay"; }

	void SetTarget(const Rect &rect);
	void Clear();

	void OnDrawSelf(Aquila::UI::Rendering::DrawList &drawList) override;

  private:
	bool m_Active = false;
	Rect m_Target;
};

} // namespace Editor
