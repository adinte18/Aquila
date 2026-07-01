#include "UI/Debug/FloatingPanelWindow.h"

#include "Aquila/Application/Events/Event.h"
#include "Aquila/Application/Events/InputEvent.h"
#include "Aquila/Application/Events/WindowEvent.h"
#include "Aquila/UI/Core/Canvas.h"
#include "Aquila/UI/Core/View.h"
#include "Aquila/UI/Style/StyleParser.h"
#include "Aquila/UI/Widgets/DockNode.h"
#include "Aquila/UI/Widgets/Label.h"
#include "Aquila/UI/Widgets/DockSpace.h"

namespace Editor {

using namespace Aquila;
using namespace Aquila::UI::Core;
namespace Events = Aquila::Application::Events;

FloatingPanelWindow::FloatingPanelWindow() = default;
FloatingPanelWindow::~FloatingPanelWindow() = default;

void FloatingPanelWindow::Build(Unique<View> panelSubtree, const std::string &title, uint32 width, uint32 height,
								const std::string &stylePath) {
	m_Title = title;
	m_Canvas = CreateUnique<Canvas>(width, height);
	UI::StyleParser::LoadFile(stylePath, m_Canvas->GetStyleSheet());

	auto *root = m_Canvas->GetRoot();
	root->AddClass("floating-panel-root");

	m_Body = root->AddChild<View>();
	m_Body->AddClass("floating-panel-body");
	m_DockSpace = m_Body->AddChild<UI::Core::DockSpace>();
	m_DockSpace->GetRootNode()->AcceptPanel(std::move(panelSubtree), title);

	m_Canvas->ReloadStyles();
}

void FloatingPanelWindow::Update(f32 deltaTime) {
	m_Canvas->Update(deltaTime);
	m_Canvas->Compute();
}

void FloatingPanelWindow::Render(Graphics::QuadBatcher &batcher, GFX::GfxCommandList &cmd) {
	m_Canvas->SubmitToQuadBatcher(batcher, cmd);
}

void FloatingPanelWindow::OnEvent(Events::Event &event) {
	Events::EventDispatcher dispatcher(event);
	dispatcher.Dispatch<Events::WindowResizeEvent>([&](Events::WindowResizeEvent &e) {
		if (e.GetWidth() > 0 && e.GetHeight() > 0) {
			m_Canvas->Resize(e.GetWidth(), e.GetHeight());
		}
		return false;
	});

	m_Canvas->OnEvent(event);
}

bool FloatingPanelWindow::HasContent() const {
	return m_DockSpace && m_DockSpace->HasAnyPanels();
}

Unique<View> FloatingPanelWindow::DetachContent() {
	if (!m_DockSpace) {
		return nullptr;
	}

	DockNode *leaf = m_DockSpace->FirstLeafWithTabs();
	if (!leaf) {
		return nullptr;
	}

	return leaf->DetachPanel(leaf->GetActivePanelPtr());
}

} // namespace Editor
