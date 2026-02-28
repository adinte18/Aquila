#ifndef AQUILA_SHARED_CONSTANTS_H
#define AQUILA_SHARED_CONSTANTS_H

#include "Aquila/Platform/PrimitiveTypes.h"

namespace Aquila::SharedConstants {

constexpr int MAX_FRAMES_IN_FLIGHT = 2;

constexpr f64 FIXED_TIMESTEP = 1.0 / 60.0;
constexpr f64 MAX_DELTA_TIME = 0.25;

constexpr f32 WORLD_SCALE = 1.0F; // 1 unit = 1 meter

constexpr int MAX_ENTITIES = 10000;
constexpr int MAX_COMPONENTS = 64;
constexpr int INVALID_ENTITY_ID = -1;

constexpr size_t NAME_MAX_LEN = 128;
constexpr size_t PATH_MAX_LEN = 512;
constexpr int MAX_LOADED_ASSETS = 1024;

constexpr int MAX_SCENES = 32;
constexpr int MAX_SCENE_OBJECTS = 5000;

}; // namespace Aquila::SharedConstants

#endif
