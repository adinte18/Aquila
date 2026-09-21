#ifndef AQUILA_SCENE_DEFAULT_SCENES_H
#define AQUILA_SCENE_DEFAULT_SCENES_H

#include "Aquila/Foundation/PrimitiveTypes.h"
#include "Aquila/Scene/Entity.h"
#include "Aquila/Scene/Scene.h"

namespace Aquila::GFX {
class GfxContext;
}

namespace Aquila::SceneManagement {

Entity spawn_default_camera(Scene &scene, F32 aspect_ratio);

void populate_demo_scene(Scene &scene, GFX::GfxContext &ctx, F32 aspect_ratio);

}

#endif
