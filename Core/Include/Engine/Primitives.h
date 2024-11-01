//
// Created by alexa on 01/11/2024.
//

#ifndef PRIMITIVES_H
#define PRIMITIVES_H

#include <stdexcept>
#include <glm/glm.hpp>
#include <vector>
#include "Common.h"
#include "Vertex.h"

namespace Engine {

    class Primitives {
    public:
        enum class PrimitiveType {
            Cube,
            Sphere,
            Plane,
            Cylinder
        };


        static void Draw(VkCommandBuffer commandBuffer, VkPipelineLayout pipelineLayout, VkPipeline pipeline, VkBuffer vertexBuffer, VkDeviceSize offset, PrimitiveType type, int vertexCount);
        static std::vector<Vertex> CreateCube(float size);
        static std::vector<Vertex> CreateSphere(float radius, int sectorCount, int stackCount);
        static std::vector<Vertex> CreateCylinder(float radius, float height, int segments);
    };
}




#endif //PRIMITIVES_H
