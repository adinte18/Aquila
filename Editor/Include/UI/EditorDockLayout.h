#pragma once

namespace Aquila::UI::Core {
class DockPanel;
class View;
}

namespace Editor {

struct DockPanels {
	Aquila::UI::Core::DockPanel *hierarchy = nullptr;
	Aquila::UI::Core::DockPanel *viewport = nullptr;
	Aquila::UI::Core::DockPanel *inspector = nullptr;
	Aquila::UI::Core::DockPanel *console = nullptr;
};

class EditorDockLayout {
  public:
	static DockPanels Build(Aquila::UI::Core::View *dockRoot);
};

} // namespace Editor
