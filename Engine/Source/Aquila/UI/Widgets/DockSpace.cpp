#include "Aquila/UI/Widgets/DockSpace.h"

#include "Aquila/Foundation/Macros.h"
#include "Aquila/Foundation/Math/Rect.h"
#include "Aquila/UI/Style/StyleTypes.h"
#include "Aquila/UI/Widgets/DockNode.h"
#include "Aquila/UI/Widgets/DockPanel.h"
#include "Aquila/UI/Widgets/DockSplitter.h"

namespace Aquila::UI::Core {

DockSpace::DockSpace() {
	StyleProperties sp;
	sp.flexDirection = FlexDirection::Column;
	sp.width = StyleLength::Grow();
	sp.height = StyleLength::Grow();
	sp.flexGrow = 1.f;
	SetStyle(sp);
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
	{
		StyleProperties psp;
		psp.display = Display::None;
		preview->SetStyle(psp);
	}
	m_DropPreview = AddChild(std::move(preview));

	m_DragCtx.onMove = [this](vec2 pos) {
		DockNode *hovered = m_Root->HitTestNode(pos);
		if (hovered == m_DragCtx.sourceNode) {
			if (!m_DragCtx.sourceNode || m_DragCtx.sourceNode->GetTabCount() < 2) {
				hovered = nullptr;
			}
		}

		if (hovered != m_DropTarget) {
			if (m_DropTarget) {
				m_DropTarget->ShowDropZones(false);
				m_DropTarget->HighlightDropZone(DropZone::None);
			}
			m_DropTarget = hovered;
			if (m_DropTarget) {
				m_DropTarget->ShowDropZones(true);
			}
			m_CurrentZone = DropZone::None;
		}

		if (m_DropTarget) {
			DropZone zone = m_DropTarget->HitTestDropZone(pos);
			if (zone != m_CurrentZone) {
				m_DropTarget->HighlightDropZone(zone);
				m_CurrentZone = zone;
				UpdatePreview(m_DropTarget, zone);
			}
		} else {
			UpdatePreview(nullptr, DropZone::None);
		}
	};

	m_DragCtx.onRelease = [this](vec2 /*pos*/) {
		if (m_DropTarget) {
			m_DropTarget->ShowDropZones(false);
			m_DropTarget->HighlightDropZone(DropZone::None);
		}
		UpdatePreview(nullptr, DropZone::None);

		if (m_DropTarget && m_DragCtx.active && m_CurrentZone != DropZone::None) {
			ExecuteDrop(m_DropTarget, m_CurrentZone);
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

void DockSpace::UpdatePreview(DockNode *target, DropZone zone) {
	if (!m_DropPreview) {
		return;
	}

	if (!target || zone == DropZone::None) {
		StyleProperties sp;
		sp.display = Display::None;
		m_DropPreview->MergeStyle(sp);
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
	sp.display = Display::Flex;
	sp.width = StyleLength::Pixel(preview.size.x);
	sp.height = StyleLength::Pixel(preview.size.y);
	m_DropPreview->MergeStyle(sp);
}

void DockSpace::ExecuteDrop(DockNode *target, DropZone zone) {
	DockPanel *panel = m_DragCtx.panel;
	DockNode *source = m_DragCtx.sourceNode;

	if (!panel || !source || !target) {
		return;
	}

	const bool selfDrop = (target == source);

	if (selfDrop && (zone == DropZone::Center || source->GetTabCount() < 2)) {
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
	View *parent = node->GetParent();
	if (!parent) {
		return;
	}

	std::vector<DockSplitter *> adjacentSplitters;
	DockNode *sibling = nullptr;

	for (auto &child : parent->GetChildren()) {
		auto *ds = dynamic_cast<DockSplitter *>(child.get());
		if (!ds) {
			continue;
		}
		if (ds->GetBefore() != node && ds->GetAfter() != node) {
			continue;
		}

		adjacentSplitters.push_back(ds);

		if (!sibling) {
			View *other = (ds->GetBefore() == node) ? ds->GetAfter() : ds->GetBefore();
			sibling = dynamic_cast<DockNode *>(other);
		}
	}

	if (!sibling) {
		return;
	}

	StyleProperties sp;
	sp.width = StyleLength::Grow();
	sp.height = StyleLength::Grow();
	sp.flexGrow = 1.f;
	sibling->MergeStyle(sp);

	if (adjacentSplitters.size() <= 1) {
		if (!adjacentSplitters.empty()) {
			parent->RemoveChild(adjacentSplitters[0]);
		}
	} else {
		DockNode *other = nullptr;
		for (auto &child : parent->GetChildren()) {
			auto *dn = dynamic_cast<DockNode *>(child.get());
			if (dn && dn != node && dn != sibling) {
				other = dn;
				break;
			}
		}
		parent->RemoveChild(adjacentSplitters[0]);
		if (other) {
			adjacentSplitters[1]->SetSiblings(sibling, other);
		} else {
			parent->RemoveChild(adjacentSplitters[1]);
		}
	}

	parent->RemoveChild(node);
	parent->InvalidateLayout();
}

} // namespace Aquila::UI::Core
