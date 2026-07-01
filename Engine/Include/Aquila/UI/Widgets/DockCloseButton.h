#pragma once

#include "Aquila/UI/Widgets/Button.h"

namespace Aquila::UI::Core {

class DockNode;
class DockPanel;

class DockCloseButton : public Button {
  public:
	DockCloseButton() = default;

	[[nodiscard]] std::string_view GetTypeName() const override { return "DockCloseButton"; }

	void SetCloseInfo(DockNode *node, DockPanel *panel);

	void OnMouseRelease(Platform::MouseButton btn, vec2 pos) override;

  private:
	DockNode *m_Node = nullptr;
	DockPanel *m_Panel = nullptr;
};

} // namespace Aquila::UI::Core
