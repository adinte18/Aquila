#include "Aquila/UI/Core/Canvas.h"
#include "Aquila/Application/Events/InputEvent.h"
#include "Aquila/Graphics/Core/QuadBatcher.h"
#include "Aquila/Platform/Input.h"

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wmissing-designated-field-initializers"
#pragma clang diagnostic ignored "-Wmissing-field-initializers"
#pragma clang diagnostic ignored "-Wgnu-anonymous-struct"
#pragma clang diagnostic ignored "-Wnested-anon-types"
#include "clay.h"
#pragma clang diagnostic pop

namespace Aquila::UI::Core {

using namespace Application::Events;

static Clay_LayoutAlignmentX JustifyToAlignX(JustifyContent j) {
	switch (j) {
	case JustifyContent::Center:
		return CLAY_ALIGN_X_CENTER;
	case JustifyContent::End:
		return CLAY_ALIGN_X_RIGHT;
	default:
		return CLAY_ALIGN_X_LEFT;
	}
}

static Clay_LayoutAlignmentY JustifyToAlignY(JustifyContent j) {
	switch (j) {
	case JustifyContent::Center:
		return CLAY_ALIGN_Y_CENTER;
	case JustifyContent::End:
		return CLAY_ALIGN_Y_BOTTOM;
	default:
		return CLAY_ALIGN_Y_TOP;
	}
}

static Clay_LayoutAlignmentX AlignToAlignX(AlignItems a) {
	switch (a) {
	case AlignItems::Center:
		return CLAY_ALIGN_X_CENTER;
	case AlignItems::End:
		return CLAY_ALIGN_X_RIGHT;
	default:
		return CLAY_ALIGN_X_LEFT;
	}
}

static Clay_LayoutAlignmentY AlignToAlignY(AlignItems a) {
	switch (a) {
	case AlignItems::Center:
		return CLAY_ALIGN_Y_CENTER;
	case AlignItems::End:
		return CLAY_ALIGN_Y_BOTTOM;
	default:
		return CLAY_ALIGN_Y_TOP;
	}
}

static Clay_SizingAxis ToCSizing(const StyleLength &len, float flexGrow = 0.f) {
	switch (len.type) {
	case Type::Pixel:
		return CLAY_SIZING_FIXED(len.value);
	case Type::Percent:
		return CLAY_SIZING_PERCENT(len.value / 100.f);
	case Type::Grow:
		return CLAY_SIZING_GROW(flexGrow);
	case Type::Auto:
	default:
		return CLAY_SIZING_FIT(0, 0);
	}
}

static Clay_Padding ToClayPadding(const StyleEdges &edges) {
	auto px = [](const StyleLength &l) -> uint16_t {
		return l.type == Type::Pixel ? static_cast<uint16_t>(l.value) : 0u;
	};
	return { .left = px(edges.left), .right = px(edges.right), .top = px(edges.top), .bottom = px(edges.bottom) };
}

static Clay_LayoutConfig ToClayLayout(const ComputedStyle &cs) {
	const bool isRow = (cs.flexDirection == FlexDirection::Row || cs.flexDirection == FlexDirection::RowReverse);
	const Clay_LayoutAlignmentX ax = isRow ? JustifyToAlignX(cs.justify) : AlignToAlignX(cs.align);
	const Clay_LayoutAlignmentY ay = isRow ? AlignToAlignY(cs.align) : JustifyToAlignY(cs.justify);

	return {
		.sizing = { .width = ToCSizing(cs.width, cs.flexGrow), .height = ToCSizing(cs.height, cs.flexGrow) },
		.padding = ToClayPadding(cs.padding),
		.childAlignment = { .x = ax, .y = ay },
		.layoutDirection = isRow ? CLAY_LEFT_TO_RIGHT : CLAY_TOP_TO_BOTTOM,
	};
}

Canvas::Canvas(uint32 width, uint32 height) : m_Width(width), m_Height(height) {
	m_Root = CreateUnique<View>();
	m_DrawList.SetCanvasSize(width, height);

	const uint32_t memSize = Clay_MinMemorySize();
	m_ClayMemory.resize(memSize);
	const Clay_Arena arena = Clay_CreateArenaWithCapacityAndMemory(memSize, m_ClayMemory.data());
	m_ClayCtx = Clay_Initialize(arena, { static_cast<float>(width), static_cast<float>(height) }, {});

	StyleProperties rootStyle;
	rootStyle.width = StyleLength::Grow();
	rootStyle.height = StyleLength::Grow();
	m_Root->SetStyle(rootStyle);
}

void Canvas::StylePass(View *node, const ComputedStyle *parentComputed) {
	ComputedStyle computed = m_StyleSheet.Resolve(*node, parentComputed);
	node->SetComputedStyle(computed);

	for (const auto &child : node->GetChildren()) {
		StylePass(child.get(), &node->GetComputedStyle());
	}
}

void Canvas::ClayLayoutPass(View *node) {
	const uint32_t raw = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(node) >> 4);
	const uint32_t id = raw != 0 ? raw : 1u;
	node->SetClayId(id);

	const Clay_ElementId clayId = { .id = id };
	CLAY(clayId, { .layout = ToClayLayout(node->GetComputedStyle()) }) {
		for (const auto &child : node->GetChildren()) {
			ClayLayoutPass(child.get());
		}
	}
}

void Canvas::ClayUpdateRects(View *node, vec2 parentAbsPos) {
	const Clay_ElementId clayId = { .id = node->GetClayId() };
	const Clay_ElementData data = Clay_GetElementData(clayId);
	vec2 myAbsPos = parentAbsPos;
	if (data.found) {
		const Clay_BoundingBox &bb = data.boundingBox;
		myAbsPos = { bb.x, bb.y };
		// Store position relative to parent so OnDraw accumulation is correct
		node->SetLayoutRect({ .position = myAbsPos - parentAbsPos, .size = { bb.width, bb.height } });
	}

	for (const auto &child : node->GetChildren()) {
		ClayUpdateRects(child.get(), myAbsPos);
	}
}

void Canvas::DrawPass(View *node, vec2 origin) {
	node->OnDraw(m_DrawList, origin);
}

void Canvas::Render(Graphics::QuadBatcher &r2d, GFX::GfxCommandList &cmd) {
	if (!m_Root) {
		return;
	}

	m_DrawList.Clear();
	StylePass(m_Root.get(), nullptr);

	Clay_SetCurrentContext(static_cast<Clay_Context *>(m_ClayCtx));
	Clay_SetLayoutDimensions({ static_cast<float>(m_Width), static_cast<float>(m_Height) });
	Clay_BeginLayout();
	ClayLayoutPass(m_Root.get());
	Clay_EndLayout(0.f);
	ClayUpdateRects(m_Root.get());

	DrawPass(m_Root.get(), vec2(0.f));
	m_DrawList.Sort();
	m_DrawList.Submit(r2d, cmd);
}

void Canvas::OnEvent(Application::Events::Event &e) {
	EventDispatcher dispatcher(e);

	dispatcher.Dispatch<MouseMovedEvent>([this](MouseMovedEvent &e) {
		View *hit = m_Root ? m_Root->HitTest({ e.GetX(), e.GetY() }) : nullptr;
		if (hit != m_HoveredView) {
			if (m_HoveredView) {
				m_HoveredView->OnMouseLeave();
			}
			m_HoveredView = hit;
			if (m_HoveredView) {
				m_HoveredView->OnMouseEnter();
			}
		}
		return false;
	});

	dispatcher.Dispatch<MouseButtonPressedEvent>([this](MouseButtonPressedEvent &e) {
		if (!m_HoveredView) {
			return false;
		}
		if (m_FocusedView && m_FocusedView != m_HoveredView) {
			m_FocusedView->OnFocusLost();
		}
		m_FocusedView = m_HoveredView;
		m_FocusedView->OnFocusGained();
		auto [x, y] = Platform::Input::GetMousePosition();
		m_FocusedView->OnMousePress(e.GetMouseButton(), { x, y });
		return false;
	});

	dispatcher.Dispatch<MouseButtonReleasedEvent>([this](MouseButtonReleasedEvent &e) {
		if (m_HoveredView) {
			auto [x, y] = Platform::Input::GetMousePosition();
			m_HoveredView->OnMouseRelease(e.GetMouseButton(), { x, y });
		}
		return false;
	});

	dispatcher.Dispatch<KeyPressedEvent>([this](KeyPressedEvent &e) {
		if (m_FocusedView) {
			m_FocusedView->OnKeyPress(e.GetKeyCode());
		}
		return false;
	});

	dispatcher.Dispatch<KeyReleasedEvent>([this](KeyReleasedEvent &e) {
		if (m_FocusedView) {
			m_FocusedView->OnKeyRelease(e.GetKeyCode());
		}
		return false;
	});
}

void Canvas::Update(float deltaTime) {
	AQUILA_UNUSED(deltaTime);
}

void Canvas::Resize(uint32 width, uint32 height) {
	m_Width = width;
	m_Height = height;
	m_DrawList.SetCanvasSize(width, height);
	Clay_SetCurrentContext(static_cast<Clay_Context *>(m_ClayCtx));
	Clay_SetLayoutDimensions({ static_cast<float>(width), static_cast<float>(height) });
}

StyleSheet &Canvas::GetStyleSheet() {
	return m_StyleSheet;
}

View *Canvas::GetRoot() {
	return m_Root.get();
}

} // namespace Aquila::UI::Core
