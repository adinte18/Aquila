#ifndef EVENTREGISTRY_H
#define EVENTREGISTRY_H

#include "Engine/Events/EventBus.h"
#include "Engine/Device.h"

#include "Engine/OffscreenRenderer.h"
#include "Scene/Scene.h"
#include "Scene/Entity.h"
#include "Scene/EntityManager.h"
#include "Scene/SceneGraph.h"
#include "Scene/SceneManager.h"

namespace Engine{
    class EventRegistry {
    public:
        EventRegistry() = delete;

        void RegisterHandlers(Device* device, Ref<SceneManager>& sceneManager, OffscreenRenderer* renderer);
    };


}

#endif //EVENTREGISTRY_H
