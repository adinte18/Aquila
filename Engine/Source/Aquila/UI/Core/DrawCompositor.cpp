#include "Aquila/UI/Core/DrawCompositor.h"
#include "Aquila/Foundation/Math/Math.h"

namespace Aquila::UI::Core {

static void GatherFloatingRoots(View *node, std::vector<View *> &roots) {
	if (node->GetDisplayStyle().display == Display::None) {
		return;
	}
	if (node->HasFloating()) {
		roots.push_back(node);
		return;
	}
	for (const auto &c : node->GetChildren()) {
		GatherFloatingRoots(c.get(), roots);
	}
}

static bool WithinAllClipAncestors(View *node, vec2 pos) {
	View *p = node->GetParent();
	while (p != nullptr) {
		const Overflow overflow = p->GetDisplayStyle().overflow;
		if (overflow == Overflow::Scroll || overflow == Overflow::Hidden) {
			if (!p->GetAbsoluteRect().Contains(pos)) {
				return false;
			}
		}
		p = p->GetParent();
	}
	return true;
}

void DrawCompositor::SetCanvasSize(uint32 width, uint32 height) {
	m_Width = width;
	m_Height = height;
	m_DrawList.SetCanvasSize(width, height);
}

void DrawCompositor::RebuildLists(View *root) {
	for (auto &b : m_ZBuckets) {
		b.clear();
	}
	m_CanvasItems.clear();
	m_CanvasLayers.clear();
	m_FloatRoots.clear();
	{
		const Rect canvasBounds = { { 0.f, 0.f }, { static_cast<f32>(m_Width), static_cast<f32>(m_Height) } };
		Cull(root, 0, &canvasBounds);
	}
	GatherFloatingRoots(root, m_FloatRoots);
	std::ranges::stable_sort(m_FloatRoots.begin(), m_FloatRoots.end(),
							 [](View *a, View *b) { return a->GetFloating().zIndex < b->GetFloating().zIndex; });
	for (View *floatRoot : m_FloatRoots) {
		CollectLayer(floatRoot);
	}
}

void DrawCompositor::InvalidateAll(View *root) {
	m_PerNodeCmds.clear();
	RebuildLists(root);
	for (View *v : m_CanvasItems) {
		v->SetDrawDirty();
	}
	for (View *v : m_CanvasLayers) {
		v->SetDrawDirty();
	}
}

bool DrawCompositor::RebuildDirty(View *root) {
	bool anyRebuilt = false;
	auto rebuildNode = [&](View *v) {
		if (v->IsDrawDirty()) {
			DrawList capture;
			capture.SetCanvasSize(m_Width, m_Height);
			v->OnDrawSelf(capture);
			auto cmds = capture.TakeCommands();
			std::ranges::stable_sort(cmds.begin(), cmds.end(),
									 [](const DrawCmd &a, const DrawCmd &b) { return DrawCmdZOrder(a) < DrawCmdZOrder(b); });
			m_PerNodeCmds[v] = std::move(cmds);
			v->ClearDrawDirty();
			anyRebuilt = true;
		}
	};
	for (View *v : m_CanvasItems) {
		rebuildNode(v);
	}
	for (View *v : m_CanvasLayers) {
		rebuildNode(v);
	}

	if (anyRebuilt) {
		RebuildLists(root);

		m_DrawList.Clear();
		for (const auto &b : m_ZBuckets) {
			for (const DrawCmd &cmd : b) {
				m_DrawList.AppendCmd(cmd);
			}
		}
		for (View *floatRoot : m_FloatRoots) {
			EmitFloatingLayer(floatRoot, nullptr);
		}
	}

	return anyRebuilt;
}

void DrawCompositor::Cull(View *node, int32 parentEffectiveZ, const Rect *clipRect) {
	if (node->GetDisplayStyle().display == Display::None) {
		return;
	}
	if (!node->IsVisible()) {
		return;
	}
	if (node->HasFloating()) {
		return;
	}

	if (clipRect != nullptr && !clipRect->Overlaps(node->GetSubtreeBounds())) {
		return;
	}

	const int32 effectiveZ = parentEffectiveZ + node->GetComputedStyle().zIndex;
	const int32 bucketIdx =
		Math::Clamp(effectiveZ, SharedConstants::Z_MIN, SharedConstants::Z_MAX) - SharedConstants::Z_MIN;

	if (clipRect == nullptr || clipRect->Overlaps(node->GetAbsoluteRect())) {
		if (auto it = m_PerNodeCmds.find(node); it != m_PerNodeCmds.end()) {
			for (const DrawCmd &cmd : it->second) {
				m_ZBuckets[bucketIdx].push_back(cmd);
			}
		}
		m_CanvasItems.push_back(node);
	}

	const Rect *childClip = clipRect;
	Rect ownClip;
	bool clipsChildren = false;
	const Overflow overflow = node->GetDisplayStyle().overflow;
	if (overflow == Overflow::Scroll || overflow == Overflow::Hidden) {
		ownClip = node->GetAbsoluteRect();
		if (clipRect != nullptr) {
			ownClip = clipRect->Intersect(ownClip);
			if (ownClip.IsEmpty()) {
				return;
			}
		}
		childClip = &ownClip;
		clipsChildren = true;

		ClipPushCmd clipPush;
		clipPush.rect = ownClip;
		m_ZBuckets[bucketIdx].push_back(clipPush);
	}

	for (const auto &child : node->GetChildren()) {
		Cull(child.get(), effectiveZ, childClip);
	}

	if (clipsChildren) {
		ClipPopCmd clipPop;
		clipPop.rect = clipRect != nullptr ? *clipRect : Rect{};
		m_ZBuckets[bucketIdx].push_back(clipPop);
	}
}

void DrawCompositor::CollectLayer(View *node) {
	m_CanvasLayers.push_back(node);
	for (const auto &child : node->GetChildren()) {
		CollectLayerSubtree(child.get());
	}
}

void DrawCompositor::CollectLayerSubtree(View *node) {
	if (node->GetDisplayStyle().display == Display::None) {
		return;
	}
	if (!node->IsVisible()) {
		return;
	}
	m_CanvasLayers.push_back(node);
	for (const auto &child : node->GetChildren()) {
		CollectLayerSubtree(child.get());
	}
}

void DrawCompositor::EmitFloatingLayer(View *node, const Rect *clipRect) {
	if (node->GetDisplayStyle().display == Display::None) {
		return;
	}
	if (!node->IsVisible()) {
		return;
	}

	if (auto it = m_PerNodeCmds.find(node); it != m_PerNodeCmds.end()) {
		for (const DrawCmd &cmd : it->second) {
			m_DrawList.AppendCmd(cmd);
		}
	}

	const Rect *childClip = clipRect;
	Rect ownClip;
	bool clipsChildren = false;
	const Overflow overflow = node->GetDisplayStyle().overflow;
	if (overflow == Overflow::Scroll || overflow == Overflow::Hidden) {
		ownClip = node->GetAbsoluteRect();
		if (clipRect != nullptr) {
			ownClip = clipRect->Intersect(ownClip);
		}
		childClip = &ownClip;
		clipsChildren = true;

		ClipPushCmd clipPush;
		clipPush.rect = ownClip;
		m_DrawList.AppendCmd(clipPush);
	}

	for (const auto &child : node->GetChildren()) {
		EmitFloatingLayer(child.get(), childClip);
	}

	if (clipsChildren) {
		ClipPopCmd clipPop;
		clipPop.rect = clipRect != nullptr ? *clipRect : Rect{};
		m_DrawList.AppendCmd(clipPop);
	}
}

View *DrawCompositor::HitTest(vec2 pos) const {
	for (int i = static_cast<int>(m_CanvasLayers.size()) - 1; i >= 0; --i) {
		View *v = m_CanvasLayers[i];

		if (v->IsSkippingHitTest()) {
			continue;
		}

		if (!v->GetAbsoluteRect().Contains(pos)) {
			continue;
		}

		View *p = v->GetParent();
		while (p) {
			if (p->IsInputLeaf() && p->GetAbsoluteRect().Contains(pos)) {
				v = p;
			}
			p = p->GetParent();
		}
		return v;
	}

	for (int i = static_cast<int>(m_CanvasItems.size()) - 1; i >= 0; --i) {
		View *v = m_CanvasItems[i];

		if (v->IsSkippingHitTest()) {
			continue;
		}

		if (!v->GetAbsoluteRect().Contains(pos)) {
			continue;
		}
		if (!WithinAllClipAncestors(v, pos)) {
			continue;
		}
		View *p = v->GetParent();
		while (p) {
			if (p->IsInputLeaf() && p->GetAbsoluteRect().Contains(pos)) {
				v = p;
			}
			p = p->GetParent();
		}
		return v;
	}
	return nullptr;
}

void DrawCompositor::Submit(Graphics::QuadBatcher &r2d, GFX::GfxCommandList &cmd) {
	if (m_DrawList.IsEmpty()) {
		return;
	}
	m_DrawList.Submit(r2d, cmd);
}

} // namespace Aquila::UI::Core
