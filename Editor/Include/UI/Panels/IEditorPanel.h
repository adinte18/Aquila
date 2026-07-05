#pragma once

#include "Aquila/Foundation/Signal.h"

namespace Aquila::UI::Core {
class DockPanel;
class View;
} // namespace Aquila::UI::Core

namespace Editor {

class IEditorPanel {
  public:
	virtual ~IEditorPanel() = default;
	virtual void build(Aquila::UI::Core::DockPanel *panel, Aquila::UI::Core::View *overlay_root) = 0;
};

} // namespace Editor
