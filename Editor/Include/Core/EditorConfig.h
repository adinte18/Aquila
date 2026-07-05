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
	Uint32 width = 1920;
	Uint32 height = 1080;
	bool maximized = false;
	bool vsync = true;
};

struct FontSettings {
	std::string regular_path = "/resources/Engine/Fonts/Lexend/Lexend-Regular.ttf";
	std::string thin_path = "/resources/Engine/Fonts/Lexend/Lexend-Thin.ttf";
	std::string medium_path = "/resources/Engine/Fonts/Lexend/Lexend-Medium.ttf";
	std::string bold_path = "/resources/Engine/Fonts/Lexend/Lexend-Bold.ttf";
	F32 size = 16.F;
};

struct UISettings {
	std::string resources_path = "/resources";
	std::string layout_path = "/resources/Engine/UI/editor.aqlayout";
	std::string style_path = "/resources/Engine/UI/widget_test.aqstyle";
};

enum class Theme { Aquila, Aquila2, Dark, Light, Custom };

struct EditorPreferences {
	WindowSettings window;
	FontSettings fonts;
	UISettings ui;
	Theme current_theme = Theme::Aquila2;

	bool show_grid = true;
	bool show_gizmos = true;
	bool auto_save = false;
	Uint32 auto_save_interval = 300; // seconds

	F32 camera_move_speed = 5.0f;
	F32 camera_rotate_speed = 0.5f;
	bool invert_mouse_y = false;

	F32 thumbnail_size = 64.0f;
	bool show_file_extensions = true;

	std::string preference_file_path = "editor_preferences.json";

	void load_from_file();
	void save_to_file() const;
	void reset_to_defaults();
};

EditorPreferences &get_preferences();

} // namespace Editor::Config

#endif // EDITOR_CONFIG_H
