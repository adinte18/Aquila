#include "Aquila/Scene/SceneManager.h"

#include "Aquila/Platform/Filesystem/VirtualFileSystem.h"

namespace Aquila::SceneManagement {

// TODO : should this class even exist? this can be done in the asset manager class but I guess its cleaner this way

// Scene Retrieval
Scene *SceneManager::get_active_scene() const {
	return m_active_scene;
}

Scene *SceneManager::get_scene(const Foundation::UUID &handle) const {
	if (auto it = m_scenes.find(handle); it != m_scenes.end()) {
		return it->second.get();
	}
	return nullptr;
}

Scene *SceneManager::get_scene_by_name(const std::string &name) const {
	for (const auto &scene : m_scenes | std::views::values) {
		if (scene->get_scene_name() == name) {
			return scene.get();
		}
	}
	return nullptr;
}

bool SceneManager::has_scene() const {
	return !m_scenes.empty();
}

bool SceneManager::has_scene(const Foundation::UUID &handle) const {
	return m_scenes.contains(handle);
}

// Scene Creation & Loading

Scene *SceneManager::create_scene(const std::string &name) {
	auto scene = std::make_unique<Scene>(name);

	auto handle = scene->get_handle();
	m_scenes[handle] = std::move(scene);

	AQUILA_LOG_INFO("Created new scene: {}", name);
	return m_scenes[handle].get();
}

Scene *SceneManager::load_scene(const std::string &filepath, Assets::AssetManager &asset_manager) {
	if (!validate_scene_file(filepath)) {
		AQUILA_LOG_ERROR("Failed to validate scene file: {}", filepath);
		return nullptr;
	}

	auto scene = std::make_unique<Scene>();
	auto handle = scene->get_handle();

	// Deserialize scene data
	if (!scene->deserialize(filepath, asset_manager)) {
		AQUILA_LOG_ERROR("Failed to deserialize scene from: {}", filepath);
		return nullptr;
	}

	Scene *scene_ptr = scene.get();

	enqueue_scene(std::move(scene));
	activate_scene(scene_ptr);

	AQUILA_LOG_INFO("Loaded scene: {} from {}", scene_ptr->get_scene_name(), filepath);
	return m_scenes[handle].get();
}

Scene *SceneManager::load_scene_async(const std::string &filepath, Assets::AssetManager &asset_manager,
									  const Delegate<void(Scene *)> &on_loaded) {
	// For now, just load synchronously lol
	Scene *scene = load_scene(filepath, asset_manager);

	if ((scene != nullptr) && on_loaded) {
		on_loaded(scene);
	}

	return scene;
}

// Scene Management

void SceneManager::enqueue_scene(Unique<Scene> scene, const Delegate<void(Scene *)> &on_activated) {
	m_scenes[scene->get_handle()] = std::move(scene);
	m_on_scene_activated = std::move(on_activated);
}

void SceneManager::change_scene(const Foundation::UUID &handle) {
	if (auto it = m_scenes.find(handle); it != m_scenes.end()) {
		if (m_active_scene && m_on_scene_unloaded) {
			m_on_scene_unloaded(m_active_scene);
		}

		m_active_scene = get_scene(handle);

		AQUILA_LOG_INFO("Changed to scene: {}", m_active_scene->get_scene_name());
	} else {
		AQUILA_LOG_ERROR("Scene not found with handle: {}", handle.to_string());
	}
}

void SceneManager::remove_scene(const Foundation::UUID &handle) {
	if (const auto it = m_scenes.find(handle); it != m_scenes.end()) {
		if (m_active_scene == it->second.get()) {
			m_active_scene = nullptr;
		}

		AQUILA_LOG_INFO("Removed scene: {}", it->second->get_scene_name());
		m_scenes.erase(it);
	}
}

void SceneManager::unload_scene(const Foundation::UUID &handle) {
	if (const auto it = m_scenes.find(handle); it != m_scenes.end()) {
		Scene *scene = it->second.get();

		if (m_active_scene == scene) {
			AQUILA_LOG_WARNING("Cannot unload active scene: {}", scene->get_scene_name());
			return;
		}

		if (m_on_scene_unloaded) {
			m_on_scene_unloaded(scene);
		}

		AQUILA_LOG_INFO("Unloaded scene: {}", scene->get_scene_name());
		m_scenes.erase(it);
	}
}

void SceneManager::unload_all_scenes_except_active() {
	std::vector<Foundation::UUID> to_remove;

	for (const auto &[handle, scene] : m_scenes) {
		if (scene.get() != m_active_scene) {
			to_remove.push_back(handle);
		}
	}

	for (const auto &handle : to_remove) {
		unload_scene(handle);
	}

	AQUILA_LOG_INFO("Unloaded {} non-active scenes", to_remove.size());
}

// Scene Activation

void SceneManager::activate_scene(const Foundation::UUID &handle) {
	if (!has_scene(handle)) {
		AQUILA_LOG_ERROR("Cannot activate scene - not found: {}", handle.to_string());
		return;
	}

	request_scene_change(handle);
	process_scene_change();
}

void SceneManager::activate_scene(Scene *scene) {
	if (scene == nullptr) {
		AQUILA_LOG_ERROR("Cannot activate null scene");
		return;
	}

	activate_scene(scene->get_handle());
}

Scene *SceneManager::load_scene_in_background(const std::string &filepath, Assets::AssetManager &asset_manager) {
	if (!validate_scene_file(filepath)) {
		AQUILA_LOG_ERROR("Failed to validate scene file: {}", filepath);
		return nullptr;
	}

	// Create new scene
	auto scene = std::make_unique<Scene>();

	// Deserialize scene data
	if (!scene->deserialize(filepath, asset_manager)) {
		AQUILA_LOG_ERROR("Failed to deserialize scene from: {}", filepath);
		return nullptr;
	}

	auto handle = scene->get_handle();
	m_scenes[handle] = std::move(scene);
	AQUILA_LOG_INFO("Loaded scene (inactive): {} from {}", m_scenes[handle]->get_scene_name(), filepath);
	return m_scenes[handle].get();
}

std::vector<Scene *> SceneManager::get_inactive_scenes() const {
	std::vector<Scene *> inactive;
	inactive.reserve(m_scenes.size() > 0 ? m_scenes.size() - 1 : 0);

	for (const auto &[handle, scene] : m_scenes) {
		if (scene.get() != m_active_scene) {
			inactive.push_back(scene.get());
		}
	}

	return inactive;
}

bool SceneManager::is_scene_active(const Foundation::UUID &handle) const {
	if (Scene *scene = get_scene(handle)) {
		return scene == m_active_scene;
	}
	return false;
}

// Scene Operations

bool SceneManager::save_scene(const Foundation::UUID &handle, const std::string &filepath) {
	Scene *scene = get_scene(handle);
	if (!scene) {
		AQUILA_LOG_ERROR("Scene not found with handle: {}", handle.to_string());
		return false;
	}

	if (!scene->serialize(filepath)) {
		AQUILA_LOG_ERROR("Failed to serialize scene: {} to {}", scene->get_scene_name(), filepath);
		return false;
	}

	AQUILA_LOG_INFO("Saved scene: {} to {}", scene->get_scene_name(), filepath);
	return true;
}

bool SceneManager::save_active_scene(const std::string &filepath) {
	if (!m_active_scene) {
		AQUILA_LOG_ERROR("No active scene to save");
		return false;
	}

	return save_scene(m_active_scene->get_handle(), filepath);
}

Scene *SceneManager::duplicate_scene(const Foundation::UUID &handle, Assets::AssetManager &asset_manager,
									 const std::string &new_name) {
	Scene *source_scene = get_scene(handle);
	if (!source_scene) {
		AQUILA_LOG_ERROR("Source scene not found with handle: {}", handle.to_string());
		return nullptr;
	}

	// Create new scene with new name
	std::string duplicate_name = new_name.empty() ? source_scene->get_scene_name() + " (Copy)" : new_name;

	auto duplicate_scene = std::make_unique<Scene>(duplicate_name);

	// Serialize source to temporary string
	std::string temp_path = "temp://scene_duplicate.aqscene";
	if (!source_scene->serialize(temp_path)) {
		AQUILA_LOG_ERROR("Failed to serialize source scene for duplication");
		return nullptr;
	}

	// Deserialize into duplicate
	if (!duplicate_scene->deserialize(temp_path, asset_manager)) {
		AQUILA_LOG_ERROR("Failed to deserialize into duplicate scene");
		return nullptr;
	}

	Scene *duplicate_ptr = duplicate_scene.get();
	m_scenes[duplicate_scene->get_handle()] = std::move(duplicate_scene);

	AQUILA_LOG_INFO("Duplicated scene: {} -> {}", source_scene->get_scene_name(), duplicate_name);
	return duplicate_ptr;
}

// Scene Change Requests

void SceneManager::request_scene_change(const Foundation::UUID &handle) {
	if (!has_scene(handle)) {
		AQUILA_LOG_ERROR("Cannot request scene change - scene not found: {}", handle.to_string());
		return;
	}

	m_pending_scene_change_handle = handle;
	m_has_pending_scene_change = true;
}

void SceneManager::request_scene_change() {
	if (m_scenes.empty()) {
		AQUILA_LOG_WARNING("Cannot request scene change - no scenes loaded");
		return;
	}

	// Take the last enqueued scene
	m_pending_scene_change_handle = m_scenes.begin()->first;
	m_has_pending_scene_change = true;
}

bool SceneManager::has_pending_scene_change() const {
	return m_has_pending_scene_change;
}

void SceneManager::process_scene_change() {
	if (!m_has_pending_scene_change) {
		return;
	}

	change_scene(m_pending_scene_change_handle);
	m_pending_scene_change_handle = Foundation::UUID::null();
	m_has_pending_scene_change = false;

	if (m_on_scene_activated && m_active_scene) {
		m_on_scene_activated(m_active_scene);
	}

	AQUILA_LOG_DEBUG("Current active scene: {}", get_active_scene()->get_scene_name());
}

// Utilities

std::vector<Scene *> SceneManager::get_all_scenes() const {
	std::vector<Scene *> scenes;
	scenes.reserve(m_scenes.size());

	for (const auto &[handle, scene] : m_scenes) {
		scenes.push_back(scene.get());
	}

	return scenes;
}

std::vector<std::string> SceneManager::get_all_scene_names() const {
	std::vector<std::string> names;
	names.reserve(m_scenes.size());

	for (const auto &[handle, scene] : m_scenes) {
		names.push_back(scene->get_scene_name());
	}

	return names;
}

// Helper Methods

std::string SceneManager::extract_scene_name(const std::string &filepath) {
	// Handle VFS paths like "assets::/path/to/myscene.aqscene" or "/path/to/myscene.aqscene"
	std::string path = filepath;

	// Find the last slash (works for both / and \)
	size_t last_slash = path.find_last_of("/\\");
	if (last_slash != std::string::npos) {
		path = path.substr(last_slash + 1);
	}

	// Remove file extension
	size_t last_dot = path.find_last_of('.');
	if (last_dot != std::string::npos) {
		path = path.substr(0, last_dot);
	}

	return path;
}

bool SceneManager::validate_scene_file(const std::string &filepath) {
	auto vfs = Platform::Filesystem::VirtualFileSystem::get();

	// Check if file exists in VFS
	if (!vfs->exists(filepath)) {
		AQUILA_LOG_ERROR("Scene file does not exist: {}", filepath);
		return false;
	}

	// Check file extension
	if (!filepath.ends_with(".aqscene")) {
		AQUILA_LOG_ERROR("Invalid scene file extension (expected .aqscene): {}", filepath);
		return false;
	}

	return true;
}

} // namespace Aquila::SceneManagement
