#include "Core/EditorApplication.h"
#include "Core/EditorCamera.h"
#include "Core/ProjectManager.h"

#include "UI/Managers/FontManager.h"
#include "UI/Panels/ConsolePanel.h"
#include "UI/Panels/HierarchyPanel.h"
#include "UI/Debug/FloatingPanelWindow.h"
#include "UI/Debug/UIDebugPanel.h"
#include "UI/Debug/UIDebugWindow.h"
#include "UI/Debug/WidgetGalleryWindow.h"
#include "UI/Windows/SettingsWindow.h"
#include "UI/Windows/ProjectLauncher.h"
#include "UI/Debug/PickerOverlay.h"
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
#include "Aquila/Scene/Components/SkyLightComponent.h"
#include "Aquila/Scene/Components/TransformComponent.h"
#include "Aquila/Scene/EntityManager.h"
#include "Aquila/UI/Core/Clipboard.h"
#include "Aquila/UI/Core/FontRegistry.h"
#include "Aquila/UI/Core/LayoutLoader.h"
#include "Aquila/UI/Core/CanvasManager.h"
#include "Aquila/UI/Rendering/ViewRenderingSystem.h"
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
#include "Aquila/Application/Events/InputEvent.h"
#include "Aquila/Platform/Input.h"

#include <algorithm>

namespace Editor {

using namespace Aquila;
using namespace Aquila::SceneManagement;
using namespace Aquila::SceneManagement::Components;
using namespace Aquila::Application;

namespace {
const std::string k_layout_path = "/app/layout.aqdl";

std::string pick_label(Aquila::UI::Core::View *view) {
	std::string label(view->get_type_name());
	if (!view->get_id().empty()) {
		label += " #" + view->get_id();
	} else if (!view->get_classes().empty()) {
		label += " ." + view->get_classes().front();
	}
	return label;
}
} // namespace

EditorApplication::EditorApplication(const ApplicationSpec &spec) : Application(spec) {}

EditorApplication::~EditorApplication() = default;

void EditorApplication::on_init() {
	Aquila::UI::Core::CanvasManager::init(get_window().get_width(), get_window().get_height());
	get_renderer2_d().add_system<Aquila::UI::Rendering::ViewRenderingSystem>();

	{
		GLFWwindow *native_win = get_window().get_native_window();
		Aquila::UI::Core::Clipboard::init(
			[native_win]() -> std::string {
				const char *s = glfwGetClipboardString(native_win);
				return s ? s : "";
			},
			[native_win](const std::string &t) { glfwSetClipboardString(native_win, t.c_str()); });
	}

	Config::get_preferences().load_from_file();
	Aquila::UI::Core::FontRegistry::set_ui_scale(Config::get_preferences().ui_scale);

	UI::FontManager::get().initialize(get_context(), Config::get_preferences().fonts);

	m_project_manager = std::make_unique<ProjectManager>();

	open_project_launcher();
}

void EditorApplication::open_project_launcher() {
	RenderWindow &rw = create_secondary_window(720, 520, "Aquila - Projects");
	m_launcher_native = rw.window->get_native_window();

	m_project_launcher = std::make_unique<ProjectLauncher>();
	m_project_launcher->build(m_project_manager.get(), 720, 520, Config::get_preferences().ui.style_path);
	m_project_launcher->on_project_ready = [this](const ProjectInfo &project) { m_pending_project = project; };

	ProjectLauncher *win = m_project_launcher.get();
	rw.on_update = [win](F32 dt) { win->update(dt); };
	rw.on_render = [win](auto &batcher, auto &cmd) { win->render(batcher, cmd); };
	rw.on_event = [win](Events::Event &event) { win->on_event(event); };
	rw.on_close = [this] {
		m_project_launcher.reset();
		m_launcher_native = nullptr;
		if (!m_editor_entered) {
			close();
		}
	};
}

void EditorApplication::enter_editor(const ProjectInfo &project) {
	AQUILA_LOG_INFO("EditorApplication: opening project '{}' ({})", project.name, project.directory);

	m_editor_entered = true;
	if (m_launcher_native != nullptr) {
		glfwSetWindowShouldClose(m_launcher_native, GLFW_TRUE);
	}

	glfwShowWindow(get_window().get_native_window());
	m_editor_camera = std::make_unique<EditorCamera>();
	setup_editor_ui();
}

void EditorApplication::on_shutdown() {
	if (m_dock_space != nullptr) {
		const Aquila::UI::Core::DockLayoutDesc layout = Aquila::UI::Core::DockLayoutSerializer::capture(*m_dock_space);
		if (Aquila::UI::Core::DockLayoutSerializer::save_to_file(k_layout_path, layout)) {
			AQUILA_LOG_INFO("Editor dock layout saved to {}", k_layout_path);
		}
	}

	m_floating_panels.clear();
	m_settings_window.reset();
	m_project_launcher.reset();
	m_widget_gallery_window.reset();
	m_ui_debug_window.reset();
	m_ui_debug_panel.reset();
	m_hierarchy_panel.reset();
	m_viewport_panel.reset();
	m_inspector_panel.reset();
	m_console_panel.reset();
	m_texture_cache.reset();

	Aquila::UI::Core::CanvasManager::shutdown();
	UI::FontManager::get().shutdown();
}

void EditorApplication::on_pre_render(F32 delta_time) {
	if (m_pending_project) {
		const ProjectInfo project = *m_pending_project;
		m_pending_project.reset();
		enter_editor(project);
	}

	if (m_console_panel) {
		m_console_panel->flush_pending();
	}
	Aquila::UI::Core::CanvasManager::get()->update(delta_time);
	Aquila::UI::Core::CanvasManager::get()->compute();

	if (m_editor_camera && m_viewport_panel) {
		const Rect viewport = m_viewport_panel->get_content_rect();
		const auto view_width = static_cast<Uint32>(viewport.size.x);
		const auto view_height = static_cast<Uint32>(viewport.size.y);
		m_editor_camera->set_viewport_rect(viewport.position, viewport.size);
		m_editor_camera->set_viewport_size(view_width, view_height);
		m_editor_camera->update(delta_time);
		get_render_pipeline().set_primary_view(m_editor_camera->get_render_view());
	}

	get_window().set_cursor(Aquila::UI::Core::CanvasManager::get()->get_active_cursor());
}

void EditorApplication::on_event(Events::Event &event) {
	Events::EventDispatcher dispatcher(event);

	if (m_editor_camera) {
		m_editor_camera->on_event(event);
	}

	if (m_pick_mode) {
		bool consumed = false;
		dispatcher.dispatch<Events::MouseMovedEvent>([&](Events::MouseMovedEvent &e) {
			auto &editor_canvas = Aquila::UI::Core::CanvasManager::get()->get_layer(Aquila::UI::Core::UILayer::Editor);
			Aquila::UI::Core::View *hit = editor_canvas.hit_test({ e.get_x(), e.get_y() });
			consumed = true;
			if (hit == m_pick_hover) {
				return true;
			}
			m_pick_hover = hit;
			if (m_picker) {
				hit ? m_picker->set_target(hit->get_absolute_rect(), pick_label(hit)) : m_picker->clear();
			}
			return true;
		});
		dispatcher.dispatch<Events::MouseButtonPressedEvent>([&](Events::MouseButtonPressedEvent &) {
			m_pick_mode = false;
			if (m_picker) {
				m_picker->clear();
			}
			if (m_ui_debug_window && m_pick_hover) {
				m_ui_debug_window->select_view(m_pick_hover);
			}
			m_pick_hover = nullptr;
			consumed = true;
			return true;
		});
		dispatcher.dispatch<Events::KeyPressedEvent>([&](Events::KeyPressedEvent &e) {
			if (e.get_key_code() == Events::KeyCode::Escape) {
				m_pick_mode = false;
				if (m_picker != nullptr) {
					m_picker->clear();
				}
				m_pick_hover = nullptr;
			}

			consumed = true;
			return true;
		});
		if (consumed) {
			return;
		}
	}

	dispatcher.dispatch<Events::KeyPressedEvent>([this](Events::KeyPressedEvent &e) {
		if (e.get_key_code() == Events::KeyCode::F5) {
			m_layout_loader.load_file(Config::get_preferences().ui.layout_path);
			AQUILA_LOG_INFO("Editor layout reloaded");
			return true;
		}

		if (e.get_key_code() == Events::KeyCode::F6) {
			auto &canvas = Aquila::UI::Core::CanvasManager::get()->get_layer(Aquila::UI::Core::UILayer::Editor);
			Aquila::UI::StyleParser::load_file(Config::get_preferences().ui.style_path, canvas.get_style_sheet());
			canvas.reload_styles();

			AQUILA_LOG_INFO("Stylesheet reloaded");
			return true;
		}
		return false;
	});

	Aquila::UI::Core::CanvasManager::get()->on_event(event);

	Events::EventDispatcher post(event);
	post.dispatch<Events::KeyPressedEvent>([this](Events::KeyPressedEvent &e) {
		if (e.get_key_code() != Events::KeyCode::S || (e.get_mods() & Events::MODIFIER_SHIFT) == 0) {
			return false;
		}
		auto &canvas = Aquila::UI::Core::CanvasManager::get()->get_layer(Aquila::UI::Core::UILayer::Editor);
		if (Aquila::UI::Core::view_is<Aquila::UI::Core::TextInput>(canvas.get_focused_view())) {
			return false;
		}
		if (m_inspector_panel != nullptr) {
			m_inspector_panel->open_add_search(Aquila::Platform::Input::get_mouse_position());
		}
		return true;
	});
}

void EditorApplication::on_resize(Uint32 width, Uint32 height) {
	if (m_viewport_panel) {
		m_viewport_panel->set_texture(&get_render_output());
	}
}

void EditorApplication::on_render_resize(Uint32 width, Uint32 height) {
	if (m_viewport_panel) {
		m_viewport_panel->set_texture(&get_render_output());
	}
}

void EditorApplication::spawn_default_camera() {
	auto *em = get_scene().get_entity_manager();
	auto cam = em->create_entity("Camera");
	auto &cam_comp = cam.add_component<CameraComponent>();
	cam_comp.fov = 60.F;
	cam_comp.near_plane = 0.1f;
	cam_comp.far_plane = 500.F;
	cam_comp.aspect_ratio = static_cast<F32>(get_window().get_width()) / static_cast<F32>(get_window().get_height());
	cam_comp.primary = true;
	cam.get_component<TransformComponent>().set_local_position({ 0.F, 1.5f, -5.F });
	get_scene().set_active_camera(cam);
}

void EditorApplication::populate_demo_scene() {
	auto *em = get_scene().get_entity_manager();
	spawn_default_camera();

	auto lit_mat = Graphics::MaterialFactory::get()->create(get_context(), SharedConstants::SHADERS_DIR + "Basic.slang",
															{
																.type = Graphics::MaterialType::Lit,
																.color_formats = { RHI::TextureFormat::RGBA16F },
																.depth_test = true,
																.depth_write = true,
															});

	auto add_cube = [&](const char *name, Vec3 pos) {
		auto entity = em->create_entity(name);
		auto mesh = std::make_shared<Graphics::Resources::Mesh>(name);
		mesh->load_from_data(Graphics::Resources::Mesh::generate_cube(0.5f));
		entity.add_component<MeshComponent>().set_mesh(mesh);
		entity.get_component<TransformComponent>().set_local_position(pos);
		auto &mat = entity.add_component<MaterialComponent>(lit_mat);
		mat.surface_properties.albedo = Vec4(0.8f, 0.6f, 0.4f, 1.F);
		mat.surface_properties.metallic = 0.0f;
		mat.surface_properties.roughness = 0.6f;
		return entity;
	};
	add_cube("CubeA", { -2.5f, 1.F, 2.F });
	add_cube("CubeB", { 2.5f, 1.F, 2.F });
	auto floor = add_cube("Floor", { 0.0f, 0.f, 2.F });
	floor.get_component<TransformComponent>().set_local_scale({ 12.F, 0.2f, 12.F });

	{
		auto e = em->create_entity("SunLight");
		auto &light = e.add_component<LightComponent>(LightComponent::Type::Directional, Vec3(1.0f, 0.95f, 0.8f), 1.0f);
		light.set_direction(glm::normalize(Vec3(0.4f, -1.0f, 0.6f)));
	}
	{
		auto e = em->create_entity("PointA");
		e.get_component<TransformComponent>().set_local_position({ -1.5f, 0.5f, 1.5f });
		auto &light = e.add_component<LightComponent>(LightComponent::Type::Point, Vec3(1.0f, 0.4f, 0.1f), 1.0f);
		light.set_range(6.0f);
	}
	{
		auto e = em->create_entity("PointB");
		e.get_component<TransformComponent>().set_local_position({ 1.5f, 0.5f, 1.5f });
		auto &light = e.add_component<LightComponent>(LightComponent::Type::Point, Vec3(0.2f, 0.5f, 1.0f), 1.0f);
		light.set_range(6.0f);
	}
	{
		auto e = em->create_entity("Sky");
		e.add_component<SkyLightComponent>();
	}
}

void EditorApplication::refresh_scene_panels() {
	if (m_hierarchy_panel) {
		m_hierarchy_panel->rebuild();
	}
	if (m_inspector_panel) {
		m_inspector_panel->clear();
	}
}

void EditorApplication::new_empty_scene() {
	get_scene().clear();
	spawn_default_camera();
	refresh_scene_panels();
	AQUILA_LOG_INFO("EditorApplication: new empty scene");
}

void EditorApplication::reset_to_demo_scene() {
	get_scene().clear();
	populate_demo_scene();
	refresh_scene_panels();
	AQUILA_LOG_INFO("EditorApplication: reset to demo scene");
}

void EditorApplication::setup_editor_ui() {
	auto &editor_canvas = Aquila::UI::Core::CanvasManager::get()->get_layer(Aquila::UI::Core::UILayer::Editor);
	const auto &cfg = Config::get_preferences();

	m_texture_cache = std::make_unique<Aquila::UI::Core::TextureCache>(get_context(), cfg.ui.resources_path);

	Aquila::UI::StyleParser::load_file(cfg.ui.style_path, editor_canvas.get_style_sheet());

	m_layout_loader.register_font("regular", UI::FontManager::get().get_font("regular"));
	m_layout_loader.register_texture_cache(m_texture_cache.get());
	m_layout_loader.register_widget(
		"ColorPicker", [this](std::string_view, Aquila::UI::Text::FontAtlas *) -> Unique<Aquila::UI::Core::View> {
			return std::make_unique<Aquila::UI::Core::ColorPicker>(get_context());
		});

	m_layout_loader.register_command("entity.create", [this] {
		auto entity = get_scene().get_entity_manager()->create_entity("New Entity");
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
		AQUILA_LOG_ERROR("EditorApplication: failed to load editor layout from {}", cfg.ui.layout_path);
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
		AQUILA_LOG_ERROR("EditorApplication: editor dock layout not found — check editor.aqlayout");
		return;
	}

	wire_dock_space(m_dock_space, get_window().get_native_window());

	m_hierarchy_panel = std::make_unique<HierarchyPanel>(*get_scene().get_entity_manager());
	m_viewport_panel = std::make_unique<ViewportPanel>(get_render_output());
	m_inspector_panel = std::make_unique<InspectorPanel>(get_context(), m_texture_cache.get());
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

	wire_menubar(layout_root);

	m_ui_debug_panel = std::make_unique<UIDebugPanel>();
	m_ui_debug_panel->build(layout_root, &editor_canvas);

	m_picker = editor_canvas.get_root()->add_child<PickerOverlay>();

	if (Option<Aquila::UI::Core::DockLayoutDesc> saved =
			Aquila::UI::Core::DockLayoutSerializer::load_from_file(k_layout_path)) {
		m_dock_space->apply_layout(*saved);
		AQUILA_LOG_INFO("Editor dock layout restored from {}", k_layout_path);
	}

	editor_canvas.reload_styles();
}

void EditorApplication::open_ui_inspector_window() {
	if (m_ui_debug_window) {
		return; // already open
	}

	auto &editor_canvas = Aquila::UI::Core::CanvasManager::get()->get_layer(Aquila::UI::Core::UILayer::Editor);
	RenderWindow &rw = create_secondary_window(800, 600, "Aquila - UI Inspector");

	m_ui_debug_window = std::make_unique<UIDebugWindow>();
	m_ui_debug_window->build(&editor_canvas, 800, 600, Config::get_preferences().ui.style_path);

	m_ui_debug_window->on_pick_requested = [this] { start_pick(); };
	m_ui_debug_window->set_ignored_view(m_picker);
	m_ui_debug_window->on_view_highlighted = [this](Aquila::UI::Core::View *view) {
		if (m_picker && view) {
			m_picker->set_target(view->get_absolute_rect(), pick_label(view));
		}
	};

	UIDebugWindow *win = m_ui_debug_window.get();
	rw.on_update = [win](F32 dt) { win->update(dt); };
	rw.on_render = [win](auto &batcher, auto &cmd) { win->render(batcher, cmd); };
	rw.on_event = [win](Events::Event &event) { win->on_event(event); };
	rw.on_close = [this] {
		m_pick_mode = false;
		if (m_picker) {
			m_picker->clear();
		}
		m_ui_debug_window.reset();
	};
}

void EditorApplication::start_pick() {
	if (!m_ui_debug_window) {
		return;
	}
	m_ui_debug_window->refresh();
	m_pick_hover = nullptr;
	m_pick_mode = true;
}

void EditorApplication::open_widget_gallery_window() {
	if (m_widget_gallery_window) {
		return;
	}

	RenderWindow &rw = create_secondary_window(420, 720, "Aquila - Widget Gallery");

	m_widget_gallery_window = std::make_unique<WidgetGalleryWindow>();
	m_widget_gallery_window->build(get_context(), m_texture_cache.get(), 420, 720,
								   Config::get_preferences().ui.style_path);

	WidgetGalleryWindow *win = m_widget_gallery_window.get();
	rw.on_update = [win](F32 dt) { win->update(dt); };
	rw.on_render = [win](auto &batcher, auto &cmd) { win->render(batcher, cmd); };
	rw.on_event = [win](Events::Event &event) { win->on_event(event); };
	rw.on_close = [this] { m_widget_gallery_window.reset(); };
}

void EditorApplication::open_settings_window() {
	if (m_settings_window) {
		return;
	}

	RenderWindow &rw = create_secondary_window(560, 640, "Aquila - Settings");
	GLFWwindow *native = rw.window->get_native_window();

	m_settings_window = std::make_unique<SettingsWindow>();
	m_settings_window->build(m_texture_cache.get(), 560, 640, Config::get_preferences().ui.style_path);
	m_settings_window->on_request_close = [native] { glfwSetWindowShouldClose(native, GLFW_TRUE); };
	m_settings_window->on_applied = [this] { apply_font_settings(); };

	SettingsWindow *win = m_settings_window.get();
	rw.on_update = [win](F32 dt) { win->update(dt); };
	rw.on_render = [win](auto &batcher, auto &cmd) { win->render(batcher, cmd); };
	rw.on_event = [win](Events::Event &event) { win->on_event(event); };
	rw.on_close = [this] { m_settings_window.reset(); };
}

void EditorApplication::apply_font_settings() {
	const auto &prefs = Config::get_preferences();

	UI::FontManager::get().reload(get_context(), prefs.fonts);
	Aquila::UI::Core::FontRegistry::set_ui_scale(prefs.ui_scale);
	Aquila::UI::Core::CanvasManager::get()->get_layer(Aquila::UI::Core::UILayer::Editor).reload_styles();
}

void EditorApplication::wire_dock_space(Aquila::UI::Core::DockSpace *dock_space, GLFWwindow *source_native) {
	dock_space->set_tear_off_callback(
		[this, source_native](Unique<Aquila::UI::Core::View> sub, std::string title, Vec2 pos) {
			handle_tear_off(source_native, std::move(sub), std::move(title), pos);
		});
	dock_space->set_external_drag_observer(
		[this, source_native](Vec2 pos) { preview_dock_targets(source_native, pos); },
		[this] { clear_dock_target_previews(); });
	dock_space->set_emptied_callback([this, source_native] { close_floating_window(source_native); });
}

void EditorApplication::close_floating_window(GLFWwindow *native) {
	for (auto &entry : m_floating_panels) {
		if (entry.window->window->get_native_window() == native) {
			glfwSetWindowShouldClose(native, GLFW_TRUE);
			return;
		}
	}
}

Aquila::UI::Core::DockSpace *EditorApplication::find_dock_target_at_screen(Vec2 screen_pos, GLFWwindow *exclude,
																		   Vec2 &out_local) {
	struct Candidate {
		Aquila::UI::Core::DockSpace *dock_space;
		GLFWwindow *native;
		float width;
		float height;
	};
	std::vector<Candidate> candidates;
	candidates.reserve(m_floating_panels.size());
	for (auto &entry : m_floating_panels) {
		candidates.push_back({ entry.panel->get_dock_space(), entry.window->window->get_native_window(),
							   static_cast<float>(entry.window->window->get_width()),
							   static_cast<float>(entry.window->window->get_height()) });
	}
	candidates.push_back({ m_dock_space, get_window().get_native_window(), static_cast<float>(get_window().get_width()),
						   static_cast<float>(get_window().get_height()) });

	for (auto &c : candidates) {
		if (c.native == exclude) {
			continue;
		}
		int cx = 0;
		int cy = 0;
		glfwGetWindowPos(c.native, &cx, &cy);
		const Vec2 local = { screen_pos.x - static_cast<float>(cx), screen_pos.y - static_cast<float>(cy) };
		if (local.x >= 0.F && local.y >= 0.F && local.x < c.width && local.y < c.height) {
			out_local = local;
			return c.dock_space;
		}
	}
	return nullptr;
}

void EditorApplication::preview_dock_targets(GLFWwindow *source_native, Vec2 source_local) {
	int sx = 0;
	int sy = 0;
	glfwGetWindowPos(source_native, &sx, &sy);
	const Vec2 screen = { static_cast<float>(sx) + source_local.x, static_cast<float>(sy) + source_local.y };

	clear_dock_target_previews();

	Vec2 target_local{ 0.F, 0.F };
	if (Aquila::UI::Core::DockSpace *target = find_dock_target_at_screen(screen, source_native, target_local)) {
		target->preview_external_drag(target_local);
	}
}

void EditorApplication::clear_dock_target_previews() {
	m_dock_space->clear_external_drag();
	for (auto &entry : m_floating_panels) {
		entry.panel->get_dock_space()->clear_external_drag();
	}
}

void EditorApplication::handle_tear_off(GLFWwindow *source_native, Unique<Aquila::UI::Core::View> content,
										std::string title, Vec2 source_local) {
	clear_dock_target_previews();

	int sx = 0, sy = 0;
	glfwGetWindowPos(source_native, &sx, &sy);
	const Vec2 screen = { static_cast<float>(sx) + source_local.x, static_cast<float>(sy) + source_local.y };

	// A release inside the source window's own bounds floats the panel — the source sits on top
	int sw = 0, sh = 0;
	glfwGetWindowSize(source_native, &sw, &sh);
	const bool inside_source = source_local.x >= 0.F && source_local.y >= 0.F &&
		source_local.x < static_cast<float>(sw) && source_local.y < static_cast<float>(sh);

	Vec2 target_local{ 0.F, 0.F };
	Aquila::UI::Core::DockSpace *target =
		inside_source ? nullptr : find_dock_target_at_screen(screen, source_native, target_local);
	const bool docked = target && target->try_dock_external(content, title, target_local);

	if (!docked) {
		spawn_floating_panel(std::move(content), std::move(title), screen);
	}

	// A floating window drained of its last tab has nothing left to show — close it.
	for (auto &entry : m_floating_panels) {
		if (entry.window->window->get_native_window() == source_native && !entry.panel->has_content()) {
			close_floating_window(source_native);
			break;
		}
	}
}

void EditorApplication::spawn_floating_panel(Unique<Aquila::UI::Core::View> panel_subtree, std::string title,
											 Vec2 screen_pos) {
	RenderWindow &rw = create_secondary_window(800, 600, title);
	glfwSetWindowPos(rw.window->get_native_window(), static_cast<int>(screen_pos.x) - 60,
					 static_cast<int>(screen_pos.y) - 12);

	auto fpw = std::make_unique<FloatingPanelWindow>();
	fpw->build(std::move(panel_subtree), title, 800, 600, Config::get_preferences().ui.style_path);

	FloatingPanelWindow *panel = fpw.get();
	RenderWindow *window = &rw;

	rw.on_update = [panel](F32 dt) { panel->update(dt); };
	rw.on_render = [panel](auto &batcher, auto &cmd) { panel->render(batcher, cmd); };
	rw.on_event = [panel](Events::Event &event) { panel->on_event(event); };
	rw.on_close = [this, panel] { on_floating_closed(panel); };

	wire_dock_space(panel->get_dock_space(), window->window->get_native_window());

	m_floating_panels.push_back({ std::move(fpw), window });
}

void EditorApplication::on_floating_closed(FloatingPanelWindow *panel) {
	auto it =
		std::ranges::find_if(m_floating_panels, [panel](const FloatingEntry &e) { return e.panel.get() == panel; });
	if (it == m_floating_panels.end()) {
		return;
	}

	while (panel->has_content()) {
		auto content = panel->detach_content();
		if (!content) {
			break;
		}
		dock_back_to_center(std::move(content), panel->get_title());
	}

	m_floating_panels.erase(it);
}

void EditorApplication::dock_back_to_center(Unique<Aquila::UI::Core::View> content, const std::string &title) {
	if (!content || (m_dock_space == nullptr)) {
		return;
	}

	Aquila::UI::Core::DockNode *node =
		m_dock_space->get_root_node()->hit_test_node(m_dock_space->get_absolute_rect().center());
	if (node == nullptr) {
		return;
	}
	node->accept_panel(std::move(content), title, Aquila::UI::Core::DropZone::Center);
}

void EditorApplication::wire_menubar(Aquila::UI::Core::View *layout_root) {
	auto wire_btn = [&](const char *id, const char *action) {
		if (auto *btn = layout_root->find_by_id<Aquila::UI::Core::Button>(id)) {
			btn->on_click.connect([action] { AQUILA_LOG_INFO("EditorApplication: {}", action); });
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
						[this] { close(); });
	file_menu->add_item("Open scene", "Ctrl+O", m_layout_loader.resolve_texture("Engine/UI/Icons/folder-open.png"),
						[this] { close(); });
	file_menu->add_separator();
	file_menu->add_item("Quit", "Ctrl+X", m_layout_loader.resolve_texture("Engine/UI/Icons/ban.png"),
						[this] { close(); });

	auto *edit_menu = menu_bar->add_menu("Edit");
	edit_menu->add_item("Preferences", "Ctrl+,", nullptr, [this] { open_settings_window(); });
	edit_menu->add_separator();
	edit_menu->add_item("New Empty Scene", {}, nullptr, [this] { new_empty_scene(); });
	edit_menu->add_item("Reset Demo Scene", {}, nullptr, [this] { reset_to_demo_scene(); });

	auto *window_menu = menu_bar->add_menu("Window");
	window_menu->add_item("UI Inspector", {}, m_layout_loader.resolve_texture("Engine/UI/Icons/bug.png"),
						  [this] { open_ui_inspector_window(); });
	window_menu->add_item("Widget Gallery", {}, nullptr, [this] { open_widget_gallery_window(); });
	window_menu->add_separator();
	window_menu->add_item("Hierarchy", {}, nullptr, [] { AQUILA_LOG_INFO("Window: Hierarchy"); });
	window_menu->add_item("Inspector", {}, nullptr, [] { AQUILA_LOG_INFO("Window: Inspector"); });
	window_menu->add_item("Viewport", {}, nullptr, [] { AQUILA_LOG_INFO("Window: Viewport"); });
	window_menu->add_item("Console", {}, nullptr, [] { AQUILA_LOG_INFO("Window: Console"); });
}

} // namespace Editor
