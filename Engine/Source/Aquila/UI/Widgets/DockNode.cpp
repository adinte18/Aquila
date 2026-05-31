#include "Aquila/UI/Widgets/DockNode.h"
#include "Aquila/UI/Widgets/DockPanel.h"
#include "Aquila/UI/Widgets/DockSplitter.h"
#include "Aquila/UI/Widgets/DockTabButton.h"

#include "Aquila/UI/Style/StyleTypes.h"
namespace Aquila::UI::Core {

View *DockNode::MakeZoneIndicator(FloatingAttachPoint elemPt, FloatingAttachPoint parentPt, vec2 offset,
								  const char *cls) {
	auto zone = CreateUnique<View>();
	zone->AddClass("dock-zone-indicator");
	zone->AddClass(cls);

	FloatingConfig cfg;
	cfg.attachTo = FloatingAttachTo::Parent;
	cfg.elementPoint = elemPt;
	cfg.parentPoint = parentPt;
	cfg.offset = offset;
	cfg.zIndex = 50;
	zone->SetFloating(cfg);

	StyleProperties sp;
	sp.display = Display::None;
	zone->SetStyle(sp);

	return AddChild(std::move(zone));
}

DockNode::DockNode(DockDragContext *dragCtx) : m_DragCtx(dragCtx) {
	StyleProperties sp;
	sp.flexDirection = FlexDirection::Column;
	sp.flexGrow = 1.f;
	sp.overflow = Overflow::Hidden;
	SetStyle(sp);
	AddClass("dock-node");

	auto tabBar = CreateUnique<View>();
	{
		StyleProperties tb;
		tb.flexDirection = FlexDirection::Row;
		tb.width = StyleLength::Grow();
		tb.height = StyleLength::Pixel(30.f);
		tabBar->SetStyle(tb);
		tabBar->AddClass("dock-tab-bar");
	}
	m_TabBar = AddChild(std::move(tabBar));

	auto panelArea = CreateUnique<View>();
	{
		StyleProperties pa;
		pa.flexDirection = FlexDirection::Column;
		pa.width = StyleLength::Grow();
		pa.height = StyleLength::Grow();
		pa.flexGrow = 1.f;
		panelArea->SetStyle(pa);
		panelArea->AddClass("dock-panel-area");
	}
	m_PanelArea = AddChild(std::move(panelArea));

	using AP = FloatingAttachPoint;
	constexpr float kStep = 52.f;
	m_ZoneCenter = MakeZoneIndicator(AP::Center, AP::Center, { 0.f, 0.f }, "dock-zone-center-ind");
	m_ZoneLeft = MakeZoneIndicator(AP::Center, AP::Center, { -kStep, 0.f }, "dock-zone-left-ind");
	m_ZoneRight = MakeZoneIndicator(AP::Center, AP::Center, { kStep, 0.f }, "dock-zone-right-ind");
	m_ZoneTop = MakeZoneIndicator(AP::Center, AP::Center, { 0.f, -kStep }, "dock-zone-top-ind");
	m_ZoneBottom = MakeZoneIndicator(AP::Center, AP::Center, { 0.f, kStep }, "dock-zone-bottom-ind");
}

std::pair<DockNode *, DockNode *> DockNode::Split(SplitDirection dir, bool anchorFirst) {
	for (View *z : { m_ZoneCenter, m_ZoneLeft, m_ZoneRight, m_ZoneTop, m_ZoneBottom }) {
		if (z) {
			RemoveChild(z);
		}
	}
	m_ZoneCenter = m_ZoneLeft = m_ZoneRight = m_ZoneTop = m_ZoneBottom = nullptr;

	if (m_TabBar) {
		RemoveChild(m_TabBar);
		m_TabBar = nullptr;
	}
	if (m_PanelArea) {
		RemoveChild(m_PanelArea);
		m_PanelArea = nullptr;
	}
	m_IsLeaf = false;

	StyleProperties sp;
	sp.flexDirection = (dir == SplitDirection::Horizontal) ? FlexDirection::Row : FlexDirection::Column;
	MergeStyle(sp);

	const bool isH = (dir == SplitDirection::Horizontal);

	auto first = CreateUnique<DockNode>(m_DragCtx);
	{
		StyleProperties fp;

		if (isH) {
			fp.height = StyleLength::Percent(100.f);
		} else {
			fp.width = StyleLength::Percent(100.f);
		}
		first->MergeStyle(fp);
	}

	auto splitter = CreateUnique<DockSplitter>(dir);
	splitter->SetResizeBefore(anchorFirst);

	auto second = CreateUnique<DockNode>(m_DragCtx);
	{
		StyleProperties sp2;
		sp2.flexGrow = 1.f;
		if (isH) {
			sp2.height = StyleLength::Percent(100.f);
		} else {
			sp2.width = StyleLength::Percent(100.f);
		}
		second->MergeStyle(sp2);
	}

	DockNode *firstRaw = static_cast<DockNode *>(AddChild(std::move(first)));
	DockSplitter *splitRaw = static_cast<DockSplitter *>(AddChild(std::move(splitter)));
	DockNode *secondRaw = static_cast<DockNode *>(AddChild(std::move(second)));

	splitRaw->SetSiblings(firstRaw, secondRaw);

	return { firstRaw, secondRaw };
}

DockNode *DockNode::AppendLeaf(SplitDirection dir) {
	DockNode *prevLast = nullptr;
	for (auto &child : GetChildren()) {
		if (auto *dn = dynamic_cast<DockNode *>(child.get())) {
			prevLast = dn;
		}
	}
	if (!prevLast) {
		return nullptr;
	}

	const bool isH = (dir == SplitDirection::Horizontal);

	auto splitter = CreateUnique<DockSplitter>(dir);
	splitter->SetResizeBefore(false);

	auto leaf = CreateUnique<DockNode>(m_DragCtx);
	{
		StyleProperties lp;
		lp.flexGrow = 0.f;
		if (isH) {
			lp.height = StyleLength::Percent(100.f);
		} else {
			lp.width = StyleLength::Percent(100.f);
		}
		leaf->MergeStyle(lp);
	}

	DockSplitter *splitRaw = static_cast<DockSplitter *>(AddChild(std::move(splitter)));
	DockNode *leafRaw = static_cast<DockNode *>(AddChild(std::move(leaf)));

	splitRaw->SetSiblings(prevLast, leafRaw);

	return leafRaw;
}

DockPanel *DockNode::AddPanel(std::string title) {
	auto btn = CreateUnique<DockTabButton>();
	btn->SetText(title);
	btn->AddClass("dock-tab-btn");
	DockTabButton *btnRaw = static_cast<DockTabButton *>(m_TabBar->AddChild(std::move(btn)));

	auto panel = CreateUnique<DockPanel>(title);
	DockPanel *panelRaw = static_cast<DockPanel *>(m_PanelArea->AddChild(std::move(panel)));

	btnRaw->SetDragInfo(m_DragCtx, panelRaw, this);
	btnRaw->SetOnClick([this, panelRaw] { SetActivePanelByPtr(panelRaw); });

	m_Tabs.push_back({ btnRaw, panelRaw, title });

	if (m_ActivePanel < 0) {
		SetActivePanel(0);
	}

	return panelRaw;
}

void DockNode::SetActivePanel(int index) {
	if (index < 0 || index >= static_cast<int>(m_Tabs.size())) {
		return;
	}
	m_ActivePanel = index;
	ApplyActivePanel();
}

void DockNode::SetActivePanelByPtr(DockPanel *panel) {
	for (int i = 0; i < static_cast<int>(m_Tabs.size()); ++i) {
		if (m_Tabs[i].panel == panel) {
			SetActivePanel(i);
			return;
		}
	}
}

void DockNode::ApplyActivePanel() {
	for (int i = 0; i < static_cast<int>(m_Tabs.size()); ++i) {
		const bool active = (i == m_ActivePanel);
		StyleProperties sp;
		sp.display = active ? Display::Flex : Display::None;
		m_Tabs[i].panel->MergeStyle(sp);
		if (active) {
			m_Tabs[i].button->AddClass("dock-tab-active");
		} else {
			m_Tabs[i].button->RemoveClass("dock-tab-active");
		}
	}
}

Unique<View> DockNode::DetachPanel(DockPanel *panel) {
	auto it = std::find_if(m_Tabs.begin(), m_Tabs.end(), [panel](const Tab &t) { return t.panel == panel; });
	if (it == m_Tabs.end()) {
		return nullptr;
	}

	m_TabBar->RemoveChild(it->button);
	auto owned = m_PanelArea->DetachChild(panel);
	m_Tabs.erase(it);

	if (m_Tabs.empty()) {
		m_ActivePanel = -1;
	} else {
		m_ActivePanel = std::clamp(m_ActivePanel, 0, static_cast<int>(m_Tabs.size()) - 1);
		ApplyActivePanel();
	}

	return owned;
}

void DockNode::AcceptPanel(Unique<View> panelView, std::string title, DropZone zone) {
	if (zone == DropZone::None) {
		return;
	}

	if (zone == DropZone::Center) {
		DockPanel *panelRaw = static_cast<DockPanel *>(m_PanelArea->AddChild(std::move(panelView)));

		auto btn = CreateUnique<DockTabButton>();
		btn->SetText(title);
		btn->AddClass("dock-tab-btn");
		DockTabButton *btnRaw = static_cast<DockTabButton *>(m_TabBar->AddChild(std::move(btn)));

		btnRaw->SetDragInfo(m_DragCtx, panelRaw, this);
		btnRaw->SetOnClick([this, panelRaw] { SetActivePanelByPtr(panelRaw); });

		m_Tabs.push_back({ btnRaw, panelRaw, std::move(title) });
		SetActivePanel(static_cast<int>(m_Tabs.size()) - 1);
		return;
	}

	struct Saved {
		Unique<View> view;
		std::string title;
	};
	std::vector<Saved> existing;
	existing.reserve(m_Tabs.size());

	for (auto &tab : m_Tabs) {
		m_TabBar->RemoveChild(tab.button);
		existing.push_back({ m_PanelArea->DetachChild(tab.panel), tab.title });
	}
	m_Tabs.clear();
	m_ActivePanel = -1;

	const SplitDirection splitDir =
		(zone == DropZone::Left || zone == DropZone::Right) ? SplitDirection::Horizontal : SplitDirection::Vertical;
	const bool newFirst = (zone == DropZone::Left || zone == DropZone::Top);

	auto [firstNode, secondNode] = Split(splitDir);

	DockNode *newNode = newFirst ? firstNode : secondNode;
	DockNode *keepNode = newFirst ? secondNode : firstNode;

	newNode->AcceptPanel(std::move(panelView), std::move(title), DropZone::Center);
	for (auto &saved : existing) {
		keepNode->AcceptPanel(std::move(saved.view), std::move(saved.title), DropZone::Center);
	}
}

DockNode *DockNode::HitTestNode(vec2 absPos) {
	if (m_IsLeaf) {
		return GetAbsoluteRect().Contains(absPos) ? this : nullptr;
	}
	for (auto &child : GetChildren()) {
		if (auto *dn = dynamic_cast<DockNode *>(child.get())) {
			if (auto *hit = dn->HitTestNode(absPos)) {
				return hit;
			}
		}
	}
	return nullptr;
}

void DockNode::ShowDropZones(bool show) {
	if (!m_IsLeaf) {
		return;
	}
	StyleProperties sp;
	sp.display = show ? Display::Flex : Display::None;
	for (View *z : { m_ZoneCenter, m_ZoneLeft, m_ZoneRight, m_ZoneTop, m_ZoneBottom }) {
		if (z) {
			z->MergeStyle(sp);
		}
	}
	if (show) {
		AddClass("dock-drop-target");
	} else {
		RemoveClass("dock-drop-target");
	}
}

void DockNode::HighlightDropZone(DropZone zone) {
	for (View *z : { m_ZoneCenter, m_ZoneLeft, m_ZoneRight, m_ZoneTop, m_ZoneBottom }) {
		if (z) {
			z->RemoveClass("dock-zone-indicator-active");
		}
	}
	View *active = nullptr;
	switch (zone) {
	case DropZone::Center:
		active = m_ZoneCenter;
		break;
	case DropZone::Left:
		active = m_ZoneLeft;
		break;
	case DropZone::Right:
		active = m_ZoneRight;
		break;
	case DropZone::Top:
		active = m_ZoneTop;
		break;
	case DropZone::Bottom:
		active = m_ZoneBottom;
		break;
	default:
		break;
	}
	if (active) {
		active->AddClass("dock-zone-indicator-active");
	}
}

DropZone DockNode::HitTestDropZone(vec2 absPos) const {
	// Precise: explicit indicator boxes take priority.
	if (m_ZoneCenter && m_ZoneCenter->GetAbsoluteRect().Contains(absPos)) {
		return DropZone::Center;
	}
	if (m_ZoneLeft && m_ZoneLeft->GetAbsoluteRect().Contains(absPos)) {
		return DropZone::Left;
	}
	if (m_ZoneRight && m_ZoneRight->GetAbsoluteRect().Contains(absPos)) {
		return DropZone::Right;
	}
	if (m_ZoneTop && m_ZoneTop->GetAbsoluteRect().Contains(absPos)) {
		return DropZone::Top;
	}
	if (m_ZoneBottom && m_ZoneBottom->GetAbsoluteRect().Contains(absPos)) {
		return DropZone::Bottom;
	}

	// Fallback: position within the node rect — no need to hit a tiny indicator.
	const Rect r = GetAbsoluteRect();
	if (!r.Contains(absPos)) {
		return DropZone::None;
	}

	const float relX = (absPos.x - r.position.x) / r.size.x;
	const float relY = (absPos.y - r.position.y) / r.size.y;
	constexpr float kEdge = 0.25f;

	if (relX < kEdge) {
		return DropZone::Left;
	}
	if (relX > 1.f - kEdge) {
		return DropZone::Right;
	}
	if (relY < kEdge) {
		return DropZone::Top;
	}
	if (relY > 1.f - kEdge) {
		return DropZone::Bottom;
	}
	return DropZone::Center;
}

} // namespace Aquila::UI::Core
