#pragma once

#include "Aquila/UI/Widgets/Button.h"
#include "Aquila/UI/Widgets/DockDragContext.h"

namespace Aquila::UI::Core {

class DockTabButton : public Button {
  public:
	DockTabButton() = default;

	[[nodiscard]] std::string_view GetTypeName() const override { return "DockTabButton"; }

	void SetDragInfo(DockDragContext *ctx, DockPanel *panel, DockNode *node);

	void OnMousePress(Platform::MouseButton btn, vec2 pos) override;
	void OnMouseMove(vec2 pos) override;
	void OnMouseRelease(Platform::MouseButton btn, vec2 pos) override;

  private:
	static constexpr float kDragThreshold = 6.f;

	DockDragContext *m_DragCtx = nullptr;
	DockPanel *m_Panel = nullptr;
	DockNode *m_Node = nullptr;

	vec2 m_PressPos{};
	bool m_Dragging = false;
};

} // namespace Aquila::UI::Core
