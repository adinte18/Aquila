#include "ECS/Scene.h"
#include <memory>

namespace Engine {
    Scene::Scene() {
        m_HasEnvMap = false;
        m_Camera = Engine::EditorCamera{};

        // root scene node
        m_RootNode = std::make_shared<SceneNode>();
        m_RootNode->entity = nullptr;
        m_RootNode->parent.reset();
    }

    Ref<Entity> Scene::CreateEntity() {
        entt::entity newEntityID;
        if (!recycledEntityIDs.empty()) {
            newEntityID = recycledEntityIDs.front();
            recycledEntityIDs.pop();
            
            return std::make_shared<Entity>(m_Registry,m_Registry.create(newEntityID));
        }

        newEntityID = m_Registry.create();
        return std::make_shared<Entity>(m_Registry, newEntityID);
    }

    Ref<Scene::SceneNode> Scene::CreateNode(Ref<SceneNode> parent) {
        // Create and link SceneNode
        auto node = std::make_shared<SceneNode>();
        auto parentNode = parent == nullptr ? m_RootNode : parent; 
        node->parent = parentNode;

        parentNode->children.push_back(node);

        return node;
    }

    Ref<Scene::SceneNode> Scene::GetSelectedNode(const Ref<Scene::SceneNode>& node) {
        if (!node) return nullptr;

        if (node->selected)
            return node;

        for (const auto& child : node->children) {
            auto selected = GetSelectedNode(child);
            if (selected)
                return selected;
        }

        return nullptr;
    }

    void Scene::DeselectAllNodes(const Ref<Engine::Scene::SceneNode>& node) {
        if (!node) return;

        node->selected = false;
        for (const auto& child : node->children) {
            DeselectAllNodes(child);
        }
    }



    const void Scene::TraverseNode(const Ref<SceneNode>& node){
        if (!node) return;

        // manipulate transform, etc.
        auto& entity = node->entity;
        if (entity->HasComponent<Transform>()){
            node->localTransform = entity->GetComponent<Transform>();
        }

        node->UpdateWorldTransform(); // updates transform if has children
        
        // continue traversing children
        for (const auto& child : node->children) {
            TraverseNode(child);
        }
    }

    const void Scene::TraverseGraph(){
        TraverseNode(m_RootNode);
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

    Engine::EditorCamera & Scene::GetActiveCamera() {
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
