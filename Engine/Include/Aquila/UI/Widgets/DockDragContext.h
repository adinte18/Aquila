#pragma once

#include "Aquila/UI/Core/View.h"
#include <functional>

namespace Aquila::UI::Core {

class DockNode;
class DockPanel;

struct DockDragContext {
	bool active = false;
	std::string title = "";
	DockPanel *panel = nullptr;
	DockNode *sourceNode = nullptr;

	Unique<View> externalView;

	std::function<void(vec2)> onMove;
	std::function<void(vec2)> onRelease;
	std::function<void(DockNode *)> onNodeEmptied;
};

} // namespace Aquila::UI::Core
