#include "Aquila/UI/Core/LayoutEngine.h"
#include "Aquila/UI/Text/FontAtlas.h"

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

static Clay_TextAlignment to_clay_text_align(TextAlign align) {
	switch (align) {
	case TextAlign::Center:
		return CLAY_TEXT_ALIGN_CENTER;
	case TextAlign::Right:
		return CLAY_TEXT_ALIGN_RIGHT;
	default:
		return CLAY_TEXT_ALIGN_LEFT;
	}
}

static Clay_Dimensions measure_clay_text(Clay_StringSlice text, Clay_TextElementConfig *config, void * /*userData*/) {
	auto *font = static_cast<Text::FontAtlas *>(config->userData);
	if (font == nullptr || text.length <= 0) {
		return { 0.F, 0.F };
	}
	const std::string_view slice(text.chars, static_cast<size_t>(text.length));
	const Vec2 dims = font->measure_text(slice, static_cast<F32>(config->fontSize));
	return { dims.x, dims.y };
}

static F32 to_absolute_px(const StyleLength &len, F32 vw_px, F32 vh_px) {
	switch (len.unit) {
	case LengthUnit::Pixel:
		return len.value;
	case LengthUnit::Vw:
		return len.value * vw_px;
	case LengthUnit::Vh:
		return len.value * vh_px;
	default:
		return 0.F;
	}
}

static Clay_SizingAxis to_c_sizing(const StyleLength &len, F32 vw_px, F32 vh_px, F32 flex_grow = 0.F) {
	switch (len.unit) {
	case LengthUnit::Pixel:
		return CLAY_SIZING_FIXED(len.value);
	case LengthUnit::Vw:
		return CLAY_SIZING_FIXED(len.value * vw_px);
	case LengthUnit::Vh:
		return CLAY_SIZING_FIXED(len.value * vh_px);
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

static Clay_SizingAxis to_c_sizing_axis(const StyleLength &len, const StyleLength &min_len, const StyleLength &max_len,
										F32 vw_px, F32 vh_px, F32 flex_grow) {
	if (len.unit == LengthUnit::Vw || len.unit == LengthUnit::Vh) {
		F32 value = to_absolute_px(len, vw_px, vh_px);
		const F32 min_px = to_absolute_px(min_len, vw_px, vh_px);
		const F32 max_px = to_absolute_px(max_len, vw_px, vh_px);
		if (max_px > 0.F && value > max_px) {
			value = max_px;
		}
		if (min_px > 0.F && value < min_px) {
			value = min_px;
		}
		return CLAY_SIZING_FIXED(value);
	}

	Clay_SizingAxis axis = to_c_sizing(len, vw_px, vh_px, flex_grow);
	if (len.unit == LengthUnit::Auto || len.unit == LengthUnit::Grow) {
		const F32 min_px = to_absolute_px(min_len, vw_px, vh_px);
		const F32 max_px = to_absolute_px(max_len, vw_px, vh_px);
		if (min_px > 0.F) {
			axis.size.minMax.min = min_px;
		}
		if (max_px > 0.F) {
			axis.size.minMax.max = max_px;
		}
	}
	return axis;
}

static Clay_LayoutConfig to_clay_layout(const ComputedStyle &cs, F32 vw_px, F32 vh_px) {
	const bool is_row = (cs.flex_direction == FlexDirection::Row || cs.flex_direction == FlexDirection::RowReverse);
	const Clay_LayoutAlignmentX align_x = is_row ? justify_to_align_x(cs.justify) : align_to_align_x(cs.align);
	const Clay_LayoutAlignmentY align_y = is_row ? align_to_align_y(cs.align) : justify_to_align_y(cs.justify);

	Clay_LayoutConfig layout = {
		.sizing = { .width = to_c_sizing_axis(cs.width, cs.min_width, cs.max_width, vw_px, vh_px, cs.flex_grow),
					.height = to_c_sizing_axis(cs.height, cs.min_height, cs.max_height, vw_px, vh_px, cs.flex_grow) },
		.padding = to_clay_padding(cs.padding),
		.childGap = static_cast<uint16_t>(cs.gap),
		.childAlignment = { .x = align_x, .y = align_y },
		.layoutDirection = is_row ? CLAY_LEFT_TO_RIGHT : CLAY_TOP_TO_BOTTOM,
	};

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
	Clay_Context *ctx = Clay_Initialize(arena, { static_cast<F32>(width), static_cast<F32>(height) }, {});
	m_clay_ctx = ctx;
	Clay_SetCurrentContext(ctx);
	Clay_SetMeasureTextFunction(measure_clay_text, nullptr);
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
	m_resized_nodes.clear();
	update_rects(root);
}

void LayoutEngine::scroll_into_view(View *target) {
	if (target == nullptr) {
		return;
	}

	View *container = nullptr;
	for (View *v = target->get_parent(); v != nullptr; v = v->get_parent()) {
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

void LayoutEngine::set_scroll_offset(View *target, float offset_y) {
	if (target == nullptr) {
		return;
	}

	Clay_SetCurrentContext(static_cast<Clay_Context *>(m_clay_ctx));
	Clay_ElementId id = {};
	id.id = target->get_clay_id();
	Clay_ScrollContainerData data = Clay_GetScrollContainerData(id);
	if (!data.found || data.scrollPosition == nullptr) {
		return;
	}

	const float view_h = target->get_absolute_rect().size.y;
	const float max_scroll = data.contentDimensions.height - view_h;
	float clamped = offset_y;
	if (max_scroll <= 0.F || clamped < 0.F) {
		clamped = 0.F;
	} else if (clamped > max_scroll) {
		clamped = max_scroll;
	}
	data.scrollPosition->y = -clamped;
}

void LayoutEngine::layout_pass(View *node) {
	const ComputedStyle &cs = node->get_display_style();

	if (cs.display == Display::None) {
		return;
	}

	const Clay_ElementId clay_id = make_element_id(node);
	node->set_clay_id(clay_id.id);

	ClayTextRun text_run;
	const bool is_text = node->get_clay_text_run(text_run);

	auto emit_content = [&]() {
		if (is_text) {
			text_run.font->ensure_glyphs(text_run.text);
			const Clay_String text_string{
				.isStaticallyAllocated = false,
				.length = static_cast<Int32>(text_run.text.size()),
				.chars = text_run.text.data(),
			};
			Clay_TextElementConfig text_config{};
			text_config.userData = text_run.font;
			text_config.fontSize = static_cast<uint16_t>(text_run.font_size);
			text_config.wrapMode = CLAY_TEXT_WRAP_WORDS;
			text_config.textAlignment = to_clay_text_align(text_run.align);
			Clay__OpenTextElement(text_string, text_config);
			return;
		}
		for (const auto &child : node->get_children()) {
			layout_pass(child.get());
		}
	};

	const F32 vw_px = static_cast<F32>(m_width) / 100.F;
	const F32 vh_px = static_cast<F32>(m_height) / 100.F;
	Clay_LayoutConfig layout = to_clay_layout(cs, vw_px, vh_px);
	if (!is_text) {
		const Vec2 intrinsic = node->get_intrinsic_size();
		if (intrinsic.x >= 0.F && cs.width.unit == LengthUnit::Auto) {
			layout.sizing.width = CLAY_SIZING_FIXED(intrinsic.x);
		}
		if (intrinsic.y >= 0.F && cs.height.unit == LengthUnit::Auto) {
			layout.sizing.height = CLAY_SIZING_FIXED(intrinsic.y);
		}
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
				emit_content();
			}
			break;
		case Overflow::Hidden:
			CLAY(clay_id,
				 { .layout = layout,
				   .aspectRatio = aspect_cfg,
				   .floating = float_cfg,
				   .clip = { .horizontal = true, .vertical = false } }) {
				emit_content();
			}
			break;
		default:
			CLAY(clay_id, { .layout = layout, .aspectRatio = aspect_cfg, .floating = float_cfg }) {
				emit_content();
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
			emit_content();
		}
		break;

	case Overflow::Hidden:
		CLAY(clay_id,
			 {
				 .layout = layout,
				 .aspectRatio = aspect_cfg,
				 .clip = { .horizontal = true, .vertical = false },
			 }) {
			emit_content();
		}
		break;

	default:
		CLAY(clay_id, { .layout = layout, .aspectRatio = aspect_cfg }) {
			emit_content();
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
				m_resized_nodes.push_back(node);
				node->queue_redraw();
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
