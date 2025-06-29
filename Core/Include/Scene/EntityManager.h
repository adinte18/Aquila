#ifndef AQUILA_ENTITY_MNG_H
#define AQUILA_ENTITY_MNG_H

#include "Entity.h"
#include "Scene.h"

namespace Engine {
    class EntityManager{
        public :

        EntityManager(AquilaScene* scene) : m_Scene(scene) { m_Registry = {}; }
        ~EntityManager();

        entt::registry& GetRegistry();

        template<typename...Components>
        AqEntity GetFirstEntityWith(){
            AQUILA_CORE_ASSERT(m_Scene);
            auto view = m_Registry.view<Components...>();

            if (view.begin() == view.end()) {
                return AqEntity{};
            }

            entt::entity firstEntity = *view.begin();
            return AqEntity{firstEntity, m_Scene};
        }

        template<typename...Components>
        std::vector<AqEntity> GetAllWith(){
            AQUILA_CORE_ASSERT(m_Scene);
            auto view = m_Registry.view<Components...>();

            std::vector<AqEntity> result{};
            result.reserve(std::distance(view.begin(), view.end()));

            for (auto entity : view){
                result.emplace(result.begin(),AqEntity{entity, m_Scene});
            }

            return result;
        }


        AqEntity AddEntity();
        AqEntity AddEntity(const std::string& name);
        AqEntity GetEntityByUUID(UUID id);
        
        void DeleteEntity();
        void Clear();

        bool EntityExists(UUID id);
        void KillEntity(AqEntity entity);
        bool IsEntityValid(AqEntity entity) const;



        private:
            AquilaScene* m_Scene;
            entt::registry m_Registry;
    };
}

#endif