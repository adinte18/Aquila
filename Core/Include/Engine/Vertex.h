//
// Created by adinte on 6/30/24.
//

#ifndef VK_APP_VERTEX_H
#define VK_APP_VERTEX_H

#include <glm/glm.hpp>
#include <vulkan/vulkan.h>
#include <array>
#include <vector>
#include <cstddef>

struct Vertex {
    glm::vec3 pos{};
    glm::vec3 color{};
    glm::vec3 normals{};
    glm::vec2 texcoord{};
    glm::vec3 tangent{};

    bool operator==(const Vertex& other) const{
        return  pos == other.pos
        && color == other.color
        && normals == other.normals
        && texcoord == other.texcoord;
    }

    static std::vector<VkVertexInputBindingDescription> getBindingDescriptions() {
        std::vector<VkVertexInputBindingDescription> bindingDescriptions(1);
        bindingDescriptions[0].binding = 0;
        bindingDescriptions[0].stride = sizeof(Vertex);
        bindingDescriptions[0].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
        return bindingDescriptions;
    }

    static std::vector<VkVertexInputAttributeDescription> getAttributeDescriptions() {
        std::vector<VkVertexInputAttributeDescription> attributeDescriptions{};

        attributeDescriptions.push_back({0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, pos)});
        attributeDescriptions.push_back({1, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, color)});
        attributeDescriptions.push_back({2, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, normals)});
        attributeDescriptions.push_back({3, 0, VK_FORMAT_R32G32B32A32_SFLOAT, offsetof(Vertex, tangent)});
        attributeDescriptions.push_back({4, 0, VK_FORMAT_R32G32_SFLOAT, offsetof(Vertex, texcoord)});

        return attributeDescriptions;
    }
};

#endif //VK_APP_VERTEX_H
