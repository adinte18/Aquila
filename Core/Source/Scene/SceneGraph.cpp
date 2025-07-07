#include "Scene/SceneGraph.h"
#include "Scene/Components/SceneNodeComponent.h"

namespace Engine {
    SceneGraph::~SceneGraph() = default;

    /**
     * @brief Constructs the scene graph by connecting callbacks to SceneNodeComponent events.
     * 
     * This function is called to set up the scene graph, allowing it to respond to
     * construction, update, and destruction of SceneNodeComponents in the entt registry.
     * 
     * @param registry The entt registry containing the scene graph.
     */
    void SceneGraph::Construct(entt::registry& registry) {
        // Connect callbacks to SceneNodeComponent events
        registry.on_construct<SceneNodeComponent>().connect<&SceneGraph::OnConstruct>();
        registry.on_update<SceneNodeComponent>().connect<&SceneGraph::OnUpdate>();
        registry.on_destroy<SceneNodeComponent>().connect<&SceneGraph::OnDestroy>();
    }

    /**
     * @brief Constructs a scene node and sets its parent-child relationship in the scene graph.
     * 
     * This function is called when a SceneNodeComponent is constructed in the registry.
     * It initializes the SceneNodeComponent and establishes the parent-child relationship
     * if a parent is specified.
     * 
     * @param registry The entt registry containing the scene graph.
     * @param entity The entity being constructed.
     */
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

    /**
     * @brief Updates the scene graph by iterating through all SceneNodeComponents.
     * 
     * This function can be used to perform operations on the scene graph, such as updating transforms,
     * checking for changes, or any other logic that needs to be applied to the nodes.
     * 
     * @param registry The entt registry containing the scene graph.
     */
    void SceneGraph::OnUpdate(entt::registry& registry) {
        registry.view<SceneNodeComponent>().each([&](entt::entity entity, SceneNodeComponent& node) {
            if (node.Parent == entt::null) {
                //didn't found a usage for this, but it might be useful in the future
            }
        });
    }

    /**
     * @brief Destroys a scene node and its children in the scene graph.
     * 
     * @param registry The entt registry containing the scene graph.
     * @param entity The entity to be destroyed.
     */
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

    /**
     * @brief Adds a child entity to a parent in the scene graph.
     * 
     * @param registry The entt registry containing the scene graph.
     * @param parent The parent entity to which the child will be added.
     * @param child The child entity to be added.
     */
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

    /**
     * @brief Attaches a node to a parent in the scene graph.
     * 
     * @param registry The entt registry containing the scene graph.
     * @param parent The parent entity to which the node will be attached.
     * @param node The node entity to be attached.
     */
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

    /**
     * @brief Checks if a given entity is a descendant of a potential parent in the scene graph.
     * 
     * @param registry The entt registry containing the scene graph.
     * @param potentialParent The entity that is being checked as a potential parent.
     * @param entityToCheck The entity that is being checked for being a descendant.
     * @return true if entityToCheck is a descendant of potentialParent, false otherwise.
     */
    bool SceneGraph::IsDescendant(entt::registry& registry, entt::entity potentialParent, entt::entity entityToCheck) {
        auto* node = registry.try_get<SceneNodeComponent>(potentialParent);
        if (!node) return false;

        for (const auto& child : node->Children) {
            if (child == entityToCheck) return true;
            if (IsDescendant(registry, child, entityToCheck)) return true;
        }
        return false;
    }

    /**
     * @brief Removes a child entity from its parent in the scene graph.
     * 
     * @param registry The entt registry containing the scene graph.
     * @param parent The parent entity from which the child will be removed.
     * @param child The child entity to be removed.
     */
    void SceneGraph::RemoveChild(entt::registry& registry, entt::entity parent, entt::entity child) {
        if (!registry.valid(parent) || !registry.valid(child)) return;

        auto* parentNode = registry.try_get<SceneNodeComponent>(parent);
        auto* childNode = registry.try_get<SceneNodeComponent>(child);
        if (!parentNode || !childNode) return;

        auto& siblings = parentNode->Children;
        siblings.erase(std::remove(siblings.begin(), siblings.end(), child), siblings.end());

        childNode->Parent = entt::null;
    }

    /**
     * @brief Recursively removes all children of a given parent entity.
     * 
     * @param registry The entt registry containing the scene graph.
     * @param parent The parent entity whose children will be removed.
     */
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