#include "Scene/SceneManager.h"

namespace Engine {
    AquilaScene* SceneManager::GetActiveScene() const {
        return m_ActiveScene;
    }

    void SceneManager::ChangeScene(const std::string& name) {
        auto it = m_Scenes.find(name);
        if (it != m_Scenes.end()) {
            m_ActiveScene = it->second.get();
            m_ActiveScene->OnStart();
        } else {
            throw std::runtime_error("Scene not found: " + name);
        }
    }

    bool SceneManager::HasScene() const {
        return !m_Scenes.empty();
    }

    void SceneManager::RemoveScene(const std::string& name) {
        auto it = m_Scenes.find(name);
        if (it != m_Scenes.end()) {
            if (m_ActiveScene == it->second.get()) {
                m_ActiveScene = nullptr;
            }
            m_Scenes.erase(it);
        } else {
            throw std::runtime_error("Scene not found: " + name);
        }
    }

    void SceneManager::EnqueueScene(const std::string& name, Unique<AquilaScene> scene) {
        m_Scenes[name] = std::move(scene);
    }

    void SceneManager::RequestSceneChange(const std::string& name) {
        m_PendingSceneChange = name;
        m_HasPendingSceneChange = true;
    }

    bool SceneManager::HasPendingSceneChange() const {
        return m_HasPendingSceneChange;
    }

    void SceneManager::ProcessSceneChange() {
        if (m_HasPendingSceneChange) {
            ChangeScene(m_PendingSceneChange);
            m_PendingSceneChange.clear();
            m_HasPendingSceneChange = false;
        }
    }
}