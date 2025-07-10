#include "Scene/SceneManager.h"

namespace Engine {
    /**
     * @brief Retrieves the currently active scene.
     * 
     * This function returns a pointer to the active scene managed by the SceneManager.
     * If no scene is active, it will return a nullptr.
     * 
     * @return AquilaScene* Pointer to the active scene, or nullptr if no scene is active.
     */
    AquilaScene* SceneManager::GetActiveScene() const {
        return m_ActiveScene;
    }

    /**
     * @brief Changes the active scene to the one specified by name.
     * 
     * If the scene is found, it will be set as the active scene and its OnStart method will be called.
     * If the scene is not found, an exception will be thrown. @todo : maybe i should not throw an exception here?
     * 
     * @param name The name of the scene to change to.
     */
    void SceneManager::ChangeScene(const UUID& handle) {
        auto it = m_Scenes.find(handle);
        if (it != m_Scenes.end()) {
            m_ActiveScene = it->second.get();
            m_ActiveScene->OnStart();
        } 
    }
    /**
     * @brief Checks if there is an active scene in the SceneManager.
     * 
     * @return true if there is at least one scene, false otherwise.
     */
    bool SceneManager::HasScene() const {
        return !m_Scenes.empty();
    }

    /**
     * @brief Removes a scene from the SceneManager by its name.
     * 
     * If the scene is currently active, it will be set to nullptr.
     * 
     * @param name The name of the scene to be removed.
     */
    void SceneManager::RemoveScene(const UUID& handle) {
        auto it = m_Scenes.find(handle);
        if (it != m_Scenes.end()) {
            if (m_ActiveScene == it->second.get()) {
                m_ActiveScene = nullptr;
            }
            m_Scenes.erase(it);
        }
    }

    /**
     * @brief Enqueues a scene to the SceneManager.
     * 
     * @param name The name of the scene to be enqueued.
     * @param scene The scene object to be enqueued, wrapped in a Unique pointer.
     * @param onActivated A callback function that will be called when the scene is activated.
     */
    void SceneManager::EnqueueScene(Unique<AquilaScene> scene, std::function<void(AquilaScene*)> onActivated) {
        m_Scenes[scene->GetHandle()] = std::move(scene);

        // callback for when the scene is activated
        m_OnSceneActivated = std::move(onActivated);
    }

    /**
     * @brief Requests a scene change by setting the pending scene name.
     * 
     * @param name The name of the scene to change to.
     */
    void SceneManager::RequestSceneChange() {
        // m_PendingSceneChange = name;

        // take the last enqueued scene
        if (!m_Scenes.empty()) {
            m_PendingSceneChangeHandle = m_Scenes.begin()->first;
        }

        m_HasPendingSceneChange = true;
    }

    /**
     * @brief Checks if there is a pending scene change.
     * 
     * @return true if there is a pending scene change, false otherwise.
     */
    bool SceneManager::HasPendingSceneChange() const {
        return m_HasPendingSceneChange;
    }

    /**
     * @brief Processes the pending scene change if there is one.
     * 
     * This function changes the active scene to the pending scene and clears the pending change.
     * If an activation callback is set, it will be called with the newly activated scene.
     */
    void SceneManager::ProcessSceneChange() {
        if (!m_HasPendingSceneChange) return;

        ChangeScene(m_PendingSceneChangeHandle);
        m_PendingSceneChangeHandle = UUID::Null();
        m_HasPendingSceneChange = false;

        if (m_OnSceneActivated && m_ActiveScene) {
            m_OnSceneActivated(m_ActiveScene);
            m_OnSceneActivated = nullptr;
        }
    }
}