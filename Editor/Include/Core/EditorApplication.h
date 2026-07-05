#pragma once

#include "Aquila/Application/ApplicationNew.h"
#include "Aquila/UI/Core/TextureCache.h"

#include <vector>

struct GLFWwindow;

namespace Aquila::UI::Core {
class View;
class DockSpace;
} // namespace Aquila::UI::Core

namespace Editor {

class ViewportPanel;
class HierarchyPanel;
class InspectorPanel;
class ConsolePanel;
class UIDebugPanel;
class UIDebugWindow;
class WidgetGalleryWindow;
class FloatingPanelWindow;
class PickerOverlay;

class EditorApplication : public Aquila::Application::Application {
  public:
	explicit EditorApplication(const ApplicationSpec &spec);
	~EditorApplication() override;

  protected:
	void on_init() override;
	void on_shutdown() override;
	void on_pre_render(F32 delta_time) override;
	void on_event(Aquila::Application::Events::Event &event) override;
	void on_resize(Uint32 width, Uint32 height) override;

  private:
	void setup_scene();
	void setup_editor_ui();
	void wire_menubar(Aquila::UI::Core::View *layout_root);
	void open_ui_inspector_window();
	void open_widget_gallery_window();
	void start_pick();

	void wire_dock_space(Aquila::UI::Core::DockSpace *dock_space, GLFWwindow *source_native);
	void handle_tear_off(GLFWwindow *source_native, Unique<Aquila::UI::Core::View> content, std::string title,
					   Vec2 source_local);
	void preview_dock_targets(GLFWwindow *source_native, Vec2 source_local);
	void clear_dock_target_previews();
	Aquila::UI::Core::DockSpace *find_dock_target_at_screen(Vec2 screen_pos, GLFWwindow *exclude, Vec2 &out_local);
	void close_floating_window(GLFWwindow *native);
	void spawn_floating_panel(Unique<Aquila::UI::Core::View> panel_subtree, std::string title, Vec2 screen_pos);
	void on_floating_closed(FloatingPanelWindow *panel);
	void dock_back_to_center(Unique<Aquila::UI::Core::View> content, const std::string &title);

	Unique<Aquila::UI::Core::TextureCache> m_texture_cache;

	Unique<ViewportPanel> m_viewport_panel;
	Unique<HierarchyPanel> m_hierarchy_panel;
	Unique<InspectorPanel> m_inspector_panel;
	Unique<ConsolePanel> m_console_panel;
	Unique<UIDebugPanel> m_ui_debug_panel;
	Unique<UIDebugWindow> m_ui_debug_window;
	Unique<WidgetGalleryWindow> m_widget_gallery_window;

	PickerOverlay *m_picker = nullptr;
	bool m_pick_mode = false;

	Aquila::UI::Core::DockSpace *m_dock_space = nullptr;

	struct FloatingEntry {
		Unique<FloatingPanelWindow> panel;
		Aquila::Application::RenderWindow *window = nullptr;
	};
	std::vector<FloatingEntry> m_floating_panels;
};

} // namespace Editor
