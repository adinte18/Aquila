#include "ECS/Scene.h"

#include <Engine/Buffer.h>
#include <Engine/Model.h>
#include <Engine/OffscreenRenderer.h>

#include "Components.h"

namespace ECS {
    Scene::Scene() {
        m_Camera = Engine::Camera{};
    }

    // Create a new entity in the scene using the shared registry
    std::shared_ptr<Entity> Scene::CreateEntity() {
        return std::make_shared<Entity>(m_Registry, entt::entity{m_Registry.create()});
    }

    // Destroy an entity in the scene
    void Scene::DestroyEntity(const Entity &entity) {
        m_Registry.destroy(entity.GetHandle());
    }

    // Clear all entities from the scene
    void Scene::Clear() {
        m_Registry.clear();
    }

    // Access the registry directly if needed
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
