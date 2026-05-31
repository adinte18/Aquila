#include "Aquila/UI/Widgets/DockSplitter.h"

namespace Aquila::UI::Core {

DockSplitter::DockSplitter(SplitDirection dir) : m_Dir(dir) {
	StyleProperties sp;
	if (dir == SplitDirection::Horizontal) {
		sp.width = StyleLength::Pixel(5.f);
		sp.height = StyleLength::Percent(100.f);
	} else {
		sp.width = StyleLength::Percent(100.f);
		sp.height = StyleLength::Pixel(5.f);
	}
	SetStyle(sp);
	SetInputLeaf(true);
	AddClass("dock-splitter");
}

void DockSplitter::SetSiblings(View *before, View *after) {
	m_Before = before;
	m_After = after;
}

void DockSplitter::UpdateSiblingRef(View *old, View *newPtr) {
	if (m_Before == old) {
		m_Before = newPtr;
	}
	if (m_After == old) {
		m_After = newPtr;
	}
}

void DockSplitter::OnMousePress(Platform::MouseButton btn, vec2 pos) {
	View::OnMousePress(btn, pos);
	if (btn != Platform::MouseButton::Left || !m_Before || !m_After) {
		return;
	}
	m_DragStartPos = pos;
	m_BeforeGrowStart =
		(m_Dir == SplitDirection::Horizontal) ? m_Before->GetLayoutRect().size.x : m_Before->GetLayoutRect().size.y;
	m_AfterGrowStart =
		(m_Dir == SplitDirection::Horizontal) ? m_After->GetLayoutRect().size.x : m_After->GetLayoutRect().size.y;
}

void DockSplitter::OnMouseRelease(Platform::MouseButton btn, vec2 pos) {
	View::OnMouseRelease(btn, pos);
}

void DockSplitter::OnMouseMove(vec2 pos) {
	if (!m_IsPressed || !m_Before || !m_After || !GetParent()) {
		return;
	}

	const float delta = (m_Dir == SplitDirection::Horizontal) ? pos.x - m_DragStartPos.x : pos.y - m_DragStartPos.y;

	const float parentPx = (m_Dir == SplitDirection::Horizontal) ? GetParent()->GetLayoutRect().size.x
																 : GetParent()->GetLayoutRect().size.y;
	if (parentPx <= 0.f) {
		return;
	}

	// Each side must stay at least kMin pixels wide/tall regardless of layout context.
	constexpr float kMin = 60.f;
	const float minPx = kMin;
	const float maxPx = std::max(minPx, parentPx - 5.f - kMin);

	StyleProperties sp;
	if (m_ResizeBefore) {
		const float newPx = std::clamp(m_BeforeGrowStart + delta, minPx, maxPx);
		const float newPct = newPx / parentPx * 100.f;
		if (m_Dir == SplitDirection::Horizontal) {
			sp.width = StyleLength::Percent(newPct);
		} else {
			sp.height = StyleLength::Percent(newPct);
		}
		m_Before->MergeStyle(sp);
	} else {
		const float newPx = std::clamp(m_AfterGrowStart - delta, minPx, maxPx);
		const float newPct = newPx / parentPx * 100.f;
		if (m_Dir == SplitDirection::Horizontal) {
			sp.width = StyleLength::Percent(newPct);
		} else {
			sp.height = StyleLength::Percent(newPct);
		}
		m_After->MergeStyle(sp);
	}

	GetParent()->InvalidateLayout();
}

} // namespace Aquila::UI::Core
