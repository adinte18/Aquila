#ifndef AQUILA_SCENEGRAPH_H
#define AQUILA_SCENEGRAPH_H

#include "AquilaCore.h"
#include "entt.h"

namespace Engine {
    // struct SceneNode {
    //     entt::entity Entity = entt::null;
    //     entt::entity Parent = entt::null;
    //     std::vector<entt::entity> Children;

    //     static void OnConstruct(entt::registry &registry, entt::entity entity);
    //     static void OnUpdate(entt::registry& registry);
    //     static void OnDestroy(entt::registry& registry, entt::entity entity);

    //     static void Traverse(entt::registry& registry, entt::entity entity, int depth);
    //     static void AddChild(entt::registry& registry, entt::entity parent, entt::entity child);

    //     static void RemoveChild(entt::registry& registry, entt::entity parent, entt::entity child);
    //     static void RemoveAllChildren(entt::registry& registry, entt::entity parent);
    // };

    class SceneGraph {
        private:
        static void OnConstruct(entt::registry &registry, entt::entity entity);
        static void OnUpdate(entt::registry& registry);
        static void OnDestroy(entt::registry& registry, entt::entity entity);


        public:
        SceneGraph() {};
        ~SceneGraph();

        SceneGraph(const SceneGraph&) = delete;
        SceneGraph(const SceneGraph&&) = delete;

        // construct 
        void Construct(entt::registry& registry);
        
        // update transforms etc
        void Update(entt::registry& registry);

        void Traverse(entt::registry& registry, entt::entity entity, int depth);

        void AddChild(entt::registry& registry, entt::entity parent, entt::entity child);
        void AttachTo(entt::registry& registry, entt::entity parent, entt::entity node);

        bool IsDescendant(entt::registry& registry, entt::entity potentialParent, entt::entity entityToCheck);

        void RemoveChild(entt::registry& registry, entt::entity parent, entt::entity child);
        void RemoveAllChildren(entt::registry& registry, entt::entity parent);

    };
}

#endif