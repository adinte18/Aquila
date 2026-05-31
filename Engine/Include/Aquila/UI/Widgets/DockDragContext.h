#pragma once

#include "Aquila/UI/Core/View.h"
#include <functional>

namespace Aquila::UI::Core {

class DockNode;
class DockPanel;

struct DockDragContext {
	bool active = false;
	DockPanel *panel = nullptr;
	DockNode *sourceNode = nullptr;

	std::function<void(vec2)> onMove;
	std::function<void(vec2)> onRelease;
};

} // namespace Aquila::UI::Core
