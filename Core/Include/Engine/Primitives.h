//
// Created by alexa on 01/11/2024.
//

#ifndef PRIMITIVES_H
#define PRIMITIVES_H

#include <glm/glm.hpp>
#include <vector>
#include "AquilaCore.h"
#include "Engine/Renderer/Vertex.h"

namespace Engine {

    class Primitives {
    public:
        enum class PrimitiveType {
            Cube,
            Sphere,
            Plane
        };


        static std::vector<Vertex> CreateCube(float size);
        static std::vector<Vertex> CreateSphere(float radius, int sectorCount, int stackCount);
    };
}




#endif //PRIMITIVES_H
