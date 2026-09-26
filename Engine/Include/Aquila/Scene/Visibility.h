#ifndef AQUILA_SCENE_VISIBILITY_H
#define AQUILA_SCENE_VISIBILITY_H

#include <entt.h>

namespace Aquila::SceneManagement {

[[nodiscard]] bool is_visible_in_hierarchy(const entt::registry &registry, entt::entity entity);

}

#endif
