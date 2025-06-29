#include "Scene/EntityManager.h"
#include "Scene/Scene.h"

namespace Engine {
    EntityManager::~EntityManager() = default;


    entt::registry& EntityManager::GetRegistry(){
        AQUILA_CORE_ASSERT(m_Scene && "Scene should not be nullptr");

        return m_Registry;
    }

    AqEntity EntityManager::AddEntity(){
        AQUILA_CORE_ASSERT(m_Scene && "Scene should not be nullptr");

        auto entity = m_Registry.create();

        m_Registry.emplace<MetadataComponent>(entity, MetadataComponent {UUID::Generate(), "Unnamed entity", true});
        m_Registry.emplace<SceneNodeComponent>(entity);
        return AqEntity(entity, m_Scene);
    }

    AqEntity EntityManager::AddEntity(const std::string& name) {
        AQUILA_CORE_ASSERT(m_Scene && "Scene should not be nullptr");

        auto entity = m_Registry.create();

        m_Registry.emplace<MetadataComponent>(entity, MetadataComponent {UUID::Generate(), name, true});
        m_Registry.emplace<SceneNodeComponent>(entity);
        return AqEntity(entity, m_Scene);
    }

    void EntityManager::Clear()
    {
        for(auto [entity] : m_Registry.storage<entt::entity>().each())
        {
            m_Registry.destroy(entity);
        }

        m_Registry.clear();
    }

    /**
     * @brief Get entity by his UUID
     * 
     * @param uuid 
     * @return AqEntity 
     */
    AqEntity EntityManager::GetEntityByUUID(UUID uuid){
        AQUILA_CORE_ASSERT(m_Scene && "Scene should not be nullptr");

        auto view = m_Registry.view<MetadataComponent>(); // get all entities with metadata component

        for (auto& entity : view){
            auto idComponent = m_Registry.get<MetadataComponent>(entity);
            if (idComponent.ID == uuid){
                AQUILA_CORE_ASSERT(m_Scene);
                return AqEntity(entity, m_Scene);
            }  
        }

        return AqEntity();
    }

    /**
     * @brief Verify if entity exists in the scene
     * 
     * @param uuid 
     * @return true if exsists
     * @return false if not
     */
    bool EntityManager::EntityExists(UUID uuid){
        AQUILA_CORE_ASSERT(m_Scene && "Scene should not be nullptr");

        auto view = m_Registry.view<MetadataComponent>(); // get all entities with metadata component

        for (auto& entity : view){
            auto idComponent = m_Registry.get<MetadataComponent>(entity);
            if (idComponent.ID == uuid){
                return true;
            }  
        }

        return false;
    }

    /**
     * @brief Kill a given entity
     * 
     * @param entity 
     */
    void EntityManager::KillEntity(AqEntity entity) {
        AQUILA_CORE_ASSERT(m_Scene && "Scene should not be nullptr");
        m_Registry.destroy(entity.GetHandle());
    }

    /**
     * @brief Verify if the entity is valid
     * 
     * @param entity 
     * @return true if valid
     * @return false if not
     */
    bool EntityManager::IsEntityValid(AqEntity entity) const {
        AQUILA_CORE_ASSERT(m_Scene && "Scene should not be nullptr");
        return m_Registry.valid(entity.GetHandle());
    }
}