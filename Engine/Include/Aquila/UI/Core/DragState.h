#pragma once

namespace Aquila::UI::Core {
struct DragState {
	std::any payload;
	bool is_dragging;
};
} // namespace Aquila::UI::Core
