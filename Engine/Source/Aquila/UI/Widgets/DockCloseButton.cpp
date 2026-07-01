#include "Aquila/UI/Widgets/DockCloseButton.h"
#include "Aquila/UI/Widgets/DockNode.h"

namespace Aquila::UI::Core {

void DockCloseButton::SetCloseInfo(DockNode *node, DockPanel *panel) {
	m_Node = node;
	m_Panel = panel;
}

void DockCloseButton::OnMouseRelease(Platform::MouseButton btn, vec2 pos) {
	View::OnMouseRelease(btn, pos);
	if (btn == Platform::MouseButton::Left && m_IsHovered) {
		DockNode *node = m_Node;
		DockPanel *panel = m_Panel;
		node->ClosePanel(panel);
	}
}

} // namespace Aquila::UI::Core
