#include "Aquila/UI/Core/LayoutEngine.h"

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wmissing-designated-field-initializers"
#pragma clang diagnostic ignored "-Wmissing-field-initializers"
#pragma clang diagnostic ignored "-Wgnu-anonymous-struct"
#pragma clang diagnostic ignored "-Wnested-anon-types"
#include "clay.h"
#pragma clang diagnostic pop

namespace Aquila::UI::Core {

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

	Clay_LayoutConfig layout = {
		.sizing = { .width = ToCSizing(cs.width, cs.flexGrow), .height = ToCSizing(cs.height, cs.flexGrow) },
		.padding = ToClayPadding(cs.padding),
		.childGap = static_cast<uint16_t>(cs.gap),
		.childAlignment = { .x = alignX, .y = alignY },
		.layoutDirection = isRow ? CLAY_LEFT_TO_RIGHT : CLAY_TOP_TO_BOTTOM,
	};

	if (cs.width.unit == LengthUnit::Auto || cs.width.unit == LengthUnit::Grow) {
		const f32 minW = cs.minWidth.Resolve(0.f);
		const f32 maxW = cs.maxWidth.Resolve(0.f);
		if (minW > 0.f) {
			layout.sizing.width.size.minMax.min = minW;
		}
		if (maxW > 0.f) {
			layout.sizing.width.size.minMax.max = maxW;
		}
	}
	if (cs.height.unit == LengthUnit::Auto || cs.height.unit == LengthUnit::Grow) {
		const f32 minH = cs.minHeight.Resolve(0.f);
		const f32 maxH = cs.maxHeight.Resolve(0.f);
		if (minH > 0.f) {
			layout.sizing.height.size.minMax.min = minH;
		}
		if (maxH > 0.f) {
			layout.sizing.height.size.minMax.max = maxH;
		}
	}

	return layout;
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

	const uint32 id = node->GetStableId();
	return Clay_ElementId{ .id = id != 0 ? id : 1u };
}

LayoutEngine::LayoutEngine(uint32 width, uint32 height) : m_Width(width), m_Height(height) {
	const uint32 memSize = Clay_MinMemorySize();
	m_ClayMemory.resize(memSize);
	const Clay_Arena arena = Clay_CreateArenaWithCapacityAndMemory(memSize, m_ClayMemory.data());
	m_ClayCtx = Clay_Initialize(arena, { static_cast<f32>(width), static_cast<f32>(height) }, {});
}

void LayoutEngine::SetDimensions(uint32 width, uint32 height) {
	m_Width = width;
	m_Height = height;
	Clay_SetCurrentContext(static_cast<Clay_Context *>(m_ClayCtx));
	Clay_SetLayoutDimensions({ static_cast<f32>(width), static_cast<f32>(height) });
}

void LayoutEngine::RunLayout(View *root, vec2 mousePos, bool mouseDown, vec2 scrollDelta, float deltaTime) {
	Clay_SetCurrentContext(static_cast<Clay_Context *>(m_ClayCtx));
	Clay_SetLayoutDimensions({ static_cast<f32>(m_Width), static_cast<f32>(m_Height) });
	Clay_SetPointerState({ mousePos.x, mousePos.y }, mouseDown);
	Clay_UpdateScrollContainers(true, { scrollDelta.x, scrollDelta.y }, deltaTime);
	Clay_BeginLayout();
	LayoutPass(root);
	Clay_EndLayout(deltaTime);
	UpdateRects(root);
}

void LayoutEngine::ScrollIntoView(View *target) {
	if (!target) {
		return;
	}

	View *container = nullptr;
	for (View *v = target->GetParent(); v; v = v->GetParent()) {
		if (v->GetDisplayStyle().overflow == Overflow::Scroll) {
			container = v;
			break;
		}
	}
	if (!container) {
		return;
	}

	Clay_SetCurrentContext(static_cast<Clay_Context *>(m_ClayCtx));
	Clay_ElementId id = {};
	id.id = container->GetClayId();
	Clay_ScrollContainerData data = Clay_GetScrollContainerData(id);
	if (!data.found || data.scrollPosition == nullptr) {
		return;
	}

	const Rect containerRect = container->GetAbsoluteRect();
	const Rect targetRect = target->GetAbsoluteRect();
	const float rel = targetRect.position.y - containerRect.position.y;
	const float viewH = containerRect.size.y;
	const float nodeH = targetRect.size.y;

	float sy = data.scrollPosition->y;
	if (rel < 0.f) {
		sy -= rel;
	} else if (rel + nodeH > viewH) {
		sy -= (rel + nodeH - viewH);
	}

	const float maxScroll = data.contentDimensions.height - viewH;
	if (maxScroll <= 0.f) {
		sy = 0.f;
	} else if (sy < -maxScroll) {
		sy = -maxScroll;
	} else if (sy > 0.f) {
		sy = 0.f;
	}
	data.scrollPosition->y = sy;
}

void LayoutEngine::LayoutPass(View *node) {
	const ComputedStyle &cs = node->GetDisplayStyle();

	if (cs.display == Display::None) {
		return;
	}

	const Clay_ElementId clayId = MakeElementId(node);
	node->SetClayId(clayId.id);

	auto emitChildren = [&]() {
		for (const auto &child : node->GetChildren()) {
			LayoutPass(child.get());
		}
	};

	Clay_LayoutConfig layout = ToClayLayout(cs);
	const vec2 intrinsic = node->GetIntrinsicSize();
	if (intrinsic.x >= 0.f && cs.width.unit == LengthUnit::Auto) {
		layout.sizing.width = CLAY_SIZING_FIXED(intrinsic.x);
	}
	if (intrinsic.y >= 0.f && cs.height.unit == LengthUnit::Auto) {
		layout.sizing.height = CLAY_SIZING_FIXED(intrinsic.y);
	}

	const Clay_AspectRatioElementConfig aspectCfg = { cs.aspectRatio };

	if (node->HasFloating()) {
		const FloatingConfig &fc = node->GetFloating();

		auto toPoint = [](FloatingAttachPoint p) {
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
				   .aspectRatio = aspectCfg,
				   .floating = floatCfg,
				   .clip = { .horizontal = true, .vertical = true, .childOffset = Clay_GetScrollOffset() } }) {
				emitChildren();
			}
			break;
		case Overflow::Hidden:
			CLAY(clayId,
				 { .layout = layout,
				   .aspectRatio = aspectCfg,
				   .floating = floatCfg,
				   .clip = { .horizontal = true, .vertical = false } }) {
				emitChildren();
			}
			break;
		default:
			CLAY(clayId, { .layout = layout, .aspectRatio = aspectCfg, .floating = floatCfg }) {
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
				 .aspectRatio = aspectCfg,
				 .clip = { .horizontal = true, .vertical = true, .childOffset = Clay_GetScrollOffset() },
			 }) {
			emitChildren();
		}
		break;

	case Overflow::Hidden:
		CLAY(clayId,
			 {
				 .layout = layout,
				 .aspectRatio = aspectCfg,
				 .clip = { .horizontal = true, .vertical = false },
			 }) {
			emitChildren();
		}
		break;

	default:
		CLAY(clayId, { .layout = layout, .aspectRatio = aspectCfg }) {
			emitChildren();
		}
		break;
	}
}

void LayoutEngine::UpdateRects(View *node, vec2 parentAbsPos) {
	const ComputedStyle &cs = node->GetDisplayStyle();
	if (cs.display == Display::None) {
		node->SetLayoutRect({});
		return;
	}

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
		}
		if (myAbsPos != node->GetAbsolutePosition()) {
			node->SetAbsolutePosition(myAbsPos);
			node->MarkSubtreeBoundsDirty();
			node->QueueRedraw();
		}
	}

	for (const auto &child : node->GetChildren()) {
		UpdateRects(child.get(), myAbsPos);
	}
}

} // namespace Aquila::UI::Core
