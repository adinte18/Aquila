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
	void OnInit() override;
	void OnShutdown() override;
	void OnPreRender(f32 deltaTime) override;
	void OnEvent(Aquila::Application::Events::Event &event) override;
	void OnResize(uint32 width, uint32 height) override;

  private:
	void SetupScene();
	void SetupEditorUI();
	void WireMenubar(Aquila::UI::Core::View *layoutRoot);
	void OpenUIInspectorWindow();
	void OpenWidgetGalleryWindow();
	void StartPick();

	void WireDockSpace(Aquila::UI::Core::DockSpace *dockSpace, GLFWwindow *sourceNative);
	void HandleTearOff(GLFWwindow *sourceNative, Unique<Aquila::UI::Core::View> content, std::string title,
					   vec2 sourceLocal);
	void PreviewDockTargets(GLFWwindow *sourceNative, vec2 sourceLocal);
	void ClearDockTargetPreviews();
	Aquila::UI::Core::DockSpace *FindDockTargetAtScreen(vec2 screenPos, GLFWwindow *exclude, vec2 &outLocal);
	void CloseFloatingWindow(GLFWwindow *native);
	void SpawnFloatingPanel(Unique<Aquila::UI::Core::View> panelSubtree, std::string title, vec2 screenPos);
	void OnFloatingClosed(FloatingPanelWindow *panel);
	void DockBackToCenter(Unique<Aquila::UI::Core::View> content, const std::string &title);

	Unique<Aquila::UI::Core::TextureCache> m_TextureCache;

	Unique<ViewportPanel> m_ViewportPanel;
	Unique<HierarchyPanel> m_HierarchyPanel;
	Unique<InspectorPanel> m_InspectorPanel;
	Unique<ConsolePanel> m_ConsolePanel;
	Unique<UIDebugPanel> m_UIDebugPanel;
	Unique<UIDebugWindow> m_UIDebugWindow;
	Unique<WidgetGalleryWindow> m_WidgetGalleryWindow;

	PickerOverlay *m_Picker = nullptr;
	bool m_PickMode = false;

	Aquila::UI::Core::DockSpace *m_DockSpace = nullptr;

	struct FloatingEntry {
		Unique<FloatingPanelWindow> panel;
		Aquila::Application::RenderWindow *window = nullptr;
	};
	std::vector<FloatingEntry> m_FloatingPanels;
};

} // namespace Editor
