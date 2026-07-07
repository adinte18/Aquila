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

void FloatingPanelWindow::build(Unique<View> panel_subtree, const std::string &title, Uint32 width, Uint32 height,
								const std::string &style_path) {
	m_title = title;
	m_canvas = std::make_unique<Canvas>(width, height);
	UI::StyleParser::load_file(style_path, m_canvas->get_style_sheet());

	auto *root = m_canvas->get_root();
	root->add_class("floating-panel-root");

	m_body = root->add_child<View>();
	m_body->add_class("floating-panel-body");
	m_dock_space = m_body->add_child<UI::Core::DockSpace>();
	m_dock_space->get_root_node()->accept_panel(std::move(panel_subtree), title);

	m_canvas->reload_styles();
}

void FloatingPanelWindow::update(F32 delta_time) {
	m_canvas->update(delta_time);
	m_canvas->compute();
}

void FloatingPanelWindow::render(Graphics::QuadBatcher &batcher, GFX::GfxCommandList &cmd) {
	m_canvas->submit_to_quad_batcher(batcher, cmd);
}

void FloatingPanelWindow::on_event(Events::Event &event) {
	Events::EventDispatcher dispatcher(event);
	dispatcher.dispatch<Events::WindowResizeEvent>([&](Events::WindowResizeEvent &e) {
		if (e.get_width() > 0 && e.get_height() > 0) {
			m_canvas->resize(e.get_width(), e.get_height());
		}
		return false;
	});

	m_canvas->on_event(event);
}

bool FloatingPanelWindow::has_content() const {
	return m_dock_space && m_dock_space->has_any_panels();
}

Unique<View> FloatingPanelWindow::detach_content() {
	if (!m_dock_space) {
		return nullptr;
	}

	DockNode *leaf = m_dock_space->first_leaf_with_tabs();
	if (!leaf) {
		return nullptr;
	}

	return leaf->detach_panel(leaf->get_active_panel_ptr());
}

} // namespace Editor
