#ifndef SCENE_NODE_COMPONENT_H
#define SCENE_NODE_COMPONENT_H

#include "entt.h"

struct SceneNodeComponent {
    entt::entity Entity = entt::null;
    entt::entity Parent = entt::null;
    std::vector<entt::entity> Children;
};


#endif