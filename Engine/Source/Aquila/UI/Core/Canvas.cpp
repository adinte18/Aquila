#include "Aquila/UI/Core/Canvas.h"
#include "Aquila/Rendering/FrameScheduler.h"

namespace Aquila::UI::Core {

using namespace Aquila::UI::Rendering;

Canvas::Canvas(uint32 width, uint32 height)
	: m_Width(width), m_Height(height), m_LayoutEngine(width, height), m_InputRouter(*this, m_DrawCompositor) {
	m_Root = CreateUnique<View>();
	m_Root->SetCanvas(this);

	m_DrawCompositor.SetCanvasSize(width, height);

	StyleProperties rootStyle;
	rootStyle.width = StyleLength::Grow();
	rootStyle.height = StyleLength::Grow();
	m_Root->SetStyle(rootStyle);
	NotifyStyleDirty(m_Root.get());
	StylePass();
}

void Canvas::MarkDirty() {
	m_Dirty = true;
	Aquila::Rendering::FrameScheduler::Get()->RequestFrame();
}

void Canvas::RequestLayout() {
	m_LayoutDirty = true;
	MarkDirty();
}

void Canvas::NotifyStyleDirty(View *view) {
	m_StyleEngine.Invalidate(view);
	MarkDirty();
}

void Canvas::NotifyAnimationStarted(View *view) {
	for (const View *v : m_ActiveAnims) {
		if (v == view) {
			return;
		}
	}
	m_ActiveAnims.push_back(view);
	MarkDirty();
}

void Canvas::NotifyDrawDirty(View *view) {
	MarkNodeDrawDirty(view);
}

void Canvas::NotifyLayoutDirty(View *view) {
	m_LayoutDirty = true;
	MarkNodeDrawDirty(view);
}

void Canvas::NotifyFocusRequest(View *view) {
	m_InputRouter.SetFocus(view);
}

void Canvas::NotifyViewRemoved(View *view) {
	m_StyleEngine.Remove(view);
	m_InputRouter.OnViewRemoved(view);
	UnregisterPopup(view);
	UnregisterTick(view);
	if (m_ScrollTarget == view) {
		m_ScrollTarget = nullptr;
	}
	if (auto it = std::ranges::find(m_ActiveAnims, view); it != m_ActiveAnims.end()) {
		m_ActiveAnims.erase(it);
	}
}

void Canvas::RegisterPopup(View *popup, Delegate<void()> onDismiss) {
	UnregisterPopup(popup);
	m_OpenPopups.push_back({ popup, std::move(onDismiss) });
}

void Canvas::UnregisterPopup(View *popup) {
	std::erase_if(m_OpenPopups, [popup](const OpenPopup &p) { return p.root == popup; });
}

void Canvas::RegisterTick(View *view) {
	if (std::ranges::find(m_Ticking, view) == m_Ticking.end()) {
		m_Ticking.push_back(view);
	}
}

void Canvas::UnregisterTick(View *view) {
	std::erase(m_Ticking, view);
}

static bool IsWithin(View *node, View *root) {
	for (View *v = node; v; v = v->GetParent()) {
		if (v == root) {
			return true;
		}
	}
	return false;
}

void Canvas::DismissPopupsOutside(View *hit) {
	if (m_OpenPopups.empty()) {
		return;
	}
	std::vector<Delegate<void()>> toDismiss;
	for (const auto &popup : m_OpenPopups) {
		if (!IsWithin(hit, popup.root)) {
			toDismiss.push_back(popup.onDismiss);
		}
	}
	for (auto &dismiss : toDismiss) {
		if (dismiss) {
			dismiss();
		}
	}
}

void Canvas::ReloadStyles() {
	MarkSubtreeDirty(m_Root.get());
}

void Canvas::MarkSubtreeDirty(View *node) {
	NotifyStyleDirty(node);
	for (const auto &child : node->GetChildren()) {
		MarkSubtreeDirty(child.get());
	}
}

void Canvas::StylePass() {
	const StyleEngine::ResolveResult result = m_StyleEngine.Resolve(m_Width, m_Height, m_LayoutDirty);
	m_LayoutDirty = result.layoutAffected;
	if (result.changed) {
		MarkDirty();
	}
}

void Canvas::AnimationPass(f32 dt) {
	auto it = m_ActiveAnims.begin();
	while (it != m_ActiveAnims.end()) {
		View *v = *it;
		v->UpdateAnimation(dt);
		MarkNodeDrawDirty(v);
		if (v->IsAnimationFinished()) {
			it = m_ActiveAnims.erase(it);
		} else {
			++it;
		}
	}
}

void Canvas::Compute() {
	if (!m_Root || !m_Dirty) {
		return;
	}

	if (m_LayoutDirty) {
		m_LayoutEngine.RunLayout(m_Root.get(), m_InputRouter.MousePos(), m_InputRouter.MouseDown(),
								 m_InputRouter.TakeScrollDelta(), m_DeltaTime);
		m_LayoutDirty = false;

		// @container rules depend on element sizes — re-resolve immediately after
		// layout so rules see the current frame's container sizes.
		if (m_StyleEngine.GetStyleSheet().HasContainerBlocks()) {
			MarkSubtreeDirty(m_Root.get());
			StylePass();
		}

		m_DrawCompositor.InvalidateAll(m_Root.get());
	}

	if (m_ScrollTarget) {
		m_LayoutEngine.ScrollIntoView(m_ScrollTarget);
		m_ScrollTarget = nullptr;
		m_LayoutEngine.RunLayout(m_Root.get(), m_InputRouter.MousePos(), m_InputRouter.MouseDown(), {}, m_DeltaTime);
		m_DrawCompositor.InvalidateAll(m_Root.get());
	}

	if (m_DrawCompositor.RebuildDirty(m_Root.get())) {
		m_DrawListDirty = true;
	}
	m_Dirty = false;
}

void Canvas::MarkNodeDrawDirty(View *node) {
	node->SetDrawDirty();
	MarkDirty();
}

void Canvas::SubmitToQuadBatcher(Graphics::QuadBatcher &r2d, GFX::GfxCommandList &cmd) {
	if (!m_Root) {
		return;
	}
	m_DrawCompositor.Submit(r2d, cmd);
}

void Canvas::OnEvent(Application::Events::Event &e) {
	m_InputRouter.OnEvent(e);
}

View *Canvas::HitTest(vec2 pos) {
	return m_DrawCompositor.HitTest(pos);
}

void Canvas::ScrollIntoView(View *target) {
	m_ScrollTarget = target;
	m_LayoutDirty = true;
	MarkDirty();
}

void Canvas::Update(f32 deltaTime) {
	m_DeltaTime = deltaTime;
	if (!m_Ticking.empty()) {
		for (View *view : m_Ticking) {
			view->OnUpdate(deltaTime);
		}
		Aquila::Rendering::FrameScheduler::Get()->RequestFrame();
	}
	StylePass();
	AnimationPass(deltaTime);
}

void Canvas::Resize(uint32 width, uint32 height) {
	m_Width = width;
	m_Height = height;
	m_DrawCompositor.SetCanvasSize(width, height);
	m_LayoutDirty = true;
	MarkDirty();
	// @media rules depend on viewport size — re-resolve all styles on resize.
	if (m_StyleEngine.GetStyleSheet().HasMediaBlocks()) {
		MarkSubtreeDirty(m_Root.get());
	}
	m_LayoutEngine.SetDimensions(width, height);
}

StyleSheet &Canvas::GetStyleSheet() {
	return m_StyleEngine.GetStyleSheet();
}

View *Canvas::GetRoot() {
	return m_Root.get();
}

} // namespace Aquila::UI::Core
