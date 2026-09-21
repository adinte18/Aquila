#pragma once

#include "Aquila/Application/EngineContext.h"
#include "Aquila/Application/IModule.h"
#include "Aquila/Rendering/IRenderWindowHost.h"
#include "Aquila/UI/Core/LayoutLoader.h"
#include "Aquila/UI/Core/TextureCache.h"
#include "Aquila/UI/Core/UIHost.h"
#include "Aquila/UI/DevTools/UIDevTools.h"
#include "Aquila/Rendering/CameraController.h"
#include "Aquila/Scene/Entity.h"
#include "Aquila/UI/Core/View.h"
#include "Aquila/UI/Widgets/Collapsible.h"
#include "Aquila/UI/Widgets/PopupMenu.h"
#include "Aquila/Scene/SceneStatistics.h"
#include "Core/EditorTools.h"
#include "Core/ProjectManager.h"

#include <array>
#include <vector>

namespace Aquila::UI::Core {
class Button;
class View;
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
	void wire_main_menu(Aquila::UI::Core::View *layout_root);
	void wire_tools(Aquila::UI::Core::View *layout_root);
	void refresh_tool_buttons();
	void wire_cards(Aquila::UI::Core::View *layout_root);
	void set_card_visible(Usize index, bool visible);
	void toggle_card(Usize index);
	void refresh_card_buttons();
	void open_settings_window();
	void open_project_launcher();
	void enter_editor(const ProjectInfo &project);
	void apply_font_settings();
	void set_ui_scale(F32 scale);

	Aquila::Application::EngineContext *m_engine = nullptr;
	EditorTools m_tools;
	Aquila::SceneManagement::StatisticsCollector m_statistics;
	Unique<class StatusBar> m_status_bar;
	std::array<Aquila::UI::Core::Button *, 3> m_tool_buttons{};
	Aquila::UI::Core::Button *m_space_button = nullptr;

	struct CardState {
		Aquila::UI::Core::Collapsible *card = nullptr;
		bool visible = true;
	};
	std::array<CardState, 3> m_cards{};
	Aquila::UI::Core::Button *m_console_button = nullptr;
	Aquila::UI::Core::PopupMenu *m_main_menu = nullptr;
	Unique<Aquila::UI::Core::UIHost> m_ui_host;
	Unique<ProjectManager> m_project_manager;
	Unique<Aquila::UI::Core::TextureCache> m_texture_cache;
	Unique<Aquila::Rendering::CameraController> m_editor_camera;

	Unique<ViewportPanel> m_viewport_panel;
	Unique<HierarchyPanel> m_hierarchy_panel;
	Unique<InspectorPanel> m_inspector_panel;
	Unique<ConsolePanel> m_console_panel;
	Unique<Aquila::UI::DevTools::UIDevTools> m_devtools;
	Unique<SettingsWindow> m_settings_window;
	Unique<ProjectLauncher> m_project_launcher;
	Aquila::Rendering::RenderWindowId m_launcher_window = nullptr;
	bool m_editor_entered = false;
	Option<ProjectInfo> m_pending_project;

	Aquila::SceneManagement::Entity m_outlined_entity;

	Aquila::UI::Core::LayoutLoader m_layout_loader;

};

}
