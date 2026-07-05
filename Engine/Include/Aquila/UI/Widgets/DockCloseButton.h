#pragma once

#include "Aquila/UI/Widgets/Button.h"

namespace Aquila::UI::Core {

class DockNode;
class DockPanel;

class DockCloseButton : public Button {
  public:
	DockCloseButton() = default;

	[[nodiscard]] std::string_view get_type_name() const override { return "DockCloseButton"; }

	void set_close_info(DockNode *node, DockPanel *panel);

	void on_mouse_release(Platform::MouseButton btn, Vec2 pos) override;

  private:
	DockNode *m_node = nullptr;
	DockPanel *m_panel = nullptr;
};

} // namespace Aquila::UI::Core
