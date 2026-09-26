#include "Aquila/UI/Core/DockLayoutBuilder.h"

namespace Aquila::UI::Core::DockLayout {

namespace {

DockNodeDesc split(Uint8 direction, std::initializer_list<Part> parts) {
	DockNodeDesc node;
	node.is_leaf = false;
	node.direction = direction;
	for (const Part &part : parts) {
		DockNodeDesc child = part.second;
		child.fraction = part.first;
		node.children.push_back(std::move(child));
	}
	return node;
}

}

DockNodeDesc tabs(std::initializer_list<std::string> panel_ids, Int32 active) {
	DockNodeDesc node;
	node.is_leaf = true;
	node.active_index = active;
	node.panel_ids.assign(panel_ids.begin(), panel_ids.end());
	return node;
}

DockNodeDesc row(std::initializer_list<Part> parts) {
	return split(0, parts);
}

DockNodeDesc column(std::initializer_list<Part> parts) {
	return split(1, parts);
}

DockLayoutDesc layout(DockNodeDesc root) {
	DockLayoutDesc desc;
	root.fraction = 1.F;
	desc.root = std::move(root);
	return desc;
}

}
