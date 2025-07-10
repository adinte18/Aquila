#ifndef SCENE_MANAGER_H
#define SCENE_MANAGER_H

#include <unordered_map>
#include "AquilaCore.h"
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

        void EnqueueScene(Unique<AquilaScene> scene, std::function<void(AquilaScene*)> onActivated = nullptr);

        void RequestSceneChange();
        void ProcessSceneChange();
    private:
        void ChangeScene(const UUID& handle);
        void RemoveScene(const UUID& handle);

        AquilaScene* m_ActiveScene = nullptr;
        std::unordered_map<UUID, Unique<AquilaScene>> m_Scenes;

        UUID m_PendingSceneChangeHandle;
        bool m_HasPendingSceneChange = false;

        Delegate<void(AquilaScene*)> m_OnSceneActivated = nullptr;
    };
}

#endif