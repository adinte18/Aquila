#include "Aquila/UI/Core/LayoutEngine.h"

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wmissing-designated-field-initializers"
#pragma clang diagnostic ignored "-Wmissing-field-initializers"
#pragma clang diagnostic ignored "-Wgnu-anonymous-struct"
#pragma clang diagnostic ignored "-Wnested-anon-types"
#include "clay.h"
#pragma clang diagnostic pop

namespace Aquila::UI::Core {

static Clay_LayoutAlignmentX justify_to_align_x(JustifyContent justify) {
	switch (justify) {
	case JustifyContent::Center:
		return CLAY_ALIGN_X_CENTER;
	case JustifyContent::End:
		return CLAY_ALIGN_X_RIGHT;
	default:
		return CLAY_ALIGN_X_LEFT;
	}
}

static Clay_LayoutAlignmentY justify_to_align_y(JustifyContent justify) {
	switch (justify) {
	case JustifyContent::Center:
		return CLAY_ALIGN_Y_CENTER;
	case JustifyContent::End:
		return CLAY_ALIGN_Y_BOTTOM;
	default:
		return CLAY_ALIGN_Y_TOP;
	}
}

static Clay_LayoutAlignmentX align_to_align_x(AlignItems align) {
	switch (align) {
	case AlignItems::Center:
		return CLAY_ALIGN_X_CENTER;
	case AlignItems::End:
		return CLAY_ALIGN_X_RIGHT;
	default:
		return CLAY_ALIGN_X_LEFT;
	}
}

static Clay_LayoutAlignmentY align_to_align_y(AlignItems align) {
	switch (align) {
	case AlignItems::Center:
		return CLAY_ALIGN_Y_CENTER;
	case AlignItems::End:
		return CLAY_ALIGN_Y_BOTTOM;
	default:
		return CLAY_ALIGN_Y_TOP;
	}
}

static Clay_SizingAxis to_c_sizing(const StyleLength &len, F32 flex_grow = 0.F) {
	switch (len.unit) {
	case LengthUnit::Pixel:
		return CLAY_SIZING_FIXED(len.value);
	case LengthUnit::Percent:
		return CLAY_SIZING_PERCENT(len.value / 100.F);
	case LengthUnit::Grow:
		return CLAY_SIZING_GROW(.min = flex_grow);
	case LengthUnit::Auto:
	default:
		return CLAY_SIZING_FIT(0, 0);
	}
}

static Clay_Padding to_clay_padding(const StyleEdges &edges) {
	auto px = [](const StyleLength &l) -> uint16_t {
		return l.unit == LengthUnit::Pixel ? static_cast<uint16_t>(l.value) : 0u;
	};
	return { .left = px(edges.left), .right = px(edges.right), .top = px(edges.top), .bottom = px(edges.bottom) };
}

static Clay_LayoutConfig to_clay_layout(const ComputedStyle &cs) {
	const bool is_row = (cs.flex_direction == FlexDirection::Row || cs.flex_direction == FlexDirection::RowReverse);
	const Clay_LayoutAlignmentX align_x = is_row ? justify_to_align_x(cs.justify) : align_to_align_x(cs.align);
	const Clay_LayoutAlignmentY align_y = is_row ? align_to_align_y(cs.align) : justify_to_align_y(cs.justify);

	Clay_LayoutConfig layout = {
		.sizing = { .width = to_c_sizing(cs.width, cs.flex_grow), .height = to_c_sizing(cs.height, cs.flex_grow) },
		.padding = to_clay_padding(cs.padding),
		.childGap = static_cast<uint16_t>(cs.gap),
		.childAlignment = { .x = align_x, .y = align_y },
		.layoutDirection = is_row ? CLAY_LEFT_TO_RIGHT : CLAY_TOP_TO_BOTTOM,
	};

	if (cs.width.unit == LengthUnit::Auto || cs.width.unit == LengthUnit::Grow) {
		const F32 min_w = cs.min_width.resolve(0.F);
		const F32 max_w = cs.max_width.resolve(0.F);
		if (min_w > 0.F) {
			layout.sizing.width.size.minMax.min = min_w;
		}
		if (max_w > 0.F) {
			layout.sizing.width.size.minMax.max = max_w;
		}
	}
	if (cs.height.unit == LengthUnit::Auto || cs.height.unit == LengthUnit::Grow) {
		const F32 min_h = cs.min_height.resolve(0.F);
		const F32 max_h = cs.max_height.resolve(0.F);
		if (min_h > 0.F) {
			layout.sizing.height.size.minMax.min = min_h;
		}
		if (max_h > 0.F) {
			layout.sizing.height.size.minMax.max = max_h;
		}
	}

	return layout;
}

static Clay_ElementId make_element_id(View *node) {
	const std::string &view_id = node->get_id();
	if (!view_id.empty()) {
		const Clay_String s{
			.isStaticallyAllocated = false,
			.length = static_cast<Int32>(view_id.size()),
			.chars = view_id.c_str(),
		};
		return CLAY_SID(s);
	}

	const Uint32 id = node->get_stable_id();
	return Clay_ElementId{ .id = id != 0 ? id : 1u };
}

LayoutEngine::LayoutEngine(Uint32 width, Uint32 height) : m_width(width), m_height(height) {
	const Uint32 mem_size = Clay_MinMemorySize();
	m_clay_memory.resize(mem_size);
	const Clay_Arena arena = Clay_CreateArenaWithCapacityAndMemory(mem_size, m_clay_memory.data());
	m_clay_ctx = Clay_Initialize(arena, { static_cast<F32>(width), static_cast<F32>(height) }, {});
}

void LayoutEngine::set_dimensions(Uint32 width, Uint32 height) {
	m_width = width;
	m_height = height;
	Clay_SetCurrentContext(static_cast<Clay_Context *>(m_clay_ctx));
	Clay_SetLayoutDimensions({ static_cast<F32>(width), static_cast<F32>(height) });
}

void LayoutEngine::run_layout(View *root, Vec2 mouse_pos, bool mouse_down, Vec2 scroll_delta, float delta_time) {
	Clay_SetCurrentContext(static_cast<Clay_Context *>(m_clay_ctx));
	Clay_SetLayoutDimensions({ static_cast<F32>(m_width), static_cast<F32>(m_height) });
	Clay_SetPointerState({ mouse_pos.x, mouse_pos.y }, mouse_down);
	Clay_UpdateScrollContainers(true, { scroll_delta.x, scroll_delta.y }, delta_time);
	Clay_BeginLayout();
	//
	layout_pass(root);
	//
	Clay_EndLayout(delta_time);
	m_size_changed = false;
	update_rects(root);
}

void LayoutEngine::scroll_into_view(View *target) {
	if (target == nullptr) {
		return;
	}

	View *container = nullptr;
	for (View *v = target->get_parent(); v; v = v->get_parent()) {
		if (v->get_display_style().overflow == Overflow::Scroll) {
			container = v;
			break;
		}
	}
	if (container == nullptr) {
		return;
	}

	Clay_SetCurrentContext(static_cast<Clay_Context *>(m_clay_ctx));
	Clay_ElementId id = {};
	id.id = container->get_clay_id();
	Clay_ScrollContainerData data = Clay_GetScrollContainerData(id);
	if (!data.found || data.scrollPosition == nullptr) {
		return;
	}

	const Rect container_rect = container->get_absolute_rect();
	const Rect target_rect = target->get_absolute_rect();
	const float rel = target_rect.position.y - container_rect.position.y;
	const float view_h = container_rect.size.y;
	const float node_h = target_rect.size.y;

	float sy = data.scrollPosition->y;
	if (rel < 0.F) {
		sy -= rel;
	} else if (rel + node_h > view_h) {
		sy -= (rel + node_h - view_h);
	}

	const float max_scroll = data.contentDimensions.height - view_h;
	if (max_scroll <= 0.F) {
		sy = 0.F;
	} else if (sy < -max_scroll) {
		sy = -max_scroll;
	} else if (sy > 0.F) {
		sy = 0.F;
	}
	data.scrollPosition->y = sy;
}

void LayoutEngine::layout_pass(View *node) {
	const ComputedStyle &cs = node->get_display_style();

	if (cs.display == Display::None) {
		return;
	}

	const Clay_ElementId clay_id = make_element_id(node);
	node->set_clay_id(clay_id.id);

	auto emit_children = [&]() {
		for (const auto &child : node->get_children()) {
			layout_pass(child.get());
		}
	};

	Clay_LayoutConfig layout = to_clay_layout(cs);
	const Vec2 intrinsic = node->get_intrinsic_size();
	if (intrinsic.x >= 0.F && cs.width.unit == LengthUnit::Auto) {
		layout.sizing.width = CLAY_SIZING_FIXED(intrinsic.x);
	}
	if (intrinsic.y >= 0.F && cs.height.unit == LengthUnit::Auto) {
		layout.sizing.height = CLAY_SIZING_FIXED(intrinsic.y);
	}

	const Clay_AspectRatioElementConfig aspect_cfg = { cs.aspect_ratio };

	if (node->has_floating()) {
		const FloatingConfig &fc = node->get_floating();

		auto to_point = [](FloatingAttachPoint p) {
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

		const Clay_FloatingElementConfig float_cfg = {
			.offset = { fc.offset.x, fc.offset.y },
			.zIndex = fc.z_index,
			.attachTo = fc.attach_to == FloatingAttachTo::Root ? CLAY_ATTACH_TO_ROOT : CLAY_ATTACH_TO_PARENT,
			.attachPoints = { .element = to_point(fc.element_point), .parent = to_point(fc.parent_point) },
			.pointerCaptureMode = CLAY_POINTER_CAPTURE_MODE_CAPTURE,
		};

		switch (cs.overflow) {
		case Overflow::Scroll:
			CLAY(clay_id,
				 { .layout = layout,
				   .aspectRatio = aspect_cfg,
				   .floating = float_cfg,
				   .clip = { .horizontal = true, .vertical = true, .childOffset = Clay_GetScrollOffset() } }) {
				emit_children();
			}
			break;
		case Overflow::Hidden:
			CLAY(clay_id,
				 { .layout = layout,
				   .aspectRatio = aspect_cfg,
				   .floating = float_cfg,
				   .clip = { .horizontal = true, .vertical = false } }) {
				emit_children();
			}
			break;
		default:
			CLAY(clay_id, { .layout = layout, .aspectRatio = aspect_cfg, .floating = float_cfg }) {
				emit_children();
			}
			break;
		}
		return;
	}

	switch (cs.overflow) {
	case Overflow::Scroll:
		CLAY(clay_id,
			 {
				 .layout = layout,
				 .aspectRatio = aspect_cfg,
				 .clip = { .horizontal = true, .vertical = true, .childOffset = Clay_GetScrollOffset() },
			 }) {
			emit_children();
		}
		break;

	case Overflow::Hidden:
		CLAY(clay_id,
			 {
				 .layout = layout,
				 .aspectRatio = aspect_cfg,
				 .clip = { .horizontal = true, .vertical = false },
			 }) {
			emit_children();
		}
		break;

	default:
		CLAY(clay_id, { .layout = layout, .aspectRatio = aspect_cfg }) {
			emit_children();
		}
		break;
	}
}

void LayoutEngine::update_rects(View *node, Vec2 parent_clay_pos, Vec2 accumulated_offset) {
	const ComputedStyle &cs = node->get_display_style();
	if (cs.display == Display::None) {
		node->set_layout_rect({});
		return;
	}

	const Clay_ElementId clay_id = make_element_id(node);
	const Clay_ElementData data = Clay_GetElementData(clay_id);

	Vec2 clay_pos = parent_clay_pos;
	const Vec2 subtree_offset = accumulated_offset + node->get_layout_anim_offset();
	if (data.found) {
		const Clay_BoundingBox &bb = data.boundingBox;
		clay_pos = { bb.x, bb.y };
		node->set_layout_home(clay_pos);
		const Vec2 old_size = node->get_layout_rect().size;
		const Rect new_rect = { .position = clay_pos - parent_clay_pos, .size = { bb.width, bb.height } };
		if (new_rect != node->get_layout_rect()) {
			if (new_rect.size != old_size) {
				m_size_changed = true;
			}
			node->set_layout_rect(new_rect);
			node->mark_subtree_bounds_dirty();
		}
		const Vec2 abs_pos = clay_pos + subtree_offset;
		if (abs_pos != node->get_absolute_position()) {
			node->set_absolute_position(abs_pos);
			node->mark_subtree_bounds_dirty();
			node->queue_redraw();
		}
	}

	for (const auto &child : node->get_children()) {
		update_rects(child.get(), clay_pos, subtree_offset);
	}
}

} // namespace Aquila::UI::Core
