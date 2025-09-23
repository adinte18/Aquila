#ifndef CORE_H
#define CORE_H

#include "AquilaCore.h"

#include "Engine/EditorCamera.h"
#include "Engine/Window.h"

#include "Engine/Renderer/Device.h"
#include "Engine/Renderer/Renderer.h"

#include "Engine/Events/EventBus.h"
#include "Engine/Events/EventRegistry.h"

#include "Platform/Filesystem/NativeFileSystem.h"
#include "Platform/Filesystem/VirtualFileSystem.h"

#include "Scene/SceneManager.h"

#include "Utilities/Singleton.h"
namespace Engine {
class Controller : public Utility::Singleton<Controller> {
  friend class Utility::Singleton<Controller>;

public:
  void OnStart();
  void OnUpdate();
  void OnEnd();

  void InvalidatePasses();
  void HandleSceneChange();

  f32 GetDeltaTime();

  Device &GetDevice() const { return *m_Device; }

  Window &GetWindow() const { return *m_Window; }

  Renderer &GetRenderer() const { return *m_Renderer; }

  SceneManager &GetSceneManager() const { return *m_SceneManager; }

  Utility::Stopwatch &GetStopwatch() { return *m_Stopwatch; }

  EditorCamera &GetCamera() { return *m_EditorCamera; }

private:
  Controller();
  ~Controller();

  AQUILA_NONMOVEABLE(Controller);
  AQUILA_NONCOPYABLE(Controller);

  Unique<Utility::Stopwatch> m_Stopwatch;
  f32 m_DeltaTime = 0.0f;

  Unique<EditorCamera> m_EditorCamera;

  Unique<Window> m_Window;
  Ref<Device> m_Device;
  Ref<Renderer> m_Renderer;
  Ref<SceneManager> m_SceneManager;
  Unique<EventRegistry> m_EventRegistry;

  VFS::VirtualFileSystem *m_VFS;
};

} // namespace Engine

#endif // CORE_H
