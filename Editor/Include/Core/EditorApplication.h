#pragma once

#include "Aquila/Application/ApplicationNew.h"
#include "Aquila/UI/Core/TextureCache.h"

namespace Aquila::UI::Core {
class View;
}

namespace Editor {

class ViewportPanel;
class HierarchyPanel;
class InspectorPanel;
class ConsolePanel;

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

	Unique<Aquila::UI::Core::TextureCache> m_TextureCache;

	Unique<ViewportPanel> m_ViewportPanel;
	Unique<HierarchyPanel> m_HierarchyPanel;
	Unique<InspectorPanel> m_InspectorPanel;
	Unique<ConsolePanel> m_ConsolePanel;
};

} // namespace Editor
