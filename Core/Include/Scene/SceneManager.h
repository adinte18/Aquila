#ifndef SCENE_MANAGER_H
#define SCENE_MANAGER_H

#include "AquilaCore.h"
#include "Scene/Scene.h"
#include "Utilities/Singleton.h"

namespace Engine {
class SceneManager {
public:
  SceneManager() = default;
  ~SceneManager() = default;

  AQUILA_NONCOPYABLE(SceneManager);
  AQUILA_NONMOVEABLE(SceneManager);

  [[nodiscard]] AquilaScene *GetActiveScene() const;
  [[nodiscard]] bool HasScene() const;
  [[nodiscard]] bool HasPendingSceneChange() const;

  void EnqueueScene(Unique<AquilaScene> scene,
                    Delegate<void(AquilaScene *)> onActivated = nullptr);

  void RequestSceneChange();
  void ProcessSceneChange();

private:
  void ChangeScene(const Utility::UUID &handle);
  void RemoveScene(const Utility::UUID &handle);

  AquilaScene *m_ActiveScene = nullptr;
  std::unordered_map<Utility::UUID, Unique<AquilaScene>> m_Scenes;

  Utility::UUID m_PendingSceneChangeHandle;
  bool m_HasPendingSceneChange = false;

  Delegate<void(AquilaScene *)> m_OnSceneActivated = nullptr;
};
} // namespace Engine

#endif