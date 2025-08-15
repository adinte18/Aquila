//
// Created by adinte on 7/5/24.
//

#ifndef VK_APP_VK_MODEL_H
#define VK_APP_VK_MODEL_H

#include "AquilaCore.h"

#include "Engine/Renderer/Device.h"
#include "Engine/Renderer/Buffer.h"
#include "Engine/Renderer/Vertex.h"

#include "assimp/Importer.hpp"
#include "assimp/postprocess.h"
#include "assimp/scene.h"

namespace Engine {

    struct MeshData {
        std::vector<Vertex> vertices;
        std::vector<uint32_t> indices;
        std::string path;

        void Load(const std::string& filepath);
    };

    class Mesh
    {
        struct Primitive {
            uint32_t firstIndex;
            uint32_t firstVertex;
            uint32_t indexCount;
            uint32_t vertexCount;
        };

    private:
        void CreateVertexBuffer(const std::vector<Vertex> &vertices);
        void CreateIndexBuffer(const std::vector<uint32_t> &indices);

        Device &m_Device;
        Unique<Buffer> m_VertexBuffer;
        uint32_t m_VertexCount;

        bool m_HasIndexBuffer = false;
        Unique<Buffer> m_IndexBuffer;
        uint32_t m_IndexCount;

        std::vector<Primitive> m_Primitives{};

        std::string m_Path;

        std::vector<Vertex> m_Vertices{};
        std::vector<uint32_t> m_Indices{};
    public:
        explicit Mesh(Device &device);
        ~Mesh();

        Mesh(const Mesh&) = delete;
        Mesh& operator=(const Mesh&) = delete;

        void UpdateVertexBuffer(std::vector<Vertex> &vertices) const;

        void Load(const std::string& filepath);

        void LoadFromData(const MeshData& meshData);

        void Bind(VkCommandBuffer commandBuffer) const;
        void Draw(VkCommandBuffer commandBuffer, VkPipelineLayout pipelineLayout, VkDescriptorSet descriptorSet);

        [[nodiscard]] std::string GetPath() const { return m_Path; }
    };
}

#endif //VK_APP_VK_MODEL_H
