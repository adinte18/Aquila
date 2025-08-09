#ifndef EVENTREGISTRY_H
#define EVENTREGISTRY_H

namespace Engine{
    class Device;
    class Renderer;
    class EventBus;
    class AquilaScene;
    class EntityManager;
    class SceneManager;

    class EventRegistry {
    friend class Event;

    public:
        EventRegistry() = delete;

        void RegisterHandlers(Device* device, SceneManager* sceneManager, Renderer* renderer);
    };


}

#endif //EVENTREGISTRY_H
