//
// Created by alexa on 20/10/2024.
//

#ifndef AQUILA_ENTITY_H
#define AQUILA_ENTITY_H

#include "AquilaCore.h"

#include "Scene/Components/MetadataComponent.h"
#include "Scene/Components/CameraComponent.h"
#include "Scene/Components/MaterialComponent.h"
#include "Scene/Components/LightComponent.h"
#include "Scene/Components/MeshComponent.h"
#include "Scene/Components/TransformComponent.h"
#include "Scene/Components/SceneNodeComponent.h"
#include "Scene/Scene.h"



namespace Engine {
    class EntityManager;

    class AqEntity {
    public:
        AqEntity(){};
        AqEntity(entt::entity handle, AquilaScene* scene) : m_Scene(scene), m_EntityHandle(handle) {};

        template<typename T, typename... Args>
        T& AddComponent(Args&&... args) {
            return m_Scene->GetRegistry().emplace<T>(m_EntityHandle, std::forward<Args>(args)...);
        }

        template <typename T>
        T& GetComponent() {
            return m_Scene->GetRegistry().get<T>(m_EntityHandle);
        }

        template <typename T>
        const T& GetComponent() const {
            AQUILA_CORE_ASSERT(m_Scene);
            return m_Scene->GetRegistry().get<T>(m_EntityHandle);
        }

        template<typename T>
        T* TryGetComponent() {
            if (HasComponent<T>()) {
                return &GetComponent<T>();
            }
            return nullptr;
        }

        template<typename T>
        const T* TryGetComponent() const {
            if (HasComponent<T>()) {
                return &GetComponent<T>();
            }
            return nullptr;
        }        

        template <typename T>
        void TryRemoveComponent() const
        {
            if(HasComponent<T>()) RemoveComponent<T>();
        }

        template<typename T>
        bool HasComponent() const {
            AQUILA_CORE_ASSERT(m_Scene);
            return m_Scene->GetRegistry().all_of<T>(m_EntityHandle);
        }

        template<typename T>
        void RemoveComponent() const {
            AQUILA_CORE_ASSERT(m_Scene);
            m_Scene->GetRegistry().remove<T>(m_EntityHandle);
        }

        entt::entity GetHandle() const { return m_EntityHandle; }
        AquilaScene* GetScene() const { AQUILA_CORE_ASSERT(m_Scene); return m_Scene; }

        const UUID& GetUUID() const;
        const std::string& GetName() const;

        void Kill();

        bool IsValid() const;



    private:
        entt::entity m_EntityHandle = entt::null;
        AquilaScene* m_Scene;

        friend class EntityManager;
    };
}


#endif //ENTITY_H
