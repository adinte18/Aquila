#ifndef EVENTREGISTRY_H
#define EVENTREGISTRY_H

#include "Engine/Device.h"

namespace Engine{
    class Device;
    class OffscreenRenderer;
    class EventBus;
    class AquilaScene;
    class EntityManager;
    class SceneManager;

    class EventRegistry {
    friend class Event;

    public:
        EventRegistry() = delete;

        void RegisterHandlers(Device* device, Ref<SceneManager>& sceneManager, OffscreenRenderer* renderer);
    };


}

#endif //EVENTREGISTRY_H
