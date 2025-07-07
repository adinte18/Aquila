#ifndef SCENE_MANAGER_H
#define SCENE_MANAGER_H

#include <string>
#include <unordered_map>
#include "Scene/Scene.h"

namespace Engine {
    class SceneManager {
    public:
        SceneManager() = default;
        ~SceneManager() = default;

        // rule of five
        SceneManager(const SceneManager&) = delete;
        SceneManager& operator=(const SceneManager&) = delete;
        SceneManager(SceneManager&&) = delete;
        SceneManager& operator=(SceneManager&&) = delete;

        [[nodiscard]] AquilaScene* GetActiveScene() const;
        [[nodiscard]] bool HasScene() const;
        [[nodiscard]] bool HasPendingSceneChange() const;

        void EnqueueScene(const std::string& name, Unique<AquilaScene> scene);

        void RequestSceneChange(const std::string& name);
        void ProcessSceneChange();
    private:
        void ChangeScene(const std::string& name);
        void RemoveScene(const std::string& name);

        AquilaScene* m_ActiveScene = nullptr;
        std::unordered_map<std::string, Unique<AquilaScene>> m_Scenes;

        std::string m_PendingSceneChange;
        bool m_HasPendingSceneChange = false;
    };
}

#endif