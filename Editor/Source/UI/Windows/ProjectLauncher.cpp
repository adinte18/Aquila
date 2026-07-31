#include "UI/Windows/ProjectLauncher.h"

#include "Aquila/Application/Events/Event.h"
#include "Aquila/Application/Events/WindowEvent.h"
#include "Aquila/UI/Core/Canvas.h"
#include "Aquila/UI/Core/View.h"
#include "Aquila/UI/Style/StyleParser.h"
#include "Aquila/UI/Widgets/Button.h"
#include "Aquila/UI/Widgets/Label.h"
#include "Aquila/UI/Widgets/ScrollView.h"
#include "Aquila/UI/Widgets/TextInput.h"

namespace Editor {

using namespace Aquila;
using namespace Aquila::UI::Core;

ProjectLauncher::ProjectLauncher() = default;
ProjectLauncher::~ProjectLauncher() = default;

void ProjectLauncher::build(ProjectManager *project_manager, Uint32 width, Uint32 height,
							const std::string &style_path) {
	m_project_manager = project_manager;

	m_canvas = std::make_unique<Canvas>(width, height);
	UI::StyleParser::load_file(style_path, m_canvas->get_style_sheet());

	auto *root = m_canvas->get_root();
	root->set_id("launcher-root");
	root->add_class("launcher-root");

	auto *header = root->add_child<View>();
	header->add_class("launcher-header");
	auto *title = header->add_child<Label>(std::string("Aquila Projects"));
	title->add_class("launcher-title");

	auto *body = root->add_child<View>();
	body->add_class("launcher-body");

	auto *open_label = body->add_child<Label>(std::string("Open a project"));
	open_label->add_class("launcher-section");

	auto *scroll = body->add_child<ScrollView>();
	scroll->add_class("launcher-list");
	m_list_host = scroll->add_child<View>();
	m_list_host->add_class("launcher-list-inner");

	auto *new_label = body->add_child<Label>(std::string("New project"));
	new_label->add_class("launcher-section");

	auto *new_row = body->add_child<View>();
	new_row->add_class("launcher-new-row");
	m_name_input = new_row->add_child<TextInput>(std::string("Project name…"));
	m_name_input->add_class("launcher-name-input");
	m_name_input->on_submit.connect([this](const std::string &) { create_project(); });

	auto *create_btn = new_row->add_child<Button>(std::string("Create"));
	create_btn->add_class("launcher-create-btn");
	create_btn->on_click.connect([this] { create_project(); });

	refresh_list();
	m_canvas->reload_styles();
}

void ProjectLauncher::refresh_list() {
	if (m_list_host == nullptr || m_project_manager == nullptr) {
		return;
	}

	while (!m_list_host->get_children().empty()) {
		m_list_host->remove_child(m_list_host->get_children().front().get());
	}

	for (const ProjectInfo &project : m_project_manager->list()) {
		auto *entry = m_list_host->add_child<Button>(project.name);
		entry->add_class("launcher-project");
		const ProjectInfo info = project;
		entry->on_click.connect([this, info] { open_project(info); });
	}
}

void ProjectLauncher::open_project(const ProjectInfo &project) {
	if (on_project_ready) {
		on_project_ready(project);
	}
}

void ProjectLauncher::create_project() {
	if (m_name_input == nullptr || m_project_manager == nullptr) {
		return;
	}

	const std::string name = m_name_input->get_text();
	if (name.empty()) {
		return;
	}

	if (Option<ProjectInfo> created = m_project_manager->create(name)) {
		m_name_input->set_text("");
		refresh_list();
		open_project(*created);
	}
}

void ProjectLauncher::update(F32 delta_time) {
	m_canvas->update(delta_time);
	m_canvas->compute();
}

void ProjectLauncher::render(Graphics::QuadBatcher &batcher, GFX::GfxCommandList &cmd) {
	m_canvas->submit_to_quad_batcher(batcher, cmd);
}

void ProjectLauncher::on_event(Application::Events::Event &event) {
	Application::Events::EventDispatcher dispatcher(event);
	dispatcher.dispatch<Application::Events::WindowResizeEvent>([this](Application::Events::WindowResizeEvent &e) {
		if (e.get_width() > 0 && e.get_height() > 0) {
			m_canvas->resize(e.get_width(), e.get_height());
		}
		return false;
	});

	m_canvas->on_event(event);
}

} // namespace Editor
