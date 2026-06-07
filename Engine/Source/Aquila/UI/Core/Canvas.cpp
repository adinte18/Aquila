#include "Aquila/UI/Core/Canvas.h"
#include "Aquila/Rendering/FrameScheduler.h"
#include "Aquila/Application/Events/InputEvent.h"
#include "Aquila/Foundation/Macros.h"
#include "Aquila/Foundation/Math/Math.h"
#include "Aquila/Foundation/Profiler.h"
#include "Aquila/Platform/Input.h"
#include <unordered_set>

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wmissing-designated-field-initializers"
#pragma clang diagnostic ignored "-Wmissing-field-initializers"
#pragma clang diagnostic ignored "-Wgnu-anonymous-struct"
#pragma clang diagnostic ignored "-Wnested-anon-types"
#include "clay.h"
#pragma clang diagnostic pop

namespace Aquila::UI::Core {

using namespace Application::Events;
using namespace Aquila::UI::Rendering;

static Clay_LayoutAlignmentX JustifyToAlignX(JustifyContent justify) {
	switch (justify) {
	case JustifyContent::Center:
		return CLAY_ALIGN_X_CENTER;
	case JustifyContent::End:
		return CLAY_ALIGN_X_RIGHT;
	default:
		return CLAY_ALIGN_X_LEFT;
	}
}

static Clay_LayoutAlignmentY JustifyToAlignY(JustifyContent justify) {
	switch (justify) {
	case JustifyContent::Center:
		return CLAY_ALIGN_Y_CENTER;
	case JustifyContent::End:
		return CLAY_ALIGN_Y_BOTTOM;
	default:
		return CLAY_ALIGN_Y_TOP;
	}
}

static Clay_LayoutAlignmentX AlignToAlignX(AlignItems align) {
	switch (align) {
	case AlignItems::Center:
		return CLAY_ALIGN_X_CENTER;
	case AlignItems::End:
		return CLAY_ALIGN_X_RIGHT;
	default:
		return CLAY_ALIGN_X_LEFT;
	}
}

static Clay_LayoutAlignmentY AlignToAlignY(AlignItems align) {
	switch (align) {
	case AlignItems::Center:
		return CLAY_ALIGN_Y_CENTER;
	case AlignItems::End:
		return CLAY_ALIGN_Y_BOTTOM;
	default:
		return CLAY_ALIGN_Y_TOP;
	}
}

static Clay_SizingAxis ToCSizing(const StyleLength &len, f32 flexGrow = 0.f) {
	switch (len.unit) {
	case LengthUnit::Pixel:
		return CLAY_SIZING_FIXED(len.value);
	case LengthUnit::Percent:
		return CLAY_SIZING_PERCENT(len.value / 100.f);
	case LengthUnit::Grow:
		return CLAY_SIZING_GROW(flexGrow);
	case LengthUnit::Auto:
	default:
		return CLAY_SIZING_FIT(0, 0);
	}
}

static Clay_Padding ToClayPadding(const StyleEdges &edges) {
	auto px = [](const StyleLength &l) -> uint16_t {
		return l.unit == LengthUnit::Pixel ? static_cast<uint16_t>(l.value) : 0u;
	};
	return { .left = px(edges.left), .right = px(edges.right), .top = px(edges.top), .bottom = px(edges.bottom) };
}

static Clay_LayoutConfig ToClayLayout(const ComputedStyle &cs) {
	const bool isRow = (cs.flexDirection == FlexDirection::Row || cs.flexDirection == FlexDirection::RowReverse);
	const Clay_LayoutAlignmentX alignX = isRow ? JustifyToAlignX(cs.justify) : AlignToAlignX(cs.align);
	const Clay_LayoutAlignmentY alignY = isRow ? AlignToAlignY(cs.align) : JustifyToAlignY(cs.justify);

	return {
		.sizing = { .width = ToCSizing(cs.width, cs.flexGrow), .height = ToCSizing(cs.height, cs.flexGrow) },
		.padding = ToClayPadding(cs.padding),
		.childGap = static_cast<uint16_t>(cs.gap),
		.childAlignment = { .x = alignX, .y = alignY },
		.layoutDirection = isRow ? CLAY_LEFT_TO_RIGHT : CLAY_TOP_TO_BOTTOM,
	};
}

static Clay_ElementId MakeElementId(View *node) {
	const std::string &viewId = node->GetId();
	if (!viewId.empty()) {
		const Clay_String s{
			.isStaticallyAllocated = false,
			.length = static_cast<int32>(viewId.size()),
			.chars = viewId.c_str(),
		};
		return CLAY_SID(s);
	}

	// shift out alignment bits so nearby objects don't collide.
	const auto raw = static_cast<uint32>(reinterpret_cast<uintptr_t>(node) >> 4);
	return Clay_ElementId{ .id = raw != 0 ? raw : 1u };
}

Canvas::Canvas(uint32 width, uint32 height) : m_Width(width), m_Height(height) {
	m_Root = CreateUnique<View>();
	m_Root->SetDirtyCallback([this](View *v) { NotifyStyleDirty(v); });
	m_Root->SetAnimationCallback([this](View *v) { NotifyAnimationStarted(v); });
	m_Root->SetDrawDirtyCallback([this](View *v) { MarkNodeDrawDirty(v); });
	m_Root->SetLayoutDirtyCallback([this](View *v) {
		m_LayoutDirty = true;
		MarkNodeDrawDirty(v);
	});
	m_Root->SetFocusRequestCallback([this](View *v) { SetFocus(v); });
	m_Root->SetRemoveCallback([this](View *v) {
		m_StyleCache.Remove(v);
		m_DirtyViews.Remove(v);
		if (m_HoveredView == v) {
			m_HoveredView = nullptr;
		}
		if (m_FocusedView == v) {
			m_FocusedView = nullptr;
		}
	});

	m_DrawList.SetCanvasSize(width, height);

	const uint32 memSize = Clay_MinMemorySize();
	m_ClayMemory.resize(memSize);
	const Clay_Arena arena = Clay_CreateArenaWithCapacityAndMemory(memSize, m_ClayMemory.data());
	m_ClayCtx = Clay_Initialize(arena, { static_cast<f32>(width), static_cast<f32>(height) }, {});

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

void Canvas::SetFocus(View *view) {
	if (m_FocusedView == view) {
		return;
	}
	if (m_FocusedView) {
		m_FocusedView->OnFocusLost();
	}
	m_FocusedView = view;
	if (m_FocusedView) {
		m_FocusedView->OnFocusGained();
	}
}

void Canvas::NotifyStyleDirty(View *view) {
	m_StyleCache.Invalidate(view);
	m_DirtyViews.MarkDirty(view);
	MarkDirty();
}

void Canvas::NotifyAnimationStarted(View *view) {
	for (View *v : m_ActiveAnims) {
		if (v == view) {
			return;
		}
	}
	m_ActiveAnims.push_back(view);
	MarkDirty();
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

static bool AffectsLayout(const ComputedStyle &a, const ComputedStyle &b) {
	return a.width != b.width || a.height != b.height || a.minWidth != b.minWidth || a.maxWidth != b.maxWidth ||
		a.minHeight != b.minHeight || a.maxHeight != b.maxHeight || a.padding != b.padding || a.gap != b.gap ||
		a.flexDirection != b.flexDirection || a.justify != b.justify || a.align != b.align || a.wrap != b.wrap ||
		a.flexGrow != b.flexGrow || a.display != b.display || a.overflow != b.overflow || a.position != b.position ||
		a.top != b.top || a.bottom != b.bottom || a.left != b.left || a.right != b.right || a.fontSize != b.fontSize ||
		a.fontFamily != b.fontFamily || a.zIndex != b.zIndex;
}

void Canvas::StylePass() {
	if (m_DirtyViews.IsEmpty()) {
		return;
	}
	PROFILE_SCOPE("Canvas::StylePass");

	auto depth = [](View *view) {
		int result = 0;
		while (view->GetParent()) {
			view = view->GetParent();
			result++;
		}
		return result;
	};

	std::vector<View *> queue = m_DirtyViews.GetOrdered();
	std::stable_sort(queue.begin(), queue.end(), [&](View *a, View *b) { return depth(a) < depth(b); });
	std::unordered_set<View *> inQueue(queue.begin(), queue.end());

	for (size_t i = 0; i < queue.size(); i++) {
		View *node = queue[i];

		const ComputedStyle *parentStyle = nullptr;
		if (View *parent = node->GetParent()) {
			parentStyle = m_StyleCache.Peek(parent);
			if (!parentStyle) {
				parentStyle = &parent->GetComputedStyle();
			}
			m_StyleCache.RegisterDependency(node, parent);
		}

		StyleSheet::ResolveContext ctx;
		ctx.viewportSize = { static_cast<f32>(m_Width), static_cast<f32>(m_Height) };
		if (View *parent = node->GetParent()) {
			ctx.containerSize = parent->GetLayoutRect().size;
		}

		const ComputedStyle old = node->GetComputedStyle();
		const ComputedStyle &resolved =
			m_StyleCache.Get(node, [&]() { return m_StyleSheet.Resolve(*node, parentStyle, ctx); });

		if (resolved != old) {
			if (!m_LayoutDirty && AffectsLayout(old, resolved)) {
				m_LayoutDirty = true;
			}
			node->SetComputedStyle(resolved);
			node->OnStyleResolved();
			MarkNodeDrawDirty(node);
			MarkDirty();

			for (const auto &child : node->GetChildren()) {
				View *c = child.get();
				if (inQueue.insert(c).second) {
					queue.push_back(c);
				}
			}
		} else {
			node->SetComputedStyle(resolved);
			node->OnStyleResolved();
		}
	}

	m_DirtyViews.Clear();
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

void Canvas::ClayLayoutPass(View *node) {
	const ComputedStyle &cs = node->GetDisplayStyle();

	if (cs.display == Display::None) {
		return;
	}

	const Clay_ElementId clayId = MakeElementId(node);
	node->SetClayId(clayId.id);

	auto emitChildren = [&]() {
		for (const auto &child : node->GetChildren()) {
			ClayLayoutPass(child.get());
		}
	};

	// For leaf nodes (e.g. Label) that report an intrinsic size, use it as a
	// FIXED size on any axis that doesn't have an explicit CSS dimension set.
	// This lets font-size and text content drive layout automatically.
	Clay_LayoutConfig layout = ToClayLayout(cs);
	const vec2 intrinsic = node->GetIntrinsicSize();
	if (intrinsic.x >= 0.f && cs.width.unit == LengthUnit::Auto) {
		layout.sizing.width = CLAY_SIZING_FIXED(intrinsic.x);
	}
	if (intrinsic.y >= 0.f && cs.height.unit == LengthUnit::Auto) {
		layout.sizing.height = CLAY_SIZING_FIXED(intrinsic.y);
	}

	if (node->HasFloating()) {
		const FloatingConfig &fc = node->GetFloating();

		auto toPoint = [](FloatingAttachPoint p) -> Clay_FloatingAttachPointType {
			switch (p) {
			case FloatingAttachPoint::LeftTop:
				return CLAY_ATTACH_POINT_LEFT_TOP;
			case FloatingAttachPoint::LeftCenter:
				return CLAY_ATTACH_POINT_LEFT_CENTER;
			case FloatingAttachPoint::LeftBottom:
				return CLAY_ATTACH_POINT_LEFT_BOTTOM;
			case FloatingAttachPoint::CenterTop:
				return CLAY_ATTACH_POINT_CENTER_TOP;
			case FloatingAttachPoint::Center:
				return CLAY_ATTACH_POINT_CENTER_CENTER;
			case FloatingAttachPoint::CenterBottom:
				return CLAY_ATTACH_POINT_CENTER_BOTTOM;
			case FloatingAttachPoint::RightTop:
				return CLAY_ATTACH_POINT_RIGHT_TOP;
			case FloatingAttachPoint::RightCenter:
				return CLAY_ATTACH_POINT_RIGHT_CENTER;
			case FloatingAttachPoint::RightBottom:
				return CLAY_ATTACH_POINT_RIGHT_BOTTOM;
			}
			return CLAY_ATTACH_POINT_LEFT_TOP;
		};

		const Clay_FloatingElementConfig floatCfg = {
			.offset = { fc.offset.x, fc.offset.y },
			.zIndex = fc.zIndex,
			.attachTo = fc.attachTo == FloatingAttachTo::Root ? CLAY_ATTACH_TO_ROOT : CLAY_ATTACH_TO_PARENT,
			.attachPoints = { .element = toPoint(fc.elementPoint), .parent = toPoint(fc.parentPoint) },
			.pointerCaptureMode = CLAY_POINTER_CAPTURE_MODE_CAPTURE,
		};

		switch (cs.overflow) {
		case Overflow::Scroll:
			CLAY(clayId,
				 { .layout = layout,
				   .floating = floatCfg,
				   .clip = { .horizontal = true, .vertical = true, .childOffset = Clay_GetScrollOffset() } }) {
				emitChildren();
			}
			break;
		case Overflow::Hidden:
			CLAY(clayId, { .layout = layout, .floating = floatCfg, .clip = { .horizontal = true, .vertical = true } }) {
				emitChildren();
			}
			break;
		default:
			CLAY(clayId, { .layout = layout, .floating = floatCfg }) {
				emitChildren();
			}
			break;
		}
		return;
	}

	switch (cs.overflow) {
	case Overflow::Scroll:
		CLAY(clayId,
			 {
				 .layout = layout,
				 .clip = { .horizontal = true, .vertical = true, .childOffset = Clay_GetScrollOffset() },
			 }) {
			emitChildren();
		}
		break;

	case Overflow::Hidden:
		CLAY(clayId,
			 {
				 .layout = layout,
				 .clip = { .horizontal = true, .vertical = true },
			 }) {
			emitChildren();
		}
		break;

	default:
		CLAY(clayId, { .layout = layout }) {
			emitChildren();
		}
		break;
	}
}

bool Canvas::ClayUpdateRects(View *node, vec2 parentAbsPos) {
	const ComputedStyle &cs = node->GetDisplayStyle();
	if (cs.display == Display::None) {
		node->SetLayoutRect({});
		return false;
	}

	bool changed = false;
	const Clay_ElementId clayId = MakeElementId(node);
	const Clay_ElementData data = Clay_GetElementData(clayId);

	vec2 myAbsPos = parentAbsPos;
	if (data.found) {
		const Clay_BoundingBox &bb = data.boundingBox;
		myAbsPos = { bb.x, bb.y };
		const Rect newRect = { .position = myAbsPos - parentAbsPos, .size = { bb.width, bb.height } };
		if (newRect != node->GetLayoutRect()) {
			node->SetLayoutRect(newRect);
			node->MarkSubtreeBoundsDirty();
			changed = true;
		}
		if (myAbsPos != node->GetAbsolutePosition()) {
			node->SetAbsolutePosition(myAbsPos);
			node->MarkSubtreeBoundsDirty();
			MarkNodeDrawDirty(node);
		}
	}

	for (const auto &child : node->GetChildren()) {
		changed |= ClayUpdateRects(child.get(), myAbsPos);
	}

	return changed;
}

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

void Canvas::Compute() {
	if (!m_Root || !m_Dirty) {
		return;
	}

	if (m_LayoutDirty) {
		Clay_SetCurrentContext(static_cast<Clay_Context *>(m_ClayCtx));
		Clay_SetLayoutDimensions({ static_cast<f32>(m_Width), static_cast<f32>(m_Height) });
		Clay_SetPointerState({ m_MousePos.x, m_MousePos.y }, m_MouseDown);
		Clay_UpdateScrollContainers(true, { m_ScrollDelta.x, m_ScrollDelta.y }, m_DeltaTime);
		m_ScrollDelta = {};
		Clay_BeginLayout();
		ClayLayoutPass(m_Root.get());
		Clay_EndLayout(m_DeltaTime);
		ClayUpdateRects(m_Root.get());
		m_LayoutDirty = false;

		// @container rules depend on element sizes — re-resolve immediately after
		// layout so rules see the current frame's container sizes.
		if (m_StyleSheet.HasContainerBlocks()) {
			MarkSubtreeDirty(m_Root.get());
			StylePass();
		}

		// Rebuild node lists. m_PerNodeCmds is cleared first so _CullCanvasItem
		// finds no commands and only populates m_CanvasItems / m_CanvasLayers.
		m_PerNodeCmds.clear();
		for (auto &b : m_ZBuckets) {
			b.clear();
		}
		m_CanvasItems.clear();
		m_CanvasLayers.clear();
		{
			const Rect canvasBounds = { { 0.f, 0.f }, { static_cast<f32>(m_Width), static_cast<f32>(m_Height) } };
			_CullCanvasItem(m_Root.get(), 0, &canvasBounds);
		}
		std::vector<View *> floatRoots;
		GatherFloatingRoots(m_Root.get(), floatRoots);
		std::stable_sort(floatRoots.begin(), floatRoots.end(),
						 [](View *a, View *b) { return a->GetFloating().zIndex < b->GetFloating().zIndex; });
		for (View *root : floatRoots) {
			_CollectCanvasLayer(root);
		}
		for (View *v : m_CanvasItems) {
			v->SetDrawDirty();
		}
		for (View *v : m_CanvasLayers) {
			v->SetDrawDirty();
		}
	}

	bool anyRebuilt = false;
	auto rebuildNode = [&](View *v) {
		if (v->IsDrawDirty()) {
			DrawList capture;
			capture.SetCanvasSize(m_Width, m_Height);
			v->OnDrawSelf(capture);
			auto cmds = capture.TakeCommands();
			std::stable_sort(cmds.begin(), cmds.end(),
							 [](const DrawCmd &a, const DrawCmd &b) { return a.zOrder < b.zOrder; });
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
		for (auto &b : m_ZBuckets) {
			b.clear();
		}
		m_CanvasItems.clear();
		m_CanvasLayers.clear();
		{
			const Rect canvasBounds = { { 0.f, 0.f }, { static_cast<f32>(m_Width), static_cast<f32>(m_Height) } };
			_CullCanvasItem(m_Root.get(), 0, &canvasBounds);
		}
		std::vector<View *> floatRoots;
		GatherFloatingRoots(m_Root.get(), floatRoots);
		std::stable_sort(floatRoots.begin(), floatRoots.end(),
						 [](View *a, View *b) { return a->GetFloating().zIndex < b->GetFloating().zIndex; });
		for (View *root : floatRoots) {
			_CollectCanvasLayer(root);
		}

		m_DrawList.Clear();
		for (const auto &b : m_ZBuckets) {
			for (const DrawCmd &cmd : b) {
				m_DrawList.AppendCmd(cmd);
			}
		}
		for (View *v : m_CanvasLayers) {
			auto it = m_PerNodeCmds.find(v);
			if (it != m_PerNodeCmds.end()) {
				for (const DrawCmd &cmd : it->second) {
					m_DrawList.AppendCmd(cmd);
				}
			}
		}
		m_DrawListDirty = true;
	}
	m_Dirty = false;
}

void Canvas::MarkNodeDrawDirty(View *node) {
	node->SetDrawDirty();
	MarkDirty();
}

void Canvas::_CullCanvasItem(View *node, int32 parentEffectiveZ, const Rect *clipRect) {
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
	const int32 bucketIdx = std::clamp(effectiveZ, kZMin, kZMax) - kZMin;

	if (clipRect == nullptr || clipRect->Overlaps(node->GetAbsoluteRect())) {
		auto it = m_PerNodeCmds.find(node);
		if (it != m_PerNodeCmds.end()) {
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

		DrawCmd clipPush{};
		clipPush.type = UICommandType::ClipPush;
		clipPush.rect = ownClip;
		m_ZBuckets[bucketIdx].push_back(clipPush);
	}

	for (const auto &child : node->GetChildren()) {
		_CullCanvasItem(child.get(), effectiveZ, childClip);
	}

	if (clipsChildren) {
		DrawCmd clipPop{};
		clipPop.type = UICommandType::ClipPop;
		clipPop.rect = clipRect != nullptr ? *clipRect : Rect{};
		m_ZBuckets[bucketIdx].push_back(clipPop);
	}
}

void Canvas::_CollectCanvasLayer(View *node) {
	m_CanvasLayers.push_back(node);
	for (const auto &child : node->GetChildren()) {
		_CollectCanvasLayerSubtree(child.get());
	}
}

void Canvas::_CollectCanvasLayerSubtree(View *node) {
	if (node->GetDisplayStyle().display == Display::None) {
		return;
	}
	if (!node->IsVisible()) {
		return;
	}
	m_CanvasLayers.push_back(node);
	for (const auto &child : node->GetChildren()) {
		_CollectCanvasLayerSubtree(child.get());
	}
}

void Canvas::SubmitToQuadBatcher(Graphics::QuadBatcher &r2d, GFX::GfxCommandList &cmd) {
	if (!m_Root || m_DrawList.IsEmpty()) {
		return;
	}
	m_DrawList.Submit(r2d, cmd);
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

View *Canvas::HitTest(vec2 pos) {
	for (int i = static_cast<int>(m_CanvasLayers.size()) - 1; i >= 0; --i) {
		View *v = m_CanvasLayers[i];
		if (v->GetAbsoluteRect().Contains(pos)) {
			return v;
		}
	}
	for (int i = static_cast<int>(m_CanvasItems.size()) - 1; i >= 0; --i) {
		View *v = m_CanvasItems[i];
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

void Canvas::OnEvent(Application::Events::Event &e) {
	EventDispatcher dispatcher(e);

	dispatcher.Dispatch<MouseMovedEvent>([this](MouseMovedEvent &e) {
		m_MousePos = { e.GetX(), e.GetY() };
		View *hit = HitTest(m_MousePos);
		if (hit != m_HoveredView) {
			if (m_HoveredView) {
				m_HoveredView->OnMouseLeave();
			}
			m_HoveredView = hit;
			if (m_HoveredView) {
				m_HoveredView->OnMouseEnter();
			}
			MarkDirty();
		}
		if (m_FocusedView && m_FocusedView->IsPressed()) {
			m_FocusedView->OnMouseMove(m_MousePos);
		}
		return false;
	});

	dispatcher.Dispatch<MouseButtonPressedEvent>([this](MouseButtonPressedEvent &e) {
		if (e.GetMouseButton() == MouseButton::Left) {
			m_MouseDown = true;
		}
		if (!m_HoveredView) {
			return false;
		}
		SetFocus(m_HoveredView);
		m_FocusedView->OnMousePress(e.GetMouseButton(), m_MousePos);
		return false;
	});

	dispatcher.Dispatch<MouseButtonReleasedEvent>([this](MouseButtonReleasedEvent &e) {
		if (e.GetMouseButton() == MouseButton::Left) {
			m_MouseDown = false;
		}
		if (m_HoveredView) {
			m_HoveredView->OnMouseRelease(e.GetMouseButton(), m_MousePos);
		}
		// Clear pressed state on the focused view even when the mouse released outside its bounds.
		if (m_FocusedView && m_FocusedView != m_HoveredView) {
			m_FocusedView->OnMouseRelease(e.GetMouseButton(), m_MousePos);
		}
		return false;
	});

	dispatcher.Dispatch<MouseScrolledEvent>([this](MouseScrolledEvent &e) {
		m_ScrollDelta += vec2(e.GetXOffset(), e.GetYOffset());
		m_LayoutDirty = true;
		MarkDirty();
		return m_HoveredView != nullptr && !m_HoveredView->GetPassThroughScroll();
	});

	dispatcher.Dispatch<KeyPressedEvent>([this](KeyPressedEvent &e) {
		if (m_FocusedView) {
			m_FocusedView->OnKeyPress(e.GetKeyCode(), e.GetMods());
			return true; // consumed — prevents the engine layer from also acting on this key
		}
		return false;
	});

	dispatcher.Dispatch<KeyReleasedEvent>([this](KeyReleasedEvent &e) {
		if (m_FocusedView) {
			m_FocusedView->OnKeyRelease(e.GetKeyCode());
			return true;
		}
		return false;
	});

	dispatcher.Dispatch<KeyTypedEvent>([this](KeyTypedEvent &e) {
		if (m_FocusedView) {
			m_FocusedView->OnCharInput(static_cast<uint32>(e.GetKeyCode()));
			return true;
		}
		return false;
	});
}

void Canvas::Update(f32 deltaTime) {
	m_DeltaTime = deltaTime;
	StylePass();
	AnimationPass(deltaTime);
}

void Canvas::Resize(uint32 width, uint32 height) {
	m_Width = width;
	m_Height = height;
	m_DrawList.SetCanvasSize(width, height);
	m_LayoutDirty = true;
	MarkDirty();
	// @media rules depend on viewport size — re-resolve all styles on resize.
	if (m_StyleSheet.HasMediaBlocks()) {
		MarkSubtreeDirty(m_Root.get());
	}
	Clay_SetCurrentContext(static_cast<Clay_Context *>(m_ClayCtx));
	Clay_SetLayoutDimensions({ static_cast<f32>(width), static_cast<f32>(height) });
}

StyleSheet &Canvas::GetStyleSheet() {
	return m_StyleSheet;
}

View *Canvas::GetRoot() {
	return m_Root.get();
}

} // namespace Aquila::UI::Core
