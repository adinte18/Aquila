#include "UI/EditorWindows/ConsoleWindow.h"

#include "UI/Panels/ConsolePanel.h"

namespace Editor {

ConsoleWindow::ConsoleWindow(EditorContext &context) : EditorWindow(context) {}

ConsoleWindow::~ConsoleWindow() = default;

void ConsoleWindow::build(Aquila::UI::Core::View &content) {
	if (auto layout = load_layout("console.aqlayout")) {
		content.add_child(std::move(layout));
	}

	m_panel = std::make_unique<ConsolePanel>(&context().textures());
	m_panel->build(&content, overlay_root());
}

void ConsoleWindow::update(F32) {
	m_panel->flush_pending();
}

void ConsoleWindow::clear() {
	m_panel->clear_all();
}

}
