#pragma once

#include "Aquila/Application/EngineContext.h"
#include "Aquila/Application/IModule.h"
#include "Aquila/Rendering/IRenderWindowHost.h"
#include "Aquila/UI/Core/DockWindowManager.h"
#include "Aquila/UI/Core/LayoutLoader.h"
#include "Aquila/UI/Core/TextureCache.h"
#include "Aquila/UI/Core/UIHost.h"
#include "Aquila/UI/DevTools/PickerOverlay.h"
#include "Aquila/UI/DevTools/UIDebugPanel.h"
#include "Aquila/UI/DevTools/UIDebugWindow.h"
#include "Aquila/UI/DevTools/WidgetGalleryWindow.h"
#include "Aquila/Rendering/CameraController.h"
#include "Aquila/Scene/Entity.h"
#include "Aquila/UI/Core/View.h"
#include "Core/ProjectManager.h"

#include <vector>

namespace Aquila::UI::Core {
class View;
class DockSpace;
}

namespace Editor {

class ViewportPanel;
class HierarchyPanel;
class InspectorPanel;
class ConsolePanel;
class SettingsWindow;
class ProjectLauncher;
class ProjectManager;

class EditorModule final : public Aquila::Application::IModule {
  public:
	EditorModule();
	~EditorModule() override;

	void on_attach(Aquila::Application::EngineContext &engine) override;
	void on_detach() override;
	void on_pre_render(F32 delta_time) override;
	void on_event(Aquila::Platform::Events::Event &event) override;
	void on_resize(Uint32 width, Uint32 height) override;
	void on_render_resize(Uint32 width, Uint32 height) override;

  private:
	F32 window_aspect();
	void new_empty_scene();
	void reset_to_demo_scene();
	void refresh_scene_panels();
	void request_viewport_pick(Vec2 uv);
	void set_outlined_entity(Aquila::SceneManagement::Entity entity);
	void setup_editor_ui();
	void wire_menubar(Aquila::UI::Core::View *layout_root);
	void open_ui_inspector_window();
	void open_widget_gallery_window();
	void open_settings_window();
	void open_project_launcher();
	void enter_editor(const ProjectInfo &project);
	void apply_font_settings();
	void start_pick();

	Aquila::Application::EngineContext *m_engine = nullptr;
	Unique<Aquila::UI::Core::UIHost> m_ui_host;
	Unique<ProjectManager> m_project_manager;
	Unique<Aquila::UI::Core::TextureCache> m_texture_cache;
	Unique<Aquila::Rendering::CameraController> m_editor_camera;

	Unique<ViewportPanel> m_viewport_panel;
	Unique<HierarchyPanel> m_hierarchy_panel;
	Unique<InspectorPanel> m_inspector_panel;
	Unique<ConsolePanel> m_console_panel;
	Unique<Aquila::UI::DevTools::UIDebugPanel> m_ui_debug_panel;
	Unique<Aquila::UI::DevTools::UIDebugWindow> m_ui_debug_window;
	Unique<Aquila::UI::DevTools::WidgetGalleryWindow> m_widget_gallery_window;
	Unique<SettingsWindow> m_settings_window;
	Unique<ProjectLauncher> m_project_launcher;
	Aquila::Rendering::RenderWindowId m_launcher_window = nullptr;
	bool m_editor_entered = false;
	Option<ProjectInfo> m_pending_project;

	Aquila::SceneManagement::Entity m_outlined_entity;

	Aquila::UI::DevTools::PickerOverlay *m_picker = nullptr;
	Aquila::UI::Core::ViewRef m_pick_hover;
	bool m_pick_mode = false;
	Aquila::UI::Core::LayoutLoader m_layout_loader;

	Aquila::UI::Core::DockSpace *m_dock_space = nullptr;
	Unique<Aquila::UI::Core::DockWindowManager> m_dock_manager;
};

}
