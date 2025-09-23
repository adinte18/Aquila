//
// Created by alexa on 01/11/2024.
//

#ifndef PRIMITIVES_H
#define PRIMITIVES_H

#include "AquilaCore.h"
#include "Engine/Renderer/Vertex.h"
#include <glm/glm.hpp>
#include <vector>

namespace Engine {

class Primitives {
public:
  enum class PrimitiveType { Cube, Sphere, Plane };

  static std::vector<Vertex> CreateCube(f32 size);
  static std::vector<Vertex> CreateSphere(f32 radius, int sectorCount,
                                          int stackCount);
};
} // namespace Engine

#endif // PRIMITIVES_H
