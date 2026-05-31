#include "Core/EditorConfig.h"
#include "Aquila/Foundation/Macros.h"

namespace Editor::Config {

static EditorPreferences s_Preferences;

EditorPreferences &GetPreferences() {
	return s_Preferences;
}

void EditorPreferences::LoadFromFile() {
	// TODO: not a priority right now, i will implement JSON loading later
	AQUILA_LOG_INFO("Loading editor preferences from: {}", preferenceFilePath);

	ResetToDefaults();

	AQUILA_LOG_INFO("Editor preferences loaded");
}

void EditorPreferences::SaveToFile() const {
	// TODO: not a priority right now, i will implement JSON saving later
	AQUILA_LOG_INFO("Saving editor preferences to: {}", preferenceFilePath);

	AQUILA_LOG_INFO("Editor preferences saved");
}

void EditorPreferences::ResetToDefaults() {
	window = WindowSettings{};
	fonts = FontSettings{};
	ui = UISettings{};
	currentTheme = Theme::Aquila2;

	showGrid = true;
	showGizmos = true;
	autoSave = false;
	autoSaveInterval = 300;

	cameraMoveSpeed = 5.0f;
	cameraRotateSpeed = 0.5f;
	invertMouseY = false;

	thumbnailSize = 64.0f;
	showFileExtensions = true;

	AQUILA_LOG_INFO("Editor preferences reset to defaults");
}

} // namespace Editor::Config
