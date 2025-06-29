//
// Created by alexa on 20/10/2024.
//

#ifndef ENTITY_H
#define ENTITY_H

#include <string>

#include "entt.h"

namespace Engine {
    class Entity {
    public:
        Entity(entt::registry& registry, entt::entity handle)
            : registry(registry), entityHandle(handle) {

            name = "Entity" + std::to_string(static_cast<uint32_t>(handle));
        }

        template<typename T, typename... Args>
        T& AddComponent(Args&&... args) {
            return registry.emplace<T>(entityHandle, std::forward<Args>(args)...);
        }

        template<typename T>
        T& GetComponent() {
            return registry.get<T>(entityHandle);
        }

        template<typename T>
        bool HasComponent() const {
            return registry.all_of<T>(entityHandle);
        }

        template<typename T>
        void RemoveComponent() const {
            registry.remove<T>(entityHandle);
        }

        entt::entity GetHandle() const {
            return entityHandle;
        }

        [[nodiscard]] std::string GetName() const {
            return name;
        }

        void SetName(const std::string& name) {
            this->name = name;
        }

    private:
        std::string name;
        entt::registry& registry;
        entt::entity entityHandle;
    };
}


#endif //ENTITY_H
