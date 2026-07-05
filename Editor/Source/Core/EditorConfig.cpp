#include "Core/EditorConfig.h"
#include "Aquila/Foundation/Macros.h"

namespace Editor::Config {

static EditorPreferences s_Preferences;

EditorPreferences &get_preferences() {
	return s_Preferences;
}

void EditorPreferences::load_from_file() {
	// TODO: not a priority right now, i will implement JSON loading later
	AQUILA_LOG_INFO("Loading editor preferences from: {}", preference_file_path);

	reset_to_defaults();

	AQUILA_LOG_INFO("Editor preferences loaded");
}

void EditorPreferences::save_to_file() const {
	// TODO: not a priority right now, i will implement JSON saving later
	AQUILA_LOG_INFO("Saving editor preferences to: {}", preference_file_path);

	AQUILA_LOG_INFO("Editor preferences saved");
}

void EditorPreferences::reset_to_defaults() {
	window = WindowSettings{};
	fonts = FontSettings{};
	ui = UISettings{};
	current_theme = Theme::Aquila2;

	show_grid = true;
	show_gizmos = true;
	auto_save = false;
	auto_save_interval = 300;

	camera_move_speed = 5.0f;
	camera_rotate_speed = 0.5f;
	invert_mouse_y = false;

	thumbnail_size = 64.0f;
	show_file_extensions = true;

	AQUILA_LOG_INFO("Editor preferences reset to defaults");
}

} // namespace Editor::Config
