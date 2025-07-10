
#include "Engine/Mesh.h"
#include "vulkan/vulkan_core.h"

namespace Engine {
    Mesh::Mesh(Device &device)
    : m_Device{device},
    m_VertexCount(0),
    m_IndexCount(0) {}

    Mesh::~Mesh() = default;

    void Mesh::Load(const std::string& filepath) {
        Assimp::Importer importer;
        const aiScene* scene = importer.ReadFile(filepath,
            aiProcess_Triangulate | aiProcess_GenSmoothNormals | aiProcess_GenUVCoords | aiProcess_CalcTangentSpace | aiProcess_JoinIdenticalVertices | aiProcess_OptimizeMeshes | aiProcess_ConvertToLeftHanded);

        if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
            throw std::runtime_error("Assimp failed to load model: " + std::string(importer.GetErrorString()));
        }

        const std::filesystem::path filePath(filepath);
        const std::string directoryPath = filePath.parent_path().string();
        m_Path = directoryPath;
        
        m_Vertices.clear();
        m_Indices.clear();
        m_Primitives.clear();

        Delegate<void(aiNode*, const aiScene*)> ProcessNode; // lambda function to process nodes

        ProcessNode = [&](aiNode* node, const aiScene* scene) {
            for (unsigned int i = 0; i < node->mNumMeshes; i++) {
                aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];

                uint32_t vertexOffset = m_Vertices.size();
                uint32_t indexOffset = m_Indices.size();

                for (unsigned int j = 0; j < mesh->mNumVertices; j++) {
                    Vertex vertex{};
                    vertex.pos = glm::vec4(mesh->mVertices[j].x, mesh->mVertices[j].y, mesh->mVertices[j].z, 1.0f);
                    vertex.normals = glm::normalize(mesh->HasNormals() ? glm::vec3(mesh->mNormals[j].x, mesh->mNormals[j].y, mesh->mNormals[j].z) : glm::vec3(0.0f));
                    vertex.texcoord = mesh->HasTextureCoords(0) ? glm::vec2(mesh->mTextureCoords[0][j].x, mesh->mTextureCoords[0][j].y) : glm::vec2(0.0f);
                    vertex.tangent = mesh->HasTangentsAndBitangents() ? glm::normalize(glm::vec3(mesh->mTangents[j].x, mesh->mTangents[j].y, mesh->mTangents[j].z)) : glm::vec3(0.0f);
                    m_Vertices.push_back(vertex);
                }

                for (unsigned int j = 0; j < mesh->mNumFaces; j++) {
                    aiFace face = mesh->mFaces[j];
                    for (unsigned int k = 0; k < face.mNumIndices; k++) {
                        m_Indices.push_back(face.mIndices[k] + vertexOffset);
                    }
                }

                Primitive prim{};
                prim.firstVertex = vertexOffset;
                prim.vertexCount = mesh->mNumVertices;
                prim.indexCount = mesh->mNumFaces * 3;
                prim.firstIndex = indexOffset;
                m_Primitives.push_back(prim);
            }

            for (unsigned int i = 0; i < node->mNumChildren; i++) {
                ProcessNode(node->mChildren[i], scene);
            }
        };

        ProcessNode(scene->mRootNode, scene);

        CreateVertexBuffer(m_Vertices);
        CreateIndexBuffer(m_Indices);
    }

    void Mesh::CreateVertexBuffer(const std::vector<Vertex> &vertices) {
        m_VertexCount = static_cast<uint32_t>(vertices.size());
        AQUILA_CORE_ASSERT(m_VertexCount >= 3 && "Vertex count must be at least 3");
        VkDeviceSize bufferSize = sizeof(vertices[0]) * m_VertexCount;
        uint32_t vertexSize = sizeof(vertices[0]);

        Buffer stagingBuffer{
            m_Device,
            vertexSize,
            m_VertexCount,
            VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
         };

        stagingBuffer.map();
        stagingBuffer.vk_WriteToBuffer((void*)vertices.data());

        m_VertexBuffer = std::make_unique<Buffer>(m_Device,
            vertexSize,
            m_VertexCount,
            VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
        );

        m_Device.CopyBuffer(stagingBuffer.vk_GetBuffer(),
            m_VertexBuffer->vk_GetBuffer(),
            bufferSize);
    }

    void Mesh::UpdateVertexBuffer(std::vector<Vertex>& vertices) const {
        if (!m_VertexBuffer) {
            throw std::runtime_error("Cannot update vertex buffer: Buffer not created!");
        }

        const VkDeviceSize bufferSize = sizeof(vertices[0]) * vertices.size();
        const uint32_t vertexSize = sizeof(vertices[0]);


        Buffer stagingBuffer{
            m_Device,
            vertexSize,
            static_cast<uint32_t>(vertices.size()),
            VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
        };

        stagingBuffer.map();
        stagingBuffer.vk_WriteToBuffer(vertices.data());

        m_Device.CopyBuffer(stagingBuffer.vk_GetBuffer(),
            m_VertexBuffer->vk_GetBuffer(),
            bufferSize);
    }


    void Mesh::CreateIndexBuffer(const std::vector<uint32_t> &indices) {
        m_IndexCount = static_cast<uint32_t>(indices.size());
        m_HasIndexBuffer = m_IndexCount > 0;

        if (!m_HasIndexBuffer) return;

        const VkDeviceSize bufferSize = sizeof(indices[0]) * m_IndexCount;
        uint32_t indexSize = sizeof(indices[0]);

        Buffer stagingBuffer{
            m_Device,
            indexSize,
            m_IndexCount,
            VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
        };

        stagingBuffer.map();
        stagingBuffer.vk_WriteToBuffer((void*)indices.data());

        m_IndexBuffer = std::make_unique<Buffer>(m_Device,
            indexSize,
            m_IndexCount,
            VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
        );

        //Move data from staging buffer to index buffer
        m_Device.CopyBuffer(stagingBuffer.vk_GetBuffer(), m_IndexBuffer->vk_GetBuffer(), bufferSize);
    }

    void Mesh::Draw(VkCommandBuffer commandBuffer, VkPipelineLayout pipelineLayout, VkDescriptorSet descriptorSet) {
        if (!m_Primitives.empty()){
            for(auto& primitive : m_Primitives){
                if (m_HasIndexBuffer){
                    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, 
                        pipelineLayout, 0, 1, &descriptorSet, 0, nullptr);
                    vkCmdDrawIndexed(commandBuffer, primitive.indexCount,
                    1, primitive.firstIndex, primitive.firstVertex, 0);
                }
                else {
                    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout,
                        0, 1, &descriptorSet, 0, nullptr);
                    vkCmdDraw(commandBuffer, primitive.vertexCount, 1, 0, 0);
                }
            }
        }
    }

    // void Mesh::Draw(VkCommandBuffer commandBuffer, VkDescriptorSet descriptorSet, VkPipelineLayout pipelineLayout) const {
    //     if (!m_Primitives.empty()) {
    //         for (auto& primitive : m_Primitives) {
    //             if (m_HasIndexBuffer) {
    //                 if (primitive.material.descriptorSet) {
    //                     // Only bind the material descriptor set if it exists
    //                     std::array<VkDescriptorSet, 2> sets{descriptorSet, primitive.material.descriptorSet};
    //                     vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout,
    //                         0, static_cast<uint32_t>(sets.size()), sets.data(), 0, nullptr);
    //                 } else {
    //                     // If no material descriptor set, just bind the default descriptor set
    //                     vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout,
    //                         0, 1, &descriptorSet, 0, nullptr);
    //                 }
    //                 vkCmdDrawIndexed(commandBuffer, primitive.indexCount,
    //                     1, primitive.firstIndex, primitive.firstVertex, 0);
    //             } else {
    //                 // If no index buffer, just draw the vertices
    //                 vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout,
    //                     0, 1, &descriptorSet, 0, nullptr);
    //                 vkCmdDraw(commandBuffer, primitive.vertexCount, 1, 0, 0);
    //             }
    //         }
    //     } else {
    //         throw std::runtime_error("No primitives found");
    //     }
    // }


    void Mesh::Bind(VkCommandBuffer commandBuffer) const {
        const VkBuffer buffers[] = {m_VertexBuffer->vk_GetBuffer()};
        constexpr VkDeviceSize offsets[] = {0};
        vkCmdBindVertexBuffers(commandBuffer, 0, 1, buffers, offsets);

        if (m_HasIndexBuffer)
            vkCmdBindIndexBuffer(commandBuffer, m_IndexBuffer->vk_GetBuffer(), 0, VK_INDEX_TYPE_UINT32);
    }

};

