#ifndef EDITOR_PREFERENCES_WINDOW_H
#define EDITOR_PREFERENCES_WINDOW_H

#include "Aquila/Core/Layer.h"
#include "Core/EditorConfig.h"

namespace Aquila::Core {
class Application;
}

namespace Editor::UI {

/**
 * @brief Editor preferences window
 *
 * Provides a UI for configuring all editor settings including
 * themes, fonts, viewport controls, and other preferences
 */
class PreferencesWindow : public Aquila::Core::Layer {
  public:
	explicit PreferencesWindow(Aquila::Core::Application &app);
	~PreferencesWindow() override = default;

	void OnImGuiRender() override;

	void Show() { m_IsOpen = true; }
	void Hide() { m_IsOpen = false; }
	bool IsOpen() const { return m_IsOpen; }
	void SetOpen(bool open) { m_IsOpen = open; }

	void SetDeferredOperationCallback(Delegate<void()> callback) { m_DeferredOperationCallback = callback; }

  private:
	void RenderGeneralSettings();
	void RenderAppearanceSettings();
	void RenderViewportSettings();
	void RenderAssetBrowserSettings();
	void RenderShortcutsSettings();

	void ApplyChanges();
	void ResetToDefaults();

	Aquila::Core::Application &m_App;
	bool m_IsOpen = false;
	Delegate<void()> m_DeferredOperationCallback;

	Config::EditorPreferences m_WorkingPrefs;
	bool m_HasUnsavedChanges = false;
};

} // namespace Editor::UI

#endif // EDITOR_PREFERENCES_WINDOW_H
