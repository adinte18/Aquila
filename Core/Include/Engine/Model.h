//
// Created by adinte on 7/5/24.
//

#ifndef VK_APP_VK_MODEL_H
#define VK_APP_VK_MODEL_H

#include "Common.h"

#include "Engine/Device.h"
#include "Engine/Buffer.h"
#include "Engine/Texture2D.h"
#include "Vertex.h"

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#define GLM_ENABLE_EXPERIMENTAL
#include <Components.h>
#include <glm/gtx/hash.hpp>

#include <memory>
#include <mutex>
#include <ECS/Scene.h>
#include <vector>

#include "Descriptor.h"
#include "Engine/Primitives.h"

namespace std {
    template <typename T, typename... Rest>
    void hashCombine(std::size_t& seed, const T& v, const Rest&... rest) {
        seed ^= std::hash<T>{}(v) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
        (hashCombine(seed, rest), ...);
    }


    template<>
    struct hash<Vertex> {
        size_t operator()(Vertex const &vertex) const noexcept {
            size_t seed = 0;
            hashCombine(seed, vertex.pos, vertex.color, vertex.normals, vertex.texcoord);
            return seed;
        }
    };
}

namespace Engine {
    class Model3D
    {
        struct Primitive {
            uint32_t firstIndex;
            uint32_t firstVertex;
            uint32_t indexCount;
            uint32_t vertexCount;
            ECS::PBRMaterial material;
        };

    private:
        void vk_CreateVertexBuffers(const std::vector<Vertex> &vertices);
        void vk_CreateIndexBuffer(const std::vector<uint32_t> &indices);

        Device &device;
        std::unique_ptr<Buffer> vertexBuffer;
        uint32_t vertexCount;

        bool hasIndexBuffer = false;
        std::unique_ptr<Buffer> indexBuffer;
        uint32_t indexCount;

        std::vector<Primitive> primitives{};
        std::vector<std::shared_ptr<Texture2D>> images;

        ECS::PBRMaterial pbrMaterial{};
        Buffer materialBuffer;

        std::string path;

    public:
        void vk_UpdateVertexBuffer(std::vector<Vertex> &vertices);

        void Load(const std::string& filepath, Engine::DescriptorSetLayout& materialSetLayout,Engine::DescriptorPool& descriptorPool);
        void CreatePrimitive(Primitives::PrimitiveType type, float size, Engine::DescriptorSetLayout &materialSetLayout, Engine::DescriptorPool &
                             descriptorPool);

        void CreateQuad(float size);
        void CreateCube();


        explicit Model3D(Device &device);
        ~Model3D();

        Model3D(const Model3D&) = delete;
        Model3D& operator=(const Model3D&) = delete;

        static std::shared_ptr<Model3D> create(Device& device);
        void bind(VkCommandBuffer commandBuffer) const;
        void draw(VkCommandBuffer commandBuffer, VkDescriptorSet descriptorSet, VkPipelineLayout pipelineLayout) const;
        void draw(VkCommandBuffer commandBuffer);

        void LoadTextureAsync(const std::string& uri, std::vector<std::shared_ptr<Engine::Texture2D>>& images, size_t imageIndex, std::mutex& imageMutex);

        [[nodiscard]] std::string GetPath() const { return path; }
        [[nodiscard]] const ECS::PBRMaterial& GetMaterial() const { return pbrMaterial; }
        [[nodiscard]] Buffer* GetMaterialBuffer() { return &materialBuffer; }

        std::vector<Vertex> vertices{};
        std::vector<uint32_t> indices{};
    };
}

#endif //VK_APP_VK_MODEL_H
