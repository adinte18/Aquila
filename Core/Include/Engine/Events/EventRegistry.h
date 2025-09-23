#ifndef EVENTREGISTRY_H
#define EVENTREGISTRY_H

#include "AquilaCore.h"

namespace Engine {
class Device;
class Renderer;
class EventBus;
class AquilaScene;
class EntityManager;
class SceneManager;

class EventRegistry {
public:
  EventRegistry() = delete;

  static void RegisterHandlers(Ref<Device> device,
                               Ref<SceneManager> sceneManager,
                               Ref<Renderer> renderer);

  static void UnregisterHandlers();

private:
  static bool s_handlersRegistered;
};

} // namespace Engine

#endif // EVENTREGISTRY_H
