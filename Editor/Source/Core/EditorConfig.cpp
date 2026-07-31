#include "Core/EditorConfig.h"
#include "Aquila/Foundation/Macros.h"
#include "Aquila/Platform/Filesystem/VirtualFileSystem.h"

#include "json.hpp"

namespace Editor::Config {

static EditorPreferences s_Preferences;

EditorPreferences &get_preferences() {
	return s_Preferences;
}

void EditorPreferences::load_from_file() {
	auto *vfs = Aquila::Platform::Filesystem::VirtualFileSystem::get();

	reset_to_defaults();

	if (vfs == nullptr || !vfs->exists(preference_file_path)) {
		AQUILA_LOG_INFO("No editor preferences at {}, using defaults", preference_file_path);
		return;
	}

	const std::string text = vfs->read_text_file(preference_file_path);
	nlohmann::json root = nlohmann::json::parse(text, nullptr, false);
	if (root.is_discarded()) {
		AQUILA_LOG_WARNING("Editor preferences at {} are malformed, using defaults", preference_file_path);
		return;
	}

	if (root.contains("window")) {
		const auto &w = root["window"];
		window.width = w.value("width", window.width);
		window.height = w.value("height", window.height);
		window.maximized = w.value("maximized", window.maximized);
		window.vsync = w.value("vsync", window.vsync);
	}
	if (root.contains("fonts")) {
		fonts.size = root["fonts"].value("size", fonts.size);
		fonts.main_family = root["fonts"].value("main_family", fonts.main_family);
		fonts.mono_family = root["fonts"].value("mono_family", fonts.mono_family);
	}

	ui_scale = root.value("ui_scale", ui_scale);
	current_theme = static_cast<Theme>(root.value("theme", static_cast<int>(current_theme)));
	show_grid = root.value("show_grid", show_grid);
	show_gizmos = root.value("show_gizmos", show_gizmos);
	auto_save = root.value("auto_save", auto_save);
	auto_save_interval = root.value("auto_save_interval", auto_save_interval);
	camera_move_speed = root.value("camera_move_speed", camera_move_speed);
	camera_rotate_speed = root.value("camera_rotate_speed", camera_rotate_speed);
	invert_mouse_y = root.value("invert_mouse_y", invert_mouse_y);
	thumbnail_size = root.value("thumbnail_size", thumbnail_size);
	show_file_extensions = root.value("show_file_extensions", show_file_extensions);

	AQUILA_LOG_INFO("Editor preferences loaded from {}", preference_file_path);
}

void EditorPreferences::save_to_file() const {
	nlohmann::ordered_json root;
	root["window"] = {
		{ "width", window.width },
		{ "height", window.height },
		{ "maximized", window.maximized },
		{ "vsync", window.vsync },
	};
	root["fonts"] = {
		{ "main_family", fonts.main_family },
		{ "mono_family", fonts.mono_family },
		{ "size", fonts.size },
	};
	root["ui_scale"] = ui_scale;
	root["theme"] = static_cast<int>(current_theme);
	root["show_grid"] = show_grid;
	root["show_gizmos"] = show_gizmos;
	root["auto_save"] = auto_save;
	root["auto_save_interval"] = auto_save_interval;
	root["camera_move_speed"] = camera_move_speed;
	root["camera_rotate_speed"] = camera_rotate_speed;
	root["invert_mouse_y"] = invert_mouse_y;
	root["thumbnail_size"] = thumbnail_size;
	root["show_file_extensions"] = show_file_extensions;

	auto *vfs = Aquila::Platform::Filesystem::VirtualFileSystem::get();
	if (vfs != nullptr && vfs->write_text_file(preference_file_path, root.dump(4))) {
		AQUILA_LOG_INFO("Editor preferences saved to {}", preference_file_path);
	} else {
		AQUILA_LOG_ERROR("Failed to save editor preferences to {}", preference_file_path);
	}
}

void EditorPreferences::reset_to_defaults() {
	window = WindowSettings{};
	fonts = FontSettings{};
	ui = UISettings{};
	current_theme = Theme::Aquila2;

	ui_scale = 1.0f;

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
