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
	DockNode *source_node = nullptr;

	Unique<View> external_view;

	std::function<void(Vec2)> on_move;
	std::function<void(Vec2)> on_release;
	std::function<void(DockNode *)> on_node_emptied;
};

} // namespace Aquila::UI::Core
