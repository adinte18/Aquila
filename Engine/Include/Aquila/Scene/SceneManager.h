#ifndef AQUILA_SCENE_MANAGER_H
#define AQUILA_SCENE_MANAGER_H

#include "Aquila/Scene/Scene.h"

namespace Aquila::Assets {
class AssetManager; // Forward declaration
}

namespace Aquila::SceneManagement {

class SceneManager {
  public:
	SceneManager() = default;
	~SceneManager() = default;

	Scene *GetActiveScene() const;
	Scene *GetScene(const Utils::UUID &handle) const;
	Scene *GetSceneByName(const std::string &name) const;
	bool HasScene() const;
	bool HasScene(const Utils::UUID &handle) const;

	// Scene Creation & Loading
	Scene *CreateScene(const std::string &name);
	Scene *LoadScene(const std::string &filepath, Assets::AssetManager &assetManager);
	Scene *LoadSceneAsync(const std::string &filepath, Assets::AssetManager &assetManager,
						  const Delegate<void(Scene *)> &onLoaded = nullptr);

	// Scene Management
	void EnqueueScene(Unique<Scene> scene, const Delegate<void(Scene *)> &onActivated = nullptr);
	void ChangeScene(const Utils::UUID &handle);
	void RemoveScene(const Utils::UUID &handle);
	void UnloadScene(const Utils::UUID &handle);
	void UnloadAllScenesExceptActive();

	// Scene Operations
	bool SaveScene(const Utils::UUID &handle, const std::string &filepath);
	bool SaveActiveScene(const std::string &filepath);
	Scene *DuplicateScene(const Utils::UUID &handle, Assets::AssetManager &assetManager,
						  const std::string &newName = "");

	// Scene Change Requests
	void RequestSceneChange(const Utils::UUID &handle);
	void RequestSceneChange(); // Uses last enqueued scene
	bool HasPendingSceneChange() const;
	void ProcessSceneChange();

	void ActivateScene(const Utils::UUID &handle);
	void ActivateScene(Scene *scene);

	// Loading without activation
	Scene *LoadSceneInBackground(const std::string &filepath, Assets::AssetManager &assetManager);

	// Query inactive scenes
	std::vector<Scene *> GetInactiveScenes() const;
	bool IsSceneActive(const Utils::UUID &handle) const;

	// Utilities
	std::vector<Scene *> GetAllScenes() const;
	std::vector<std::string> GetAllSceneNames() const;
	size_t GetSceneCount() const { return m_Scenes.size(); }

	// Callbacks
	void SetOnSceneActivated(const Delegate<void(Scene *)> &callback) { m_OnSceneActivated = callback; }
	void SetOnSceneUnloaded(const Delegate<void(Scene *)> &callback) { m_OnSceneUnloaded = callback; }

  private:
	std::unordered_map<Utils::UUID, Unique<Scene>> m_Scenes;
	Scene *m_ActiveScene = nullptr;
	// Scene change handling
	Utils::UUID m_PendingSceneChangeHandle = Utils::UUID::Null();
	bool m_HasPendingSceneChange = false;

	// Callbacks
	Delegate<void(Scene *)> m_OnSceneActivated;
	Delegate<void(Scene *)> m_OnSceneUnloaded;

	// Helper methods
	std::string ExtractSceneName(const std::string &filepath);
	bool ValidateSceneFile(const std::string &filepath);
};

} // namespace Aquila::SceneManagement

#endif