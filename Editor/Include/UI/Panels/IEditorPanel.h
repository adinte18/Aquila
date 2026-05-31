#pragma once

namespace Aquila::UI::Core {
class DockPanel;
class View;
}

namespace Editor {

class IEditorPanel {
  public:
	virtual ~IEditorPanel() = default;
	virtual void Build(Aquila::UI::Core::DockPanel *panel, Aquila::UI::Core::View *overlayRoot) = 0;
};

} // namespace Editor
