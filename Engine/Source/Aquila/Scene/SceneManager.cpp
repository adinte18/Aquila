#include "Aquila/Scene/SceneManager.h"

#include "Aquila/Platform/Filesystem/VirtualFileSystem.h"

namespace Aquila::SceneManagement {

// TODO : should this class even exist? this can be done in the asset manager class but I guess its cleaner this way

// Scene Retrieval
Scene *SceneManager::GetActiveScene() const {
	return m_ActiveScene;
}

Scene *SceneManager::GetScene(const Utils::UUID &handle) const {
	if (auto it = m_Scenes.find(handle); it != m_Scenes.end()) {
		return it->second.get();
	}
	return nullptr;
}

Scene *SceneManager::GetSceneByName(const std::string &name) const {
	for (const auto &scene : m_Scenes | std::views::values) {
		if (scene->GetSceneName() == name) {
			return scene.get();
		}
	}
	return nullptr;
}

bool SceneManager::HasScene() const {
	return !m_Scenes.empty();
}

bool SceneManager::HasScene(const Utils::UUID &handle) const {
	return m_Scenes.contains(handle);
}

// Scene Creation & Loading

Scene *SceneManager::CreateScene(const std::string &name) {
	auto scene = CreateUnique<Scene>(name);

	auto handle = scene->GetHandle();
	m_Scenes[handle] = std::move(scene);

	AQUILA_LOG_INFO("Created new scene: {}", name);
	return m_Scenes[handle].get();
}

Scene *SceneManager::LoadScene(const std::string &filepath, Assets::AssetManager &assetManager) {
	if (!ValidateSceneFile(filepath)) {
		AQUILA_LOG_ERROR("Failed to validate scene file: {}", filepath);
		return nullptr;
	}

	auto scene = CreateUnique<Scene>();
	auto handle = scene->GetHandle();

	// Deserialize scene data
	if (!scene->Deserialize(filepath, assetManager)) {
		AQUILA_LOG_ERROR("Failed to deserialize scene from: {}", filepath);
		return nullptr;
	}

	Scene *scenePtr = scene.get();

	EnqueueScene(std::move(scene));
	ActivateScene(scenePtr);

	AQUILA_LOG_INFO("Loaded scene: {} from {}", scenePtr->GetSceneName(), filepath);
	return m_Scenes[handle].get();
}

Scene *SceneManager::LoadSceneAsync(const std::string &filepath, Assets::AssetManager &assetManager,
									const Delegate<void(Scene *)> &onLoaded) {
	// For now, just load synchronously lol
	Scene *scene = LoadScene(filepath, assetManager);

	if ((scene != nullptr) && onLoaded) {
		onLoaded(scene);
	}

	return scene;
}

// Scene Management

void SceneManager::EnqueueScene(Unique<Scene> scene, const Delegate<void(Scene *)> &onActivated) {
	m_Scenes[scene->GetHandle()] = std::move(scene);
	m_OnSceneActivated = std::move(onActivated);
}

void SceneManager::ChangeScene(const Utils::UUID &handle) {
	if (auto it = m_Scenes.find(handle); it != m_Scenes.end()) {
		if (m_ActiveScene && m_OnSceneUnloaded) {
			m_OnSceneUnloaded(m_ActiveScene);
		}

		m_ActiveScene = GetScene(handle);

		AQUILA_LOG_INFO("Changed to scene: {}", m_ActiveScene->GetSceneName());
	} else {
		AQUILA_LOG_ERROR("Scene not found with handle: {}", handle.ToString());
	}
}

void SceneManager::RemoveScene(const Utils::UUID &handle) {
	if (const auto it = m_Scenes.find(handle); it != m_Scenes.end()) {
		if (m_ActiveScene == it->second.get()) {
			m_ActiveScene = nullptr;
		}

		AQUILA_LOG_INFO("Removed scene: {}", it->second->GetSceneName());
		m_Scenes.erase(it);
	}
}

void SceneManager::UnloadScene(const Utils::UUID &handle) {
	if (const auto it = m_Scenes.find(handle); it != m_Scenes.end()) {
		Scene *scene = it->second.get();

		if (m_ActiveScene == scene) {
			AQUILA_LOG_WARNING("Cannot unload active scene: {}", scene->GetSceneName());
			return;
		}

		if (m_OnSceneUnloaded) {
			m_OnSceneUnloaded(scene);
		}

		AQUILA_LOG_INFO("Unloaded scene: {}", scene->GetSceneName());
		m_Scenes.erase(it);
	}
}

void SceneManager::UnloadAllScenesExceptActive() {
	std::vector<Utils::UUID> toRemove;

	for (const auto &[handle, scene] : m_Scenes) {
		if (scene.get() != m_ActiveScene) {
			toRemove.push_back(handle);
		}
	}

	for (const auto &handle : toRemove) {
		UnloadScene(handle);
	}

	AQUILA_LOG_INFO("Unloaded {} non-active scenes", toRemove.size());
}

// Scene Activation

void SceneManager::ActivateScene(const Utils::UUID &handle) {
	if (!HasScene(handle)) {
		AQUILA_LOG_ERROR("Cannot activate scene - not found: {}", handle.ToString());
		return;
	}

	RequestSceneChange(handle);
	ProcessSceneChange();
}

void SceneManager::ActivateScene(Scene *scene) {
	if (scene == nullptr) {
		AQUILA_LOG_ERROR("Cannot activate null scene");
		return;
	}

	ActivateScene(scene->GetHandle());
}

Scene *SceneManager::LoadSceneInBackground(const std::string &filepath, Assets::AssetManager &assetManager) {
	if (!ValidateSceneFile(filepath)) {
		AQUILA_LOG_ERROR("Failed to validate scene file: {}", filepath);
		return nullptr;
	}

	// Create new scene
	auto scene = CreateUnique<Scene>();

	// Deserialize scene data
	if (!scene->Deserialize(filepath, assetManager)) {
		AQUILA_LOG_ERROR("Failed to deserialize scene from: {}", filepath);
		return nullptr;
	}

	auto handle = scene->GetHandle();
	m_Scenes[handle] = std::move(scene);
	AQUILA_LOG_INFO("Loaded scene (inactive): {} from {}", m_Scenes[handle]->GetSceneName(), filepath);
	return m_Scenes[handle].get();
}

std::vector<Scene *> SceneManager::GetInactiveScenes() const {
	std::vector<Scene *> inactive;
	inactive.reserve(m_Scenes.size() > 0 ? m_Scenes.size() - 1 : 0);

	for (const auto &[handle, scene] : m_Scenes) {
		if (scene.get() != m_ActiveScene) {
			inactive.push_back(scene.get());
		}
	}

	return inactive;
}

bool SceneManager::IsSceneActive(const Utils::UUID &handle) const {
	if (Scene *scene = GetScene(handle)) {
		return scene == m_ActiveScene;
	}
	return false;
}

// Scene Operations

bool SceneManager::SaveScene(const Utils::UUID &handle, const std::string &filepath) {
	Scene *scene = GetScene(handle);
	if (!scene) {
		AQUILA_LOG_ERROR("Scene not found with handle: {}", handle.ToString());
		return false;
	}

	if (!scene->Serialize(filepath)) {
		AQUILA_LOG_ERROR("Failed to serialize scene: {} to {}", scene->GetSceneName(), filepath);
		return false;
	}

	AQUILA_LOG_INFO("Saved scene: {} to {}", scene->GetSceneName(), filepath);
	return true;
}

bool SceneManager::SaveActiveScene(const std::string &filepath) {
	if (!m_ActiveScene) {
		AQUILA_LOG_ERROR("No active scene to save");
		return false;
	}

	return SaveScene(m_ActiveScene->GetHandle(), filepath);
}

Scene *SceneManager::DuplicateScene(const Utils::UUID &handle, Assets::AssetManager &assetManager,
									const std::string &newName) {
	Scene *sourceScene = GetScene(handle);
	if (!sourceScene) {
		AQUILA_LOG_ERROR("Source scene not found with handle: {}", handle.ToString());
		return nullptr;
	}

	// Create new scene with new name
	std::string duplicateName = newName.empty() ? sourceScene->GetSceneName() + " (Copy)" : newName;

	auto duplicateScene = CreateUnique<Scene>(duplicateName);

	// Serialize source to temporary string
	std::string tempPath = "temp://scene_duplicate.aqscene";
	if (!sourceScene->Serialize(tempPath)) {
		AQUILA_LOG_ERROR("Failed to serialize source scene for duplication");
		return nullptr;
	}

	// Deserialize into duplicate
	if (!duplicateScene->Deserialize(tempPath, assetManager)) {
		AQUILA_LOG_ERROR("Failed to deserialize into duplicate scene");
		return nullptr;
	}

	Scene *duplicatePtr = duplicateScene.get();
	m_Scenes[duplicateScene->GetHandle()] = std::move(duplicateScene);

	AQUILA_LOG_INFO("Duplicated scene: {} -> {}", sourceScene->GetSceneName(), duplicateName);
	return duplicatePtr;
}

// Scene Change Requests

void SceneManager::RequestSceneChange(const Utils::UUID &handle) {
	if (!HasScene(handle)) {
		AQUILA_LOG_ERROR("Cannot request scene change - scene not found: {}", handle.ToString());
		return;
	}

	m_PendingSceneChangeHandle = handle;
	m_HasPendingSceneChange = true;
}

void SceneManager::RequestSceneChange() {
	if (m_Scenes.empty()) {
		AQUILA_LOG_WARNING("Cannot request scene change - no scenes loaded");
		return;
	}

	// Take the last enqueued scene
	m_PendingSceneChangeHandle = m_Scenes.begin()->first;
	m_HasPendingSceneChange = true;
}

bool SceneManager::HasPendingSceneChange() const {
	return m_HasPendingSceneChange;
}

void SceneManager::ProcessSceneChange() {
	if (!m_HasPendingSceneChange) {
		return;
	}

	ChangeScene(m_PendingSceneChangeHandle);
	m_PendingSceneChangeHandle = Utils::UUID::Null();
	m_HasPendingSceneChange = false;

	if (m_OnSceneActivated && m_ActiveScene) {
		m_OnSceneActivated(m_ActiveScene);
	}

	AQUILA_LOG_DEBUG("Current active scene: {}", GetActiveScene()->GetSceneName());
}

// Utilities

std::vector<Scene *> SceneManager::GetAllScenes() const {
	std::vector<Scene *> scenes;
	scenes.reserve(m_Scenes.size());

	for (const auto &[handle, scene] : m_Scenes) {
		scenes.push_back(scene.get());
	}

	return scenes;
}

std::vector<std::string> SceneManager::GetAllSceneNames() const {
	std::vector<std::string> names;
	names.reserve(m_Scenes.size());

	for (const auto &[handle, scene] : m_Scenes) {
		names.push_back(scene->GetSceneName());
	}

	return names;
}

// Helper Methods

std::string SceneManager::ExtractSceneName(const std::string &filepath) {
	// Handle VFS paths like "assets::/path/to/myscene.aqscene" or "/path/to/myscene.aqscene"
	std::string path = filepath;

	// Find the last slash (works for both / and \)
	size_t lastSlash = path.find_last_of("/\\");
	if (lastSlash != std::string::npos) {
		path = path.substr(lastSlash + 1);
	}

	// Remove file extension
	size_t lastDot = path.find_last_of('.');
	if (lastDot != std::string::npos) {
		path = path.substr(0, lastDot);
	}

	return path;
}

bool SceneManager::ValidateSceneFile(const std::string &filepath) {
	auto vfs = Platform::Filesystem::VirtualFileSystem::Get();

	// Check if file exists in VFS
	if (!vfs->Exists(filepath)) {
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
