#ifndef EDITOR_CONFIG_H
#define EDITOR_CONFIG_H

#include "Aquila/Platform/PrimitiveTypes.h"

namespace Editor::Config {

constexpr const char *EDITOR_NAME = "Aquila Editor";
constexpr const char *EDITOR_VERSION = AQUILA_VERSION_STRING;
constexpr uint32_t VERSION_MAJOR = AQUILA_VERSION_MAJOR;
constexpr uint32_t VERSION_MINOR = AQUILA_VERSION_MINOR;
constexpr uint32_t VERSION_PATCH = AQUILA_VERSION_PATCH;
constexpr const char *EDITOR_BUILD_DATE = __DATE__;
constexpr const char *EDITOR_TITLE = "Aquila Editor " AQUILA_VERSION_STRING " | " __DATE__;

struct WindowSettings {
	uint32 width = 1920;
	uint32 height = 1080;
	bool maximized = false;
	bool vsync = true;
};

struct FontSettings {
	std::string mainFontPath = ENGINE_RESOURCES_PATH "/Fonts/Roboto-VariableFont_wdth,wght.ttf";
	std::string iconsFontPath = ENGINE_RESOURCES_PATH "/Fonts/lucide.ttf";
	f32 defaultFontSize = 16.0f;
	int oversampleH = 3;
	int oversampleV = 1;
};

enum class Theme { Aquila, Aquila2, Dark, Light, Custom };

struct EditorPreferences {
	WindowSettings window;
	FontSettings fonts;
	Theme currentTheme = Theme::Aquila2;

	bool showGrid = true;
	bool showGizmos = true;
	bool autoSave = false;
	uint32 autoSaveInterval = 300; // seconds

	f32 cameraMoveSpeed = 5.0f;
	f32 cameraRotateSpeed = 0.5f;
	bool invertMouseY = false;

	f32 thumbnailSize = 64.0f;
	bool showFileExtensions = true;

	std::string preferenceFilePath = "editor_preferences.json";

	void LoadFromFile();
	void SaveToFile() const;
	void ResetToDefaults();
};

EditorPreferences &GetPreferences();

} // namespace Editor::Config

#endif // EDITOR_CONFIG_H
