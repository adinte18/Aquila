#include "ECS/Scene.h"

namespace ECS {
    Scene::Scene() {
        m_Camera = Engine::Camera{};
    }

    std::shared_ptr<Entity> Scene::CreateEntity() {
        entt::entity newEntityID;
        if (!recycledEntityIDs.empty()) {
            newEntityID = recycledEntityIDs.front();
            recycledEntityIDs.pop();
            return std::make_shared<Entity>(m_Registry,m_Registry.create(newEntityID));
        }

        newEntityID = m_Registry.create();
        return std::make_shared<Entity>(m_Registry, newEntityID);
    }

    void Scene::DestroyEntity(const Entity &entity) {
        if (m_Registry.valid(entity.GetHandle())) {
            recycledEntityIDs.push(entity.GetHandle());
            m_Registry.destroy(entity.GetHandle());
        }
    }

    void Scene::Clear() {
        m_Registry.clear();
    }

    entt::registry& Scene::GetRegistry() {
        return m_Registry;
    }

    Engine::Camera & Scene::GetActiveCamera() {
        return m_Camera;
    }

    std::vector<Entity>& Scene::GetEntitesToDelete() {
        return m_QueuedForDestruction;
    }

    void Scene::QueueForDestruction(entt::entity entity) {
        m_QueuedForDestruction.emplace_back(m_Registry, entity);
    }

    void Scene::ClearQueue() {
        m_QueuedForDestruction.clear();
    }
}
