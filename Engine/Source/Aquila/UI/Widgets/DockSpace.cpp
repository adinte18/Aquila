#include "Aquila/UI/Widgets/DockSpace.h"

#include "Aquila/Foundation/Macros.h"
#include "Aquila/Foundation/Math/Rect.h"
#include "Aquila/UI/Core/Canvas.h"
#include "Aquila/UI/Style/StyleTypes.h"
#include "Aquila/UI/Widgets/DockNode.h"
#include "Aquila/UI/Widgets/DockPanel.h"
#include "Aquila/UI/Widgets/DockSplitter.h"

namespace Aquila::UI::Core {

DockSpace::DockSpace() {
	AddClass("dock-space");

	auto preview = CreateUnique<View>();
	preview->AddClass("dock-drop-preview");
	{
		FloatingConfig cfg;
		cfg.attachTo = FloatingAttachTo::Root;
		cfg.elementPoint = FloatingAttachPoint::LeftTop;
		cfg.parentPoint = FloatingAttachPoint::LeftTop;
		cfg.zIndex = 30;
		preview->SetFloating(cfg);
	}
	preview->SetHidden(true);
	m_DropPreview = AddChild(std::move(preview));

	m_DragCtx.onNodeEmptied = [this](DockNode *node) {
		CollapseNode(node);
		if (!HasAnyPanels() && m_OnEmptied) {
			m_OnEmptied();
		}
	};

	m_DragCtx.onMove = [this](vec2 pos) {
		if (IsOutsideCanvas(pos)) {
			if (m_DropTarget) {
				m_DropTarget->ShowDropZones(false);
				m_DropTarget->HighlightDropZone(DropZone::None);
				m_DropTarget = nullptr;
			}
			UpdatePreview(nullptr, DropZone::None);
			m_CurrentZone = DropZone::None;
			m_DragLeftCanvas = true;
			if (m_OnExternalDragMove) {
				m_OnExternalDragMove(pos);
			}
			return;
		}
		if (m_DragLeftCanvas) {
			m_DragLeftCanvas = false;
			if (m_OnExternalDragClear) {
				m_OnExternalDragClear();
			}
		}

		DockNode *hovered = m_Root->HitTestNode(pos);
		bool isSelfDrag = (hovered != nullptr && hovered == m_DragCtx.sourceNode);

		if (isSelfDrag) {
			View *bar = m_DragCtx.sourceNode ? m_DragCtx.sourceNode->GetTabBar() : nullptr;
			const bool overBar = bar && bar->GetAbsoluteRect().Contains(pos);
			if (!overBar || m_DragCtx.sourceNode->GetTabCount() < 2) {
				hovered = nullptr;
				isSelfDrag = false;
			}
		}

		if (hovered != m_DropTarget) {
			if (m_DropTarget) {
				m_DropTarget->ShowDropZones(false);
				m_DropTarget->HighlightDropZone(DropZone::None);
			}
			UpdatePreview(nullptr, DropZone::None);
			m_DropTarget = hovered;
			if (m_DropTarget && !isSelfDrag) {
				m_DropTarget->ShowDropZones(true);
			}
			m_CurrentZone = DropZone::None;
		}

		if (!m_DropTarget) {
			return;
		}

		if (isSelfDrag) {
			// Reorder mode — no zone indicators, zone locked to Center.
			m_CurrentZone = DropZone::Center;
		} else {
			DropZone zone = m_DropTarget->HitTestDropZone(pos);
			if (zone != m_CurrentZone) {
				m_DropTarget->HighlightDropZone(zone);
				m_CurrentZone = zone;
				UpdatePreview(m_DropTarget, zone);
			}
		}
	};

	m_DragCtx.onRelease = [this](vec2 pos) {
		if (m_DragLeftCanvas) {
			m_DragLeftCanvas = false;
			if (m_OnExternalDragClear) {
				m_OnExternalDragClear();
			}
		}
		const bool hasValidDrop = (m_DropTarget && m_DragCtx.active && m_CurrentZone != DropZone::None);

		if (m_DropTarget) {
			m_DropTarget->ShowDropZones(false);
			m_DropTarget->HighlightDropZone(DropZone::None);
		}
		UpdatePreview(nullptr, DropZone::None);

		if (hasValidDrop) {
			ExecuteDrop(m_DropTarget, m_CurrentZone, pos);
		} else if (m_DragCtx.active && m_DragCtx.panel && m_DragCtx.sourceNode && m_OnTearOff) {
			// No dock zone under the cursor. If the tab was pulled off its bar, tear it off — the
			View *bar = m_DragCtx.sourceNode->GetTabBar();
			const bool leftBar = !bar || !bar->GetAbsoluteRect().Contains(pos);
			if (leftBar) {
				DockNode *source = m_DragCtx.sourceNode;
				DockPanel *panel = m_DragCtx.panel;
				std::string title(panel->GetTitle());
				auto panelView = source->DetachPanel(panel);
				if (panelView) {
					if (source->IsEmpty()) {
						CollapseNode(source);
					}
					m_OnTearOff(std::move(panelView), std::move(title), pos);
				}
			}
		}

		m_DropTarget = nullptr;
		m_CurrentZone = DropZone::None;
		m_DragCtx.active = false;
		m_DragCtx.panel = nullptr;
		m_DragCtx.sourceNode = nullptr;
	};

	auto root = CreateUnique<DockNode>(&m_DragCtx);
	m_Root = static_cast<DockNode *>(AddChild(std::move(root)));
}

namespace {

std::vector<View *> DeclaredDockChildren(View *node) {
	std::vector<View *> out;
	for (auto &child : node->GetChildren()) {
		View *c = child.get();
		if (dynamic_cast<DockNode *>(c) != nullptr || dynamic_cast<DockPanel *>(c) != nullptr) {
			out.push_back(c);
		}
	}
	return out;
}

} // namespace

void DockSpace::OnXmlLoaded() {
	DockNode *declRoot = nullptr;
	for (auto &child : GetChildren()) {
		auto *dn = dynamic_cast<DockNode *>(child.get());
		if (dn != nullptr && dn != m_Root) {
			declRoot = dn;
			break;
		}
	}
	if (declRoot == nullptr) {
		return; // no declarative layout — keep the default empty root
	}

	CompileDeclaration(m_Root, declRoot);
	RemoveChild(declRoot);
}

void DockSpace::CompileDeclaration(DockNode *realNode, DockNode *declNode) {
	const Option<SplitDirection> split = declNode->GetDeclaredSplit();
	const std::vector<View *> slots = DeclaredDockChildren(declNode);

	if (!split.has_value()) {
		for (View *slot : slots) {
			if (auto *panel = dynamic_cast<DockPanel *>(slot)) {
				RealizePanel(realNode, panel);
			}
		}
		return;
	}

	if (slots.empty()) {
		return;
	}
	if (slots.size() == 1) {
		RealizeSlot(realNode, slots[0]);
		return;
	}

	std::vector<DockNode *> leaves;
	auto [first, second] = realNode->Split(*split);
	leaves.push_back(first);
	leaves.push_back(second);
	for (size_t i = 2; i < slots.size(); ++i) {
		DockNode *leaf = realNode->AppendLeaf(*split);
		if (leaf == nullptr) {
			break;
		}
		leaves.push_back(leaf);
	}

	for (size_t i = 0; i < leaves.size(); ++i) {
		RealizeSlot(leaves[i], slots[i]);
	}
}

void DockSpace::RealizeSlot(DockNode *realLeaf, View *slot) {
	if (auto *childNode = dynamic_cast<DockNode *>(slot)) {
		CompileDeclaration(realLeaf, childNode);
	} else if (auto *panel = dynamic_cast<DockPanel *>(slot)) {
		RealizePanel(realLeaf, panel);
	}
}

void DockSpace::RealizePanel(DockNode *realLeaf, DockPanel *declPanel) {
	DockPanel *realPanel = realLeaf->AddPanel(declPanel->GetTitle(), declPanel->GetTabIcon());
	realPanel->SetId(declPanel->GetId());
	for (const auto &cls : declPanel->GetClasses()) {
		realPanel->AddClass(cls);
	}

	std::vector<View *> content;
	for (const auto &child : declPanel->GetChildren()) {
		content.push_back(child.get());
	}
	for (View *child : content) {
		realPanel->AddChild(declPanel->DetachChild(child));
	}
}

static DockNode *FindLeafWithTabs(View *view) {
	if (auto *node = dynamic_cast<DockNode *>(view)) {
		if (node->GetTabCount() > 0) {
			return node;
		}
	}
	for (auto &child : view->GetChildren()) {
		if (DockNode *found = FindLeafWithTabs(child.get())) {
			return found;
		}
	}
	return nullptr;
}

bool DockSpace::HasAnyPanels() const {
	return m_Root && FindLeafWithTabs(m_Root) != nullptr;
}

DockNode *DockSpace::FirstLeafWithTabs() const {
	return m_Root ? FindLeafWithTabs(m_Root) : nullptr;
}

void DockSpace::UpdatePreview(DockNode *target, DropZone zone) {
	if (!m_DropPreview) {
		return;
	}

	if (!target || zone == DropZone::None) {
		m_DropPreview->SetHidden(true);
		return;
	}

	Rect r = target->GetAbsoluteRect();
	Rect preview = r;

	switch (zone) {
	case DropZone::Center:

		break;
	case DropZone::Left:
		preview.size.x *= 0.5f;
		break;
	case DropZone::Right:
		preview.position.x += r.size.x * 0.5f;
		preview.size.x *= 0.5f;
		break;
	case DropZone::Top:
		preview.size.y *= 0.5f;
		break;
	case DropZone::Bottom:
		preview.position.y += r.size.y * 0.5f;
		preview.size.y *= 0.5f;
		break;
	default:
		break;
	}

	FloatingConfig cfg;
	cfg.attachTo = FloatingAttachTo::Root;
	cfg.elementPoint = FloatingAttachPoint::LeftTop;
	cfg.parentPoint = FloatingAttachPoint::LeftTop;
	cfg.offset = preview.position;
	cfg.zIndex = 30;
	m_DropPreview->SetFloating(cfg);
	m_DropPreview->InvalidateLayout();

	StyleProperties sp;
	sp.width = StyleLength::Pixel(preview.size.x);
	sp.height = StyleLength::Pixel(preview.size.y);
	m_DropPreview->MergeStyle(sp);
	m_DropPreview->SetHidden(false);
}

void DockSpace::ExecuteDrop(DockNode *target, DropZone zone, vec2 releasePos) {
	if (!target) {
		return;
	}

	DockNode *source = m_DragCtx.sourceNode;

	// External drag: no source node — the panel subtree travels in the drag context.
	if (!source) {
		if (!m_DragCtx.externalView) {
			return;
		}
		Unique<View> panelView = std::move(m_DragCtx.externalView);
		target->AcceptPanel(std::move(panelView), m_DragCtx.title, zone);
		return;
	}

	DockPanel *panel = m_DragCtx.panel;
	if (!panel) {
		return;
	}

	const bool selfDrop = (target == source);

	if (selfDrop && zone == DropZone::Center) {
		source->ReorderPanel(panel, releasePos);
		return;
	}

	if (selfDrop && source->GetTabCount() < 2) {
		return;
	}

	std::string title = std::string(panel->GetTitle());

	auto panelView = source->DetachPanel(panel);
	if (!panelView) {
		return;
	}

	if (!selfDrop && source->IsEmpty()) {
		CollapseNode(source);
	}

	target->AcceptPanel(std::move(panelView), std::move(title), zone);
}

void DockSpace::CollapseNode(DockNode *node) {
	if (node == m_Root) {
		return;
	}

	auto *parent = dynamic_cast<DockNode *>(node->GetParent());
	if (!parent) {
		return;
	}

	DockSplitter *leftSplit = nullptr;
	DockSplitter *rightSplit = nullptr;
	for (auto &child : parent->GetChildren()) {
		auto *ds = dynamic_cast<DockSplitter *>(child.get());
		if (!ds) {
			continue;
		}
		if (ds->GetAfter() == node) {
			leftSplit = ds;
		}
		if (ds->GetBefore() == node) {
			rightSplit = ds;
		}
	}

	DockNode *survivor = nullptr;
	if (leftSplit && rightSplit) {
		leftSplit->SetSiblings(leftSplit->GetBefore(), rightSplit->GetAfter());
		parent->RemoveChild(rightSplit);
	} else if (leftSplit) {
		survivor = dynamic_cast<DockNode *>(leftSplit->GetBefore());
		parent->RemoveChild(leftSplit);
	} else if (rightSplit) {
		survivor = dynamic_cast<DockNode *>(rightSplit->GetAfter());
		parent->RemoveChild(rightSplit);
	}

	parent->RemoveChild(node);

	if (survivor) {
		StyleProperties sp;
		sp.width = StyleLength::Grow();
		sp.height = StyleLength::Grow();
		sp.flexGrow = 1.f;
		survivor->MergeStyle(sp);
	}

	// A container reduced to a single node is redundant — hoist that node into the container's slot
	DockNode *onlyChild = nullptr;
	int nodeCount = 0;
	for (auto &child : parent->GetChildren()) {
		if (auto *dn = dynamic_cast<DockNode *>(child.get())) {
			++nodeCount;
			onlyChild = dn;
		}
	}
	if (nodeCount == 1 && onlyChild) {
		HoistSingleChild(parent, onlyChild);
	} else {
		parent->InvalidateLayout();
	}
}

void DockSpace::HoistSingleChild(DockNode *container, DockNode *only) {
	View *grandparent = container->GetParent();
	Unique<View> owned = container->DetachChild(only);
	if (!owned) {
		return;
	}

	StyleProperties sp;
	sp.width = StyleLength::Grow();
	sp.height = StyleLength::Grow();
	sp.flexGrow = 1.f;
	only->MergeStyle(sp);

	if (container == m_Root) {
		ReplaceChild(container, std::move(owned));
		m_Root = only;
	} else if (auto *gpNode = dynamic_cast<DockNode *>(grandparent)) {
		for (auto &child : gpNode->GetChildren()) {
			if (auto *ds = dynamic_cast<DockSplitter *>(child.get())) {
				ds->UpdateSiblingRef(container, only);
			}
		}
		gpNode->ReplaceChild(container, std::move(owned));
	} else if (grandparent) {
		grandparent->ReplaceChild(container, std::move(owned));
	}

	if (grandparent) {
		grandparent->InvalidateLayout();
	}
}

bool DockSpace::IsOutsideCanvas(vec2 pos) const {
	const Canvas *canvas = GetCanvas();
	if (!canvas) {
		return false;
	}
	const auto w = static_cast<float>(canvas->GetWidth());
	const auto h = static_cast<float>(canvas->GetHeight());
	return pos.x < 0.f || pos.y < 0.f || pos.x >= w || pos.y >= h;
}

void DockSpace::PreviewExternalDrag(vec2 localPos) {
	DockNode *hovered = m_Root->HitTestNode(localPos);
	if (hovered != m_DropTarget) {
		if (m_DropTarget) {
			m_DropTarget->ShowDropZones(false);
			m_DropTarget->HighlightDropZone(DropZone::None);
		}
		UpdatePreview(nullptr, DropZone::None);
		m_DropTarget = hovered;
		if (m_DropTarget) {
			m_DropTarget->ShowDropZones(true);
		}
		m_CurrentZone = DropZone::None;
	}

	if (!m_DropTarget) {
		return;
	}

	DropZone zone = m_DropTarget->HitTestDropZone(localPos);
	if (zone != m_CurrentZone) {
		m_DropTarget->HighlightDropZone(zone);
		m_CurrentZone = zone;
		UpdatePreview(m_DropTarget, zone);
	}
}

void DockSpace::ClearExternalDrag() {
	if (m_DropTarget) {
		m_DropTarget->ShowDropZones(false);
		m_DropTarget->HighlightDropZone(DropZone::None);
	}
	UpdatePreview(nullptr, DropZone::None);
	m_DropTarget = nullptr;
	m_CurrentZone = DropZone::None;
}

void DockSpace::BeginExternalDrag(DockPanel *panel, const std::string &title) {
	m_DragCtx.active = true;
	m_DragCtx.sourceNode = nullptr;
	m_DragCtx.panel = panel;
	m_DragCtx.title = title;
	m_DragCtx.externalView.reset();
	m_DropTarget = nullptr;
	m_CurrentZone = DropZone::None;
}

bool DockSpace::TryDockExternal(Unique<View> &panelView, const std::string &title, vec2 localPos) {
	PreviewExternalDrag(localPos);

	DockNode *target = m_DropTarget;
	DropZone zone = m_CurrentZone;

	const bool canDock = (target != nullptr && zone != DropZone::None);
	if (canDock) {
		// resolves it — no direct AcceptPanel from the external path.
		if (!title.empty()) {
			m_DragCtx.title = title;
		}
		m_DragCtx.externalView = std::move(panelView);
		ExecuteDrop(target, zone, localPos);
	}

	ClearExternalDrag();
	m_DragCtx.active = false;
	m_DragCtx.sourceNode = nullptr;
	m_DragCtx.panel = nullptr;
	m_DragCtx.title.clear();
	m_DragCtx.externalView.reset();
	return canDock;
}

} // namespace Aquila::UI::Core
