#ifndef EDITOR_MENUBAR_H
#define EDITOR_MENUBAR_H

#include "Aquila/Core/Layer.h"
#include "Aquila/Events/Event.h"

namespace Aquila::Core {
class Application;
}

namespace Editor {
class MaterialEditorPanel;

namespace UI {
class AboutWindow;
class PreferencesWindow;
}

/**
 * @brief Main menubar for the editor
 * 
 * Provides access to all editor functions through a traditional
 * menu bar interface (File, Edit, View, Help, etc.)
 */
class Menubar : public Aquila::Core::Layer {
public:
	explicit Menubar(Aquila::Core::Application& app);
	~Menubar() override = default;

	void OnAttach() override;
	void OnDetach() override;
	void OnUpdate(f32 deltaTime) override;
	void OnImGuiRender() override;
	void OnEvent(Aquila::Events::Event& event) override;

	// Set references to other windows/panels
	void SetMaterialEditorPanel(MaterialEditorPanel* panel) { m_MaterialEditor = panel; }
	void SetAboutWindow(UI::AboutWindow* window) { m_AboutWindow = window; }
	void SetPreferencesWindow(UI::PreferencesWindow* window) { m_PreferencesWindow = window; }

private:
	void RenderFileMenu();
	void RenderEditMenu();
	void RenderViewMenu();
	void RenderHelpMenu();
	
	// Modals
	void RenderNewSceneModal();
	void RenderSaveSceneAsModal();
	
	// Actions
	void CreateNewScene(const std::string& sceneName, int templateType) const;
	void SaveCurrentScene() const;
	void SaveSceneAs(const std::string& filepath) const;
	void OpenSceneDialog();

	Aquila::Core::Application& m_App;
	
	// References to other windows/panels
	MaterialEditorPanel* m_MaterialEditor = nullptr;
	UI::AboutWindow* m_AboutWindow = nullptr;
	UI::PreferencesWindow* m_PreferencesWindow = nullptr;
	
	// Modal state
	bool m_ShowNewSceneModal = false;
	bool m_ShowSaveSceneAsModal = false;
	
	// Buffers for modals
	char m_NewSceneNameBuffer[128];
	char m_SaveSceneNameBuffer[128];
	char m_SaveScenePathBuffer[256];
};

} // namespace Editor

#endif // EDITOR_MENUBAR_H
