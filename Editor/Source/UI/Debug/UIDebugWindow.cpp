#include "UI/Debug/UIDebugWindow.h"

#include "Aquila/Application/Events/Event.h"
#include "Aquila/Application/Events/WindowEvent.h"
#include "Aquila/UI/Core/Canvas.h"
#include "Aquila/UI/Core/View.h"
#include "Aquila/UI/Style/ComputedStyle.h"
#include "Aquila/UI/Style/StyleParser.h"
#include "Aquila/UI/Widgets/Button.h"
#include "Aquila/UI/Widgets/Label.h"
#include "Aquila/UI/Widgets/ScrollView.h"
#include "Aquila/UI/Widgets/TreeView.h"

#include <algorithm>
#include <cstdio>

namespace Editor {

using namespace Aquila;
using namespace Aquila::UI::Core;

namespace {

using namespace Aquila::UI;

std::string MakeLabel(View *view) {
	std::string label(view->GetTypeName());
	if (!view->GetId().empty()) {
		label += " #" + view->GetId();
	}
	for (const auto &cls : view->GetClasses()) {
		label += " ." + cls;
	}
	return label;
}

const char *Str(Display d) { return d == Display::Flex ? "flex" : "none"; }
const char *Str(Overflow o) { return o == Overflow::Visible ? "visible" : (o == Overflow::Hidden ? "hidden" : "scroll"); }
const char *Str(Position p) {
	switch (p) {
	case Position::Relative:
		return "relative";
	case Position::Absolute:
		return "absolute";
	default:
		return "static";
	}
}
const char *Str(FlexDirection f) {
	switch (f) {
	case FlexDirection::Column:
		return "column";
	case FlexDirection::RowReverse:
		return "row-reverse";
	case FlexDirection::ColumnReverse:
		return "column-reverse";
	default:
		return "row";
	}
}
const char *Str(JustifyContent j) {
	switch (j) {
	case JustifyContent::End:
		return "end";
	case JustifyContent::Center:
		return "center";
	default:
		return "start";
	}
}
const char *Str(AlignItems a) {
	switch (a) {
	case AlignItems::End:
		return "end";
	case AlignItems::Center:
		return "center";
	case AlignItems::Stretch:
		return "stretch";
	default:
		return "start";
	}
}
const char *Str(TextAlign t) { return t == TextAlign::Center ? "center" : (t == TextAlign::Right ? "right" : "left"); }

std::string Num(f32 v) {
	char buf[32];
	std::snprintf(buf, sizeof(buf), "%g", v);
	return buf;
}

std::string LenStr(StyleLength l) {
	switch (l.unit) {
	case LengthUnit::Pixel:
		return Num(l.value) + "px";
	case LengthUnit::Percent:
		return Num(l.value) + "%";
	case LengthUnit::Grow:
		return "grow";
	default:
		return "auto";
	}
}

std::string EdgesStr(const StyleEdges &e) {
	return LenStr(e.top) + " " + LenStr(e.right) + " " + LenStr(e.bottom) + " " + LenStr(e.left);
}

std::string ColorStr(vec4 c) {
	auto ch = [](float v) { return static_cast<int>(std::clamp(v, 0.f, 1.f) * 255.f + 0.5f); };
	char buf[40];
	std::snprintf(buf, sizeof(buf), "#%02X%02X%02X%02X", ch(c.r), ch(c.g), ch(c.b), ch(c.a));
	return buf;
}

} // namespace

UIDebugWindow::UIDebugWindow() = default;
UIDebugWindow::~UIDebugWindow() = default;

void UIDebugWindow::Build(Canvas *target, uint32 width, uint32 height, const std::string &stylePath) {
	m_Target = target;
	m_Canvas = CreateUnique<Canvas>(width, height);
	UI::StyleParser::LoadFile(stylePath, m_Canvas->GetStyleSheet());

	auto *root = m_Canvas->GetRoot();
	root->SetId("ui-debug-root");
	root->AddClass("ui-debug-root");

	auto *header = root->AddChild<View>();
	header->AddClass("ui-debug-header");
	auto *title = header->AddChild<Label>(std::string("UI Inspector"));
	title->AddClass("ui-debug-title");
	auto *refresh = header->AddChild<Button>(std::string("Refresh"));
	refresh->AddClass("ui-debug-refresh");
	refresh->onClick.Connect([this] { Refresh(); });

	auto *pick = header->AddChild<Button>(std::string("Pick"));
	pick->AddClass("ui-debug-refresh");
	pick->onClick.Connect([this] {
		if (onPickRequested) {
			onPickRequested();
		}
	});

	m_TreeHost = root->AddChild<View>();
	m_TreeHost->AddClass("ui-debug-tree-pane");

	auto *detailScroll = root->AddChild<ScrollView>();
	detailScroll->AddClass("ui-debug-detail-pane");
	m_DetailsHost = detailScroll->AddContent<View>();
	m_DetailsHost->AddClass("ui-debug-details");

	m_Canvas->ReloadStyles();
	Refresh();
}

void UIDebugWindow::Update(f32 deltaTime) {
	m_Canvas->Update(deltaTime);
	m_Canvas->Compute();
}

void UIDebugWindow::Render(Graphics::QuadBatcher &batcher, GFX::GfxCommandList &cmd) {
	m_Canvas->SubmitToQuadBatcher(batcher, cmd);
}

void UIDebugWindow::OnEvent(Application::Events::Event &event) {
	Application::Events::EventDispatcher dispatcher(event);
	dispatcher.Dispatch<Application::Events::WindowResizeEvent>([this](Application::Events::WindowResizeEvent &e) {
		if (e.GetWidth() > 0 && e.GetHeight() > 0) {
			m_Canvas->Resize(e.GetWidth(), e.GetHeight());
		}
		return false;
	});

	m_Canvas->OnEvent(event);
}

TreeNode *UIDebugWindow::AddViewNode(View *view, TreeNode *parentNode) {
	TreeNode *node = parentNode ? parentNode->AddChildNode(MakeLabel(view)) : m_Tree->AddNode(MakeLabel(view));
	m_NodeToView[node] = view;

	for (const auto &child : view->GetChildren()) {
		AddViewNode(child.get(), node);
	}
	return node;
}

void UIDebugWindow::Refresh() {
	if (!m_Target || !m_TreeHost) {
		return;
	}

	m_NodeToView.clear();
	while (!m_TreeHost->GetChildren().empty()) {
		m_TreeHost->RemoveChild(m_TreeHost->GetChildren().front().get());
	}

	m_Tree = m_TreeHost->AddChild<TreeView>();
	m_Tree->onSelected.Connect([this](TreeNode *node) {
		auto it = m_NodeToView.find(node);
		ShowDetails(it != m_NodeToView.end() ? it->second : nullptr);
	});

	for (const auto &child : m_Target->GetRoot()->GetChildren()) {
		AddViewNode(child.get(), nullptr);
	}
}

void UIDebugWindow::SelectView(View *view) {
	if (!view || !m_Tree) {
		ShowDetails(view);
		return;
	}

	TreeNode *node = nullptr;
	for (auto &[n, v] : m_NodeToView) {
		if (v == view) {
			node = n;
			break;
		}
	}
	if (!node) {
		ShowDetails(view);
		return;
	}

	for (View *parent = view->GetParent(); parent; parent = parent->GetParent()) {
		for (auto &[n, v] : m_NodeToView) {
			if (v == parent) {
				n->SetExpanded(true);
				break;
			}
		}
	}
	m_Tree->SelectNode(node);
	ShowDetails(view);
	m_Canvas->ScrollIntoView(node);
}

void UIDebugWindow::ShowDetails(View *view) {
	if (!m_DetailsHost) {
		return;
	}

	while (!m_DetailsHost->GetChildren().empty()) {
		m_DetailsHost->RemoveChild(m_DetailsHost->GetChildren().front().get());
	}

	if (!view) {
		auto *empty = m_DetailsHost->AddChild<Label>(std::string("Select a node to inspect"));
		empty->AddClass("ui-debug-key");
		return;
	}

	auto section = [&](const std::string &name) {
		auto *s = m_DetailsHost->AddChild<Label>(name);
		s->AddClass("ui-debug-section");
	};
	auto row = [&](const std::string &key, const std::string &value) {
		auto *r = m_DetailsHost->AddChild<View>();
		r->AddClass("ui-debug-row");
		auto *k = r->AddChild<Label>(key);
		k->AddClass("ui-debug-key");
		auto *v = r->AddChild<Label>(value);
		v->AddClass("ui-debug-val");
	};

	section("Identity");
	row("type", std::string(view->GetTypeName()));
	row("id", view->GetId().empty() ? "(none)" : view->GetId());
	std::string classes;
	for (const auto &c : view->GetClasses()) {
		classes += (classes.empty() ? "" : " ") + c;
	}
	row("classes", classes.empty() ? "(none)" : classes);
	row("stableId", std::to_string(view->GetStableId()));
	row("clayId", std::to_string(view->GetClayId()));
	row("children", std::to_string(view->GetChildren().size()));
	if (View *parent = view->GetParent()) {
		row("parent", std::string(parent->GetTypeName()));
	}

	const Rect rect = view->GetAbsoluteRect();
	section("Geometry");
	row("position", Num(rect.position.x) + ", " + Num(rect.position.y));
	row("size", Num(rect.size.x) + " x " + Num(rect.size.y));
	row("effectiveZ", std::to_string(view->GetEffectiveZ()));

	section("State");
	row("visible", view->IsVisible() ? "true" : "false");
	row("enabled", view->IsEnabled() ? "true" : "false");
	row("hovered", view->IsHovered() ? "true" : "false");
	row("pressed", view->IsPressed() ? "true" : "false");
	row("focused", view->IsFocused() ? "true" : "false");

	const UI::ComputedStyle &cs = view->GetComputedStyle();
	section("Layout");
	row("display", Str(cs.display));
	row("position", Str(cs.position));
	row("overflow", Str(cs.overflow));
	row("flex-direction", Str(cs.flexDirection));
	row("justify", Str(cs.justify));
	row("align", Str(cs.align));
	row("flex-grow", Num(cs.flexGrow));
	row("width", LenStr(cs.width));
	row("height", LenStr(cs.height));
	row("min w/h", LenStr(cs.minWidth) + " / " + LenStr(cs.minHeight));
	row("max w/h", LenStr(cs.maxWidth) + " / " + LenStr(cs.maxHeight));
	row("padding", EdgesStr(cs.padding));
	row("gap", Num(cs.gap));
	if (cs.aspectRatio > 0.f) {
		row("aspect-ratio", Num(cs.aspectRatio));
	}
	row("inset", LenStr(cs.top) + " " + LenStr(cs.right) + " " + LenStr(cs.bottom) + " " + LenStr(cs.left));
	row("z-index", std::to_string(cs.zIndex));

	section("Appearance");
	row("background", ColorStr(cs.backgroundColor));
	row("color", ColorStr(cs.color));
	row("border", Num(cs.borderWidth) + "px " + ColorStr(cs.borderColor));
	row("border-radius", LenStr(StyleLength::Pixel(cs.borderRadius.x)) + " (x)");
	row("opacity", Num(cs.opacity));
	row("box-shadows", std::to_string(cs.boxShadows.size()));

	section("Text");
	row("font-size", Num(cs.fontSize));
	row("font-family", cs.fontFamily.empty() ? "(inherit)" : cs.fontFamily);
	row("text-align", Str(cs.textAlign));
}

} // namespace Editor
