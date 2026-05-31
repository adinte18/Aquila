#ifndef EDITOR_CONFIG_H
#define EDITOR_CONFIG_H

#include "Aquila/Foundation/PrimitiveTypes.h"

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
	std::string regularPath = "/resources/Engine/Fonts/Lexend/Lexend-Regular.ttf";
	std::string thinPath    = "/resources/Engine/Fonts/Lexend/Lexend-Thin.ttf";
	std::string mediumPath  = "/resources/Engine/Fonts/Lexend/Lexend-Medium.ttf";
	std::string boldPath    = "/resources/Engine/Fonts/Lexend/Lexend-Bold.ttf";
	f32 size = 16.f;
};

struct UISettings {
	std::string resourcesPath = "/resources";
	std::string layoutPath    = "/resources/Engine/UI/widget_test.aqlayout";
	std::string stylePath     = "/resources/Engine/UI/widget_test.aqstyle";
};

enum class Theme { Aquila, Aquila2, Dark, Light, Custom };

struct EditorPreferences {
	WindowSettings window;
	FontSettings fonts;
	UISettings ui;
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
