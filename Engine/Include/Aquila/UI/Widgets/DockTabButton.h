#pragma once

#include "Aquila/UI/Widgets/Button.h"
#include "Aquila/UI/Widgets/DockDragContext.h"

namespace Aquila::UI::Core {

class DockTabButton : public Button {
  public:
	DockTabButton() = default;

	[[nodiscard]] std::string_view get_type_name() const override { return "DockTabButton"; }

	void set_drag_info(DockDragContext *ctx, DockPanel *panel, DockNode *node);

	void on_mouse_press(Platform::MouseButton btn, Vec2 pos) override;
	void on_mouse_move(Vec2 pos) override;
	void on_mouse_release(Platform::MouseButton btn, Vec2 pos) override;

  private:
	static constexpr float K_DRAG_THRESHOLD = 6.F;

	DockDragContext *m_drag_ctx = nullptr;
	DockPanel *m_panel = nullptr;
	DockNode *m_node = nullptr;

	Vec2 m_press_pos{};
	bool m_dragging = false;
};

} // namespace Aquila::UI::Core
