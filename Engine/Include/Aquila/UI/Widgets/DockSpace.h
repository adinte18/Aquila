#pragma once

#include "Aquila/UI/Core/View.h"
#include "Aquila/UI/Widgets/DockDragContext.h"
#include "Aquila/UI/Widgets/DockTypes.h"

namespace Aquila::UI::Core {

class DockNode;
class DockSplitter;

class DockSpace : public View {
  public:
	DockSpace();

	[[nodiscard]] std::string_view GetTypeName() const override { return "DockSpace"; }
	[[nodiscard]] DockNode *GetRootNode() const { return m_Root; }

  private:
	void ExecuteDrop(DockNode *target, DropZone zone);
	void CollapseNode(DockNode *node);
	void UpdatePreview(DockNode *target, DropZone zone);

	DockDragContext m_DragCtx;
	DockNode *m_Root = nullptr;
	DockNode *m_DropTarget = nullptr;
	DropZone m_CurrentZone = DropZone::None;
	View *m_DropPreview = nullptr;
};

} // namespace Aquila::UI::Core
