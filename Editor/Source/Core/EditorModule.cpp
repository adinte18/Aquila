#include "Core/EditorModule.h"
#include "Core/ProjectManager.h"

#include "Aquila/Application/ApplicationNew.h"

#include "UI/Panels/ConsolePanel.h"
#include "UI/Panels/HierarchyPanel.h"
#include "Aquila/UI/Core/DockWindowManager.h"
#include "Aquila/UI/Core/FontManager.h"
#include "Aquila/UI/DevTools/UIDevTools.h"
#include "UI/Windows/SettingsWindow.h"
#include "UI/Windows/ProjectLauncher.h"
#include "UI/Panels/InspectorPanel.h"
#include "UI/Panels/ViewportPanel.h"

#include "Aquila/Foundation/Macros.h"
#include "Aquila/Foundation/SharedConstants.h"
#include "Aquila/Graphics/Material/MaterialFactory.h"
#include "Aquila/Graphics/Resources/Mesh.h"
#include "Aquila/Scene/Components/CameraComponent.h"
#include "Aquila/Scene/Components/LightComponent.h"
#include "Aquila/Scene/Components/MaterialComponent.h"
#include "Aquila/Scene/Components/MeshComponent.h"
#include "Aquila/Scene/Components/OutlineComponent.h"
#include "Aquila/Scene/Components/SkyLightComponent.h"
#include "Aquila/Scene/Components/TransformComponent.h"
#include "Aquila/Scene/DefaultScenes.h"
#include "Aquila/Scene/EntityManager.h"
#include "Aquila/UI/Core/FontRegistry.h"
#include "Aquila/UI/Core/LayoutLoader.h"
#include "Aquila/UI/Core/UIHost.h"
#include "Aquila/UI/Style/StyleParser.h"
#include "Aquila/UI/Widgets/Button.h"
#include "Aquila/UI/Widgets/ColorPicker.h"
#include "Aquila/UI/Widgets/PopupMenu.h"
#include "Aquila/UI/Widgets/DockNode.h"
#include "Aquila/UI/Widgets/DockPanel.h"
#include "Aquila/UI/Widgets/DockSpace.h"
#include "Aquila/UI/Widgets/DockTypes.h"
#include "Aquila/UI/Core/DockLayoutSerializer.h"
#include "Aquila/UI/Widgets/Menubar.h"
#include "Aquila/UI/Widgets/TextInput.h"
#include "Aquila/Platform/Events/InputEvent.h"
#include "Aquila/Platform/Input.h"
#include "Aquila/Rendering/Systems/ObjectPickingSystem.h"

#include <algorithm>

namespace Editor {

using namespace Aquila;
using namespace Aquila::SceneManagement;
using namespace Aquila::SceneManagement::Components;
using namespace Aquila::Application;
namespace Events = Aquila::Platform::Events;

namespace {
const std::string k_layout_path = "/app/layout.aqdl";
}

EditorModule::EditorModule() = default;

EditorModule::~EditorModule() = default;

void EditorModule::on_attach(EngineContext &engine) {
	m_engine = &engine;

	Window &main_window = m_engine->get_window();
	m_ui_host = std::make_unique<Aquila::UI::Core::UIHost>(Aquila::UI::Core::UIHostDesc{
		.renderer_2d = m_engine->get_renderer_2d(),
		.width = main_window.get_width(),
		.height = main_window.get_height(),
		.clipboard_get = [&main_window] { return main_window.get_clipboard_text(); },
		.clipboard_set = [&main_window](const std::string &text) { main_window.set_clipboard_text(text); },
	});

	Config::get_preferences().load_from_file();
	Aquila::UI::Core::FontRegistry::set_ui_scale(Config::get_preferences().ui_scale);

	Aquila::UI::Core::FontManager::get().initialize(m_engine->get_context(), Config::get_preferences().fonts);

	m_project_manager = std::make_unique<ProjectManager>();

	open_project_launcher();
}

void EditorModule::open_project_launcher() {
	m_project_launcher = std::make_unique<ProjectLauncher>();
	m_project_launcher->build(m_project_manager.get(), 720, 520, Config::get_preferences().ui.style_path);
	m_project_launcher->on_project_ready = [this](const ProjectInfo &project) { m_pending_project = project; };

	m_launcher_window = Aquila::Rendering::open_content_window(m_engine->get_window_host(), *m_project_launcher, 720, 520, "Aquila - Projects", [this] {
		m_project_launcher.reset();
		m_launcher_window = nullptr;
		if (!m_editor_entered) {
			m_engine->request_close();
		}
	});
}

void EditorModule::enter_editor(const ProjectInfo &project) {
	AQUILA_LOG_INFO("EditorModule: opening project '{}' ({})", project.name, project.directory);

	m_editor_entered = true;
	if (m_launcher_window != nullptr) {
		m_engine->get_window_host().request_close(m_launcher_window);
	}

	m_engine->get_window().show();
	m_editor_camera = std::make_unique<Aquila::Rendering::CameraController>();
	setup_editor_ui();
}

void EditorModule::on_detach() {
	if (m_dock_space != nullptr) {
		const Aquila::UI::Core::DockLayoutDesc layout = Aquila::UI::Core::DockLayoutSerializer::capture(*m_dock_space);
		if (Aquila::UI::Core::DockLayoutSerializer::save_to_file(k_layout_path, layout)) {
			AQUILA_LOG_INFO("Editor dock layout saved to {}", k_layout_path);
		}
	}

	m_dock_manager.reset();
	m_settings_window.reset();
	m_project_launcher.reset();
	m_devtools.reset();
	m_hierarchy_panel.reset();
	m_viewport_panel.reset();
	m_inspector_panel.reset();
	m_console_panel.reset();
	m_texture_cache.reset();

	m_ui_host.reset();
	Aquila::UI::Core::FontManager::get().shutdown();
}

void EditorModule::on_pre_render(F32 delta_time) {
	if (m_pending_project) {
		const ProjectInfo project = *m_pending_project;
		m_pending_project.reset();
		enter_editor(project);
	}

	if (m_console_panel) {
		m_console_panel->flush_pending();
	}
	m_ui_host->update(delta_time);

	if (m_editor_camera && m_viewport_panel) {
		const Rect viewport = m_viewport_panel->get_content_rect();
		const auto view_width = static_cast<Uint32>(viewport.size.x);
		const auto view_height = static_cast<Uint32>(viewport.size.y);
		m_editor_camera->set_viewport_rect(viewport.position, viewport.size);
		m_editor_camera->set_viewport_size(view_width, view_height);
		m_editor_camera->update(delta_time);
		m_engine->get_render_pipeline().set_primary_view(m_editor_camera->get_render_view());
	}

	m_engine->get_window().set_cursor(m_ui_host->get_active_cursor());
}

void EditorModule::on_event(Events::Event &event) {
	Events::EventDispatcher dispatcher(event);

	if (m_editor_camera) {
		m_editor_camera->on_event(event);
	}

	if (m_devtools && m_devtools->on_event(event)) {
		return;
	}

	dispatcher.dispatch<Events::KeyPressedEvent>([this](Events::KeyPressedEvent &e) {
		if (e.get_key_code() == Events::KeyCode::F5) {
			m_layout_loader.load_file(Config::get_preferences().ui.layout_path);
			AQUILA_LOG_INFO("Editor layout reloaded");
			return true;
		}

		if (e.get_key_code() == Events::KeyCode::F6) {
			auto &canvas = m_ui_host->get_canvas(Aquila::UI::Core::UILayer::Editor);
			Aquila::UI::StyleParser::load_file(Config::get_preferences().ui.style_path, canvas.get_style_sheet());
			canvas.reload_styles();

			AQUILA_LOG_INFO("Stylesheet reloaded");
			return true;
		}
		return false;
	});

	m_ui_host->on_event(event);

	Events::EventDispatcher post(event);
	post.dispatch<Events::KeyPressedEvent>([this](Events::KeyPressedEvent &e) {
		if (e.get_key_code() != Events::KeyCode::S || e.get_mods() != Events::MODIFIER_SHIFT) {
			return false;
		}
		auto &canvas = m_ui_host->get_canvas(Aquila::UI::Core::UILayer::Editor);
		if (Aquila::UI::Core::view_is<Aquila::UI::Core::TextInput>(canvas.get_focused_view())) {
			return false;
		}
		if (Aquila::Platform::Input::is_mouse_button_pressed(Events::MouseButton::Right) || Aquila::Platform::Input::is_mouse_button_pressed(Events::MouseButton::Left)) {
			return false;
		}
		if (m_inspector_panel != nullptr) {
			m_inspector_panel->open_add_search(Aquila::Platform::Input::get_mouse_position());
		}
		return true;
	});
}

void EditorModule::set_outlined_entity(Entity entity) {
	if (m_outlined_entity.is_valid() && m_outlined_entity.exists()) {
		m_outlined_entity.try_remove_component<OutlineComponent>();
	}
	m_outlined_entity = entity;
	if (m_outlined_entity.is_valid()) {
		m_outlined_entity.add_component<OutlineComponent>();
	}
}

void EditorModule::request_viewport_pick(Vec2 uv) {
	if (Aquila::Platform::Input::is_key_pressed(Events::KeyCode::LeftAlt)) {
		return;
	}

	auto &output = m_engine->get_render_output();
	const F32 x = uv.x * static_cast<F32>(output.get_width());
	const F32 y = uv.y * static_cast<F32>(output.get_height());
	if (x < 0.F || y < 0.F) {
		return;
	}

	m_engine->get_object_picking().request_pick(static_cast<Uint32>(x), static_cast<Uint32>(y));
}

void EditorModule::on_resize(Uint32 width, Uint32 height) {
	if (m_viewport_panel) {
		m_viewport_panel->set_texture(&m_engine->get_render_output());
	}
}

void EditorModule::on_render_resize(Uint32 width, Uint32 height) {
	if (m_viewport_panel) {
		m_viewport_panel->set_texture(&m_engine->get_render_output());
	}
}

F32 EditorModule::window_aspect() {
	return static_cast<F32>(m_engine->get_window().get_width()) / static_cast<F32>(m_engine->get_window().get_height());
}

void EditorModule::refresh_scene_panels() {
	if (m_hierarchy_panel) {
		m_hierarchy_panel->rebuild();
	}
	if (m_inspector_panel) {
		m_inspector_panel->clear();
	}
}

void EditorModule::new_empty_scene() {
	m_engine->get_scene().clear();
	SceneManagement::spawn_default_camera(m_engine->get_scene(), window_aspect());
	refresh_scene_panels();
	AQUILA_LOG_INFO("EditorModule: new empty scene");
}

void EditorModule::reset_to_demo_scene() {
	m_engine->get_scene().clear();
	SceneManagement::populate_demo_scene(m_engine->get_scene(), m_engine->get_context(), window_aspect());
	refresh_scene_panels();
	AQUILA_LOG_INFO("EditorModule: reset to demo scene");
}

void EditorModule::setup_editor_ui() {
	auto &editor_canvas = m_ui_host->get_canvas(Aquila::UI::Core::UILayer::Editor);
	const auto &cfg = Config::get_preferences();

	m_texture_cache = std::make_unique<Aquila::UI::Core::TextureCache>(m_engine->get_context(), cfg.ui.resources_path);

	Aquila::UI::StyleParser::load_file(cfg.ui.style_path, editor_canvas.get_style_sheet());

	m_layout_loader.register_font("regular", Aquila::UI::Core::FontManager::get().get_font("regular"));
	m_layout_loader.register_texture_cache(m_texture_cache.get());
	m_layout_loader.register_widget(
		"ColorPicker", [this](std::string_view, Aquila::UI::Text::FontAtlas *) -> Unique<Aquila::UI::Core::View> {
			return std::make_unique<Aquila::UI::Core::ColorPicker>(m_engine->get_context());
		});

	m_layout_loader.register_command("entity.create", [this] {
		auto entity = m_engine->get_scene().get_entity_manager()->create_entity("New Entity");
		if (m_hierarchy_panel) {
			m_hierarchy_panel->add_entity(entity);
		}
	});
	m_layout_loader.register_command("console.clear", [this] {
		if (m_console_panel) {
			m_console_panel->clear_all();
		}
	});

	auto root = m_layout_loader.load_file(cfg.ui.layout_path);
	if (!root) {
		AQUILA_LOG_ERROR("EditorModule: failed to load editor layout from {}", cfg.ui.layout_path);
		return;
	}

	Aquila::UI::Core::View *layout_root = editor_canvas.get_root()->add_child(std::move(root));

	m_dock_space = layout_root->find_by_id<Aquila::UI::Core::DockSpace>("editor-dock");
	auto *hierarchy_panel = layout_root->find_by_id<Aquila::UI::Core::DockPanel>("panel-hierarchy");
	auto *viewport_panel = layout_root->find_by_id<Aquila::UI::Core::DockPanel>("panel-viewport");
	auto *inspector_panel = layout_root->find_by_id<Aquila::UI::Core::DockPanel>("panel-inspector");
	auto *console_panel = layout_root->find_by_id<Aquila::UI::Core::DockPanel>("panel-console");
	if ((m_dock_space == nullptr) || (hierarchy_panel == nullptr) || (viewport_panel == nullptr) ||
		(inspector_panel == nullptr) || (console_panel == nullptr)) {
		AQUILA_LOG_ERROR("EditorModule: editor dock layout not found — check editor.aqlayout");
		return;
	}

	m_dock_manager = std::make_unique<Aquila::UI::Core::DockWindowManager>(m_engine->get_window_host(), cfg.ui.style_path);
	m_dock_manager->set_main_dock_space(m_dock_space);

	m_hierarchy_panel = std::make_unique<HierarchyPanel>(*m_engine->get_scene().get_entity_manager());
	m_viewport_panel = std::make_unique<ViewportPanel>(m_engine->get_render_output());
	m_inspector_panel = std::make_unique<InspectorPanel>(m_engine->get_context(), m_texture_cache.get());
	m_console_panel = std::make_unique<ConsolePanel>(m_texture_cache.get());

	m_hierarchy_panel->build(hierarchy_panel, layout_root);
	m_hierarchy_panel->set_tree_icons(m_layout_loader.resolve_texture("Engine/UI/Icons/chevron-right.png"),
									  m_layout_loader.resolve_texture("Engine/UI/Icons/chevron-down.png"));
	m_viewport_panel->build(viewport_panel, layout_root);
	m_inspector_panel->build(inspector_panel, layout_root);
	m_console_panel->build(console_panel, layout_root);

	m_hierarchy_panel->on_entity_selected.connect([this](Entity entity) { m_inspector_panel->show_entity(entity); });
	m_hierarchy_panel->on_entity_deselected.connect([this] { m_inspector_panel->clear(); });
	m_inspector_panel->on_entity_renamed.connect([this](Entity entity) { m_hierarchy_panel->refresh_entity(entity); });

	m_hierarchy_panel->on_entity_selected.connect([this](Entity entity) { set_outlined_entity(entity); });
	m_hierarchy_panel->on_entity_deselected.connect([this] { set_outlined_entity(Entity::null()); });

	m_viewport_panel->on_clicked_uv.connect([this](Vec2 uv) { request_viewport_pick(uv); });
	m_engine->get_object_picking().on_picked.connect([this](Entity entity) {
		if (entity.is_valid()) {
			m_hierarchy_panel->select_entity(entity);
		} else {
			m_hierarchy_panel->deselect_entity();
		}
	});

	wire_menubar(layout_root);

	m_devtools = std::make_unique<Aquila::UI::DevTools::UIDevTools>(Aquila::UI::DevTools::UIDevToolsDesc{
		.target = editor_canvas,
		.host = m_engine->get_window_host(),
		.style_path = cfg.ui.style_path,
	});
	m_devtools->attach(layout_root);

	if (Option<Aquila::UI::Core::DockLayoutDesc> saved =
			Aquila::UI::Core::DockLayoutSerializer::load_from_file(k_layout_path)) {
		m_dock_space->apply_layout(*saved);
		AQUILA_LOG_INFO("Editor dock layout restored from {}", k_layout_path);
	}

	editor_canvas.reload_styles();
}

void EditorModule::open_settings_window() {
	if (m_settings_window) {
		return;
	}

	m_settings_window = std::make_unique<SettingsWindow>();
	m_settings_window->build(m_texture_cache.get(), 560, 640, Config::get_preferences().ui.style_path);
	m_settings_window->on_applied = [this] { apply_font_settings(); };

	const Aquila::Rendering::RenderWindowId window =
		Aquila::Rendering::open_content_window(m_engine->get_window_host(), *m_settings_window, 560, 640, "Aquila - Settings", [this] { m_settings_window.reset(); });
	m_settings_window->on_request_close = [this, window] { m_engine->get_window_host().request_close(window); };
}

void EditorModule::apply_font_settings() {
	const auto &prefs = Config::get_preferences();

	Aquila::UI::Core::FontManager::get().reload(m_engine->get_context(), prefs.fonts);
	Aquila::UI::Core::FontRegistry::set_ui_scale(prefs.ui_scale);
	m_ui_host->get_canvas(Aquila::UI::Core::UILayer::Editor).reload_styles();
}

void EditorModule::wire_menubar(Aquila::UI::Core::View *layout_root) {
	auto wire_btn = [&](const char *id, const char *action) {
		if (auto *btn = layout_root->find_by_id<Aquila::UI::Core::Button>(id)) {
			btn->on_click.connect([action] { AQUILA_LOG_INFO("EditorModule: {}", action); });
		}
	};
	wire_btn("btn-play", "Play");
	wire_btn("btn-pause", "Pause");
	wire_btn("btn-stop", "Stop");

	auto *menu_bar = layout_root->find_by_id<Aquila::UI::Core::MenuBar>("main-menubar");
	if (menu_bar == nullptr) {
		return;
	}

	menu_bar->set_overlay_root(layout_root);

	auto *file_menu = menu_bar->add_menu("File");
	file_menu->add_item("New scene", "Ctrl+N", m_layout_loader.resolve_texture("Engine/UI/Icons/layers-plus.png"),
						[this] { m_engine->request_close(); });
	file_menu->add_item("Open scene", "Ctrl+O", m_layout_loader.resolve_texture("Engine/UI/Icons/folder-open.png"),
						[this] { m_engine->request_close(); });
	file_menu->add_separator();
	file_menu->add_item("Quit", "Ctrl+X", m_layout_loader.resolve_texture("Engine/UI/Icons/ban.png"),
						[this] { m_engine->request_close(); });

	auto *edit_menu = menu_bar->add_menu("Edit");
	edit_menu->add_item("Preferences", "Ctrl+,", nullptr, [this] { open_settings_window(); });
	edit_menu->add_separator();
	edit_menu->add_item("New Empty Scene", {}, nullptr, [this] { new_empty_scene(); });
	edit_menu->add_item("Reset Demo Scene", {}, nullptr, [this] { reset_to_demo_scene(); });

	auto *window_menu = menu_bar->add_menu("Window");
	window_menu->add_item("UI Inspector", {}, m_layout_loader.resolve_texture("Engine/UI/Icons/bug.png"),
						  [this] { m_devtools->open_inspector_window(); });
	window_menu->add_item("Widget Gallery", {}, nullptr, [this] { m_devtools->open_widget_gallery(m_engine->get_context(), m_texture_cache.get()); });
	window_menu->add_separator();
	window_menu->add_item("Hierarchy", {}, nullptr, [] { AQUILA_LOG_INFO("Window: Hierarchy"); });
	window_menu->add_item("Inspector", {}, nullptr, [] { AQUILA_LOG_INFO("Window: Inspector"); });
	window_menu->add_item("Viewport", {}, nullptr, [] { AQUILA_LOG_INFO("Window: Viewport"); });
	window_menu->add_item("Console", {}, nullptr, [] { AQUILA_LOG_INFO("Window: Console"); });
}

}
