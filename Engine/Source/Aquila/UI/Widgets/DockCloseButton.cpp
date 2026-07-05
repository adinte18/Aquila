#include "Aquila/UI/Widgets/DockCloseButton.h"
#include "Aquila/UI/Widgets/DockNode.h"

namespace Aquila::UI::Core {

void DockCloseButton::set_close_info(DockNode *node, DockPanel *panel) {
	m_node = node;
	m_panel = panel;
}

void DockCloseButton::on_mouse_release(Platform::MouseButton btn, Vec2 pos) {
	View::on_mouse_release(btn, pos);
	if (btn == Platform::MouseButton::Left && m_is_hovered) {
		DockNode *node = m_node;
		DockPanel *panel = m_panel;
		node->close_panel(panel);
	}
}

} // namespace Aquila::UI::Core
