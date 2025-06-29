#include "Scene/SceneGraph.h"
#include "Scene/Components/SceneNodeComponent.h"

namespace Engine {
    SceneGraph::~SceneGraph() = default;

    void SceneGraph::Construct(entt::registry& registry) {
        // Connect callbacks to SceneNodeComponent events
        registry.on_construct<SceneNodeComponent>().connect<&SceneGraph::OnConstruct>();
        registry.on_update<SceneNodeComponent>().connect<&SceneGraph::OnUpdate>();
        registry.on_destroy<SceneNodeComponent>().connect<&SceneGraph::OnDestroy>();
    }

    void SceneGraph::OnConstruct(entt::registry& registry, entt::entity entity) {
        auto& node = registry.get_or_emplace<SceneNodeComponent>(entity);
        node.Entity = entity;

        if (node.Parent != entt::null) {
            auto& parentNode = registry.get_or_emplace<SceneNodeComponent>(node.Parent);

            auto& siblings = parentNode.Children;
            if (std::find(siblings.begin(), siblings.end(), entity) == siblings.end()) {
                siblings.push_back(entity);
            }
        }
    }

    void SceneGraph::OnUpdate(entt::registry& registry) {
        registry.view<SceneNodeComponent>().each([&](entt::entity entity, SceneNodeComponent& node) {
            if (node.Parent == entt::null) {
                //
            }
        });
    }

    void SceneGraph::OnDestroy(entt::registry& registry, entt::entity entity) {
        auto* node = registry.try_get<SceneNodeComponent>(entity);
        if (!node)
            return;

        for (auto child : node->Children) {
            if (registry.valid(child)) {
                registry.destroy(child);
            }
        }
        node->Children.clear();

        if (node->Parent != entt::null) {
            auto* parentNode = registry.try_get<SceneNodeComponent>(node->Parent);
            if (parentNode) {
                auto& siblings = parentNode->Children;
                siblings.erase(std::remove(siblings.begin(), siblings.end(), entity), siblings.end());
            }
        }
    }

    void SceneGraph::AddChild(entt::registry& registry, entt::entity parent, entt::entity child) {
        if (!registry.valid(parent) || !registry.valid(child)) return;

        auto* parentNode = registry.try_get<SceneNodeComponent>(parent);
        auto* childNode = registry.try_get<SceneNodeComponent>(child);
        if (!parentNode || !childNode) return;

        childNode->Parent = parentNode->Entity;

        if (std::find(parentNode->Children.begin(), parentNode->Children.end(), child) == parentNode->Children.end()) {
            parentNode->Children.push_back(child);
        }
    }

    void SceneGraph::AttachTo(entt::registry& registry, entt::entity parent, entt::entity node){
        if (!registry.valid(parent) || !registry.valid(node)) return;

        auto* parentNode = registry.try_get<SceneNodeComponent>(parent);
        auto* nodeToAttach = registry.try_get<SceneNodeComponent>(node);
        if (!parentNode || !nodeToAttach) return;

        // if has parent -> detach from parent
        if (nodeToAttach->Parent != entt::null) {
            RemoveChild(registry, nodeToAttach->Parent, nodeToAttach->Entity);
        }

        // reattach to another parent
        nodeToAttach->Parent = parentNode->Entity;
        parentNode->Children.push_back(nodeToAttach->Entity);
    }

    bool SceneGraph::IsDescendant(entt::registry& registry, entt::entity potentialParent, entt::entity entityToCheck) {
        auto* node = registry.try_get<SceneNodeComponent>(potentialParent);
        if (!node) return false;

        for (const auto& child : node->Children) {
            if (child == entityToCheck) return true;
            if (IsDescendant(registry, child, entityToCheck)) return true;
        }
        return false;
    }

    void SceneGraph::RemoveChild(entt::registry& registry, entt::entity parent, entt::entity child) {
        if (!registry.valid(parent) || !registry.valid(child)) return;

        auto* parentNode = registry.try_get<SceneNodeComponent>(parent);
        auto* childNode = registry.try_get<SceneNodeComponent>(child);
        if (!parentNode || !childNode) return;

        auto& siblings = parentNode->Children;
        siblings.erase(std::remove(siblings.begin(), siblings.end(), child), siblings.end());

        childNode->Parent = entt::null;
    }

    void SceneGraph::RemoveAllChildren(entt::registry& registry, entt::entity parent) {
        if (!registry.valid(parent)) return;

        auto* parentNode = registry.try_get<SceneNodeComponent>(parent);
        if (!parentNode) return;

        auto childrenCopy = parentNode->Children;

        for (auto& child : childrenCopy) {
            RemoveAllChildren(registry, child);

            if (registry.valid(child))
                registry.destroy(child);
        }

        parentNode->Children.clear();
    }


}