#ifndef EDITOR_APPLICATION_H
#define EDITOR_APPLICATION_H

#include "Aquila/Core/Application.h"
#include "UI/Windows/AboutWindow.h"
#include "UI/Windows/PreferencesWindow.h"
#include "UI/Windows/ProfilerWindow.h"

namespace Editor {
class EditorLayer;
class Menubar;
class ViewportPanel;
class ContentBrowserPanel;
class PropertiesPanel;
class SceneHierarchyPanel;
class MaterialEditorPanel;
} // namespace Editor

namespace Editor {

/**
 * @brief Main editor application class
 *
 * Manages the editor's lifecycle, initializes all systems,
 * and coordinates between different editor components.
 */
class Application : public Aquila::Core::Application {
  public:
	explicit Application(const Aquila::Core::ApplicationConfig &config);
	~Application() override;

	void OnInit() override;
	void OnUpdate(f32 deltaTime) override;
	void OnRender(VkCommandBuffer commandBuffer) override;
	void OnShutdown() override;

	void BeginUIFrame() override;
	void EndUIFrame(VkCommandBuffer commandBuffer) override;

	UI::AboutWindow *GetAboutWindow() { return m_AboutWindow.get(); }
	UI::PreferencesWindow *GetPreferencesWindow() { return m_PreferencesWindow.get(); }
	void RequestFontReload() { m_FontReloadRequested = true; }

  private:
	void InitializeEditor();
	void InitializeImGui();
	void InitializeManagers();
	void ShutdownImGui();
	void ShutdownManagers();

	void LoadEditorPreferences();
	void ProcessDeferredOperations();

	Unique<Menubar> m_MenubarLayer;
	Unique<EditorLayer> m_EditorLayer;
	Unique<ViewportPanel> m_ViewportLayer;
	Unique<ContentBrowserPanel> m_ContentBrowserLayer;
	Unique<PropertiesPanel> m_PropertiesLayer;
	Unique<SceneHierarchyPanel> m_SceneHierarchyLayer;
	Unique<MaterialEditorPanel> m_MaterialEditorLayer;

	Unique<UI::AboutWindow> m_AboutWindow;
	Unique<UI::PreferencesWindow> m_PreferencesWindow;
	Unique<UI::ProfilerWindow> m_ProfilerWindow;

	bool m_FontReloadRequested = false;

	VkFormat m_ImGuiColorFormat;
};

} // namespace Editor

#endif
