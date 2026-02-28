#ifndef SCENE_NODE_COMPONENT_H
#define SCENE_NODE_COMPONENT_H

#include "Aquila/Scene/Entity.h"
namespace Aquila::SceneManagement::Components {
struct SceneNodeComponent {
	Entity Ent;
	Entity Parent;
	std::vector<Entity> Children;

	SceneNodeComponent() = default;

	SceneNodeComponent(const SceneNodeComponent &other) = default;
	SceneNodeComponent &operator=(const SceneNodeComponent &other) = default;

	SceneNodeComponent(SceneNodeComponent &&other) noexcept = default;
	SceneNodeComponent &operator=(SceneNodeComponent &&other) noexcept = default;

	~SceneNodeComponent() = default;
};
} // namespace Aquila::SceneManagement::Components
#endif