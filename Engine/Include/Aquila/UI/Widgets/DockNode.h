#pragma once

#include "Aquila/UI/Core/View.h"
#include "Aquila/UI/Widgets/DockDragContext.h"
#include "Aquila/UI/Widgets/DockTypes.h"
#include <string>
#include <utility>
#include <vector>

namespace Aquila::UI::Core {

class DockPanel;
class DockTabButton;

class DockNode : public View {
  public:
	explicit DockNode(DockDragContext *dragCtx = nullptr);

	[[nodiscard]] std::string_view GetTypeName() const override { return "DockNode"; }

	std::pair<DockNode *, DockNode *> Split(SplitDirection dir, bool anchorFirst = true);
	DockNode *AppendLeaf(SplitDirection dir);

	DockPanel *AddPanel(std::string title);

	void SetActivePanel(int index);
	void SetActivePanelByPtr(DockPanel *panel);
	[[nodiscard]] int GetActivePanel() const { return m_ActivePanel; }
	[[nodiscard]] bool IsEmpty() const { return m_IsLeaf && m_Tabs.empty(); }
	[[nodiscard]] int GetTabCount() const { return static_cast<int>(m_Tabs.size()); }
	[[nodiscard]] DockPanel *GetActivePanelPtr() const;
	[[nodiscard]] View *GetTabBar() const { return m_TabBar; }
	// Drag-drop
	DockNode *HitTestNode(vec2 absPos);
	Unique<View> DetachPanel(DockPanel *panel);
	void AcceptPanel(Unique<View> panelView, std::string title, DropZone zone = DropZone::Center);

	// Close a tab (no destination — panel is destroyed). Triggers CollapseNode if empty.
	void ClosePanel(DockPanel *panel);

	void ReorderPanel(DockPanel *panel, vec2 cursorPos);

	// Drop zone visual feedback
	void ShowDropZones(bool show);
	void HighlightDropZone(DropZone zone);
	[[nodiscard]] DropZone HitTestDropZone(vec2 absPos) const;

  private:
	View *MakeZoneIndicator(FloatingAttachPoint elemPt, FloatingAttachPoint parentPt, vec2 offset, const char *cls);
	void ApplyActivePanel();
	void AppendTab(DockPanel *panel, std::string title);

	DockDragContext *m_DragCtx = nullptr;
	bool m_IsLeaf = true;
	View *m_TabBar = nullptr;
	View *m_PanelArea = nullptr;

	View *m_ZoneCenter = nullptr;
	View *m_ZoneLeft = nullptr;
	View *m_ZoneRight = nullptr;
	View *m_ZoneTop = nullptr;
	View *m_ZoneBottom = nullptr;

	struct Tab {
		View *wrapper = nullptr;
		DockTabButton *button = nullptr;
		DockPanel *panel = nullptr;
		std::string title;
	};
	std::vector<Tab> m_Tabs;
	int m_ActivePanel = -1;
};

} // namespace Aquila::UI::Core
