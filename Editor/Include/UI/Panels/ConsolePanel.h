#pragma once

#include "UI/Panels/IEditorPanel.h"

namespace Editor {

class ConsolePanel : public IEditorPanel {
  public:
	ConsolePanel() = default;
	void Build(Aquila::UI::Core::DockPanel *panel, Aquila::UI::Core::View *overlayRoot) override;
};

} // namespace Editor
