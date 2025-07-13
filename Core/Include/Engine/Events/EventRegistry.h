#ifndef EVENTREGISTRY_H
#define EVENTREGISTRY_H

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

        void RegisterHandlers(Device* device, SceneManager* sceneManager, OffscreenRenderer* renderer);
    };


}

#endif //EVENTREGISTRY_H
