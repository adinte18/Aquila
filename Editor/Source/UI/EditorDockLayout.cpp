#include "UI/EditorDockLayout.h"

#include "Aquila/Foundation/Macros.h"
#include "Aquila/UI/Widgets/DockNode.h"
#include "Aquila/UI/Widgets/DockPanel.h"
#include "Aquila/UI/Widgets/DockSpace.h"

namespace Editor {

using namespace Aquila;

DockPanels EditorDockLayout::Build(UI::Core::View *dockRoot) {
	auto dockSpaceUniq = CreateUnique<UI::Core::DockSpace>();
	auto *dockSpace = static_cast<UI::Core::DockSpace *>(dockRoot->AddChild(std::move(dockSpaceUniq)));

	// top work area | bottom console strip
	auto [topArea, bottomNode] = dockSpace->GetRootNode()->Split(UI::Core::SplitDirection::Vertical, false);
	bottomNode->AddClass("dock-side-bottom");

	// left hierarchy | center viewport | right inspector
	auto [leftNode, centerNode] = topArea->Split(UI::Core::SplitDirection::Horizontal);
	auto *rightNode = topArea->AppendLeaf(UI::Core::SplitDirection::Horizontal);

	leftNode->AddClass("dock-side-left");
	centerNode->AddClass("dock-center");
	rightNode->AddClass("dock-side-right");

	DockPanels panels;
	panels.hierarchy = leftNode->AddPanel("Hierarchy");
	panels.viewport = centerNode->AddPanel("Viewport");
	panels.inspector = rightNode->AddPanel("Inspector");
	panels.console = bottomNode->AddPanel("Console");

	panels.hierarchy->SetId("panel-hierarchy");
	panels.viewport->SetId("panel-viewport");
	panels.inspector->SetId("panel-inspector");
	panels.console->SetId("panel-console");

	return panels;
}

} // namespace Editor
