
#define TINYGLTF_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "Engine/Descriptor.h"
#include "Engine/Model.h"

#include "Components.h"
#include "assimp/Importer.hpp"
#include "assimp/postprocess.h"
#include "assimp/scene.h"
#include "External/tiny_gltf.h"

Engine::Model3D::Model3D(Device &device)
    : device{device},
    vertexCount(0),
    indexCount(0),
    materialBuffer{device,
        sizeof(ECS::MaterialData),
        1,
        VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT} {}

Engine::Model3D::~Model3D() = default;

void Engine::Model3D::LoadTextureAsync(const std::string& uri, std::vector<std::shared_ptr<Texture2D>>& images, size_t index, std::mutex& imageMutex) {
    std::cout << "LoadTextureAsync called for index " << index << " (images.size() = " << images.size() << ")" << std::endl;

    if (index >= images.size()) {
        std::cerr << "Index out of range: " << index << " (images.size() = " << images.size() << ")" << std::endl;
        return;
    }

    std::shared_ptr<Texture2D> texture = Texture2D::create(device);
    texture->CreateTexture(uri);

    std::lock_guard<std::mutex> lock(imageMutex);
    images[index] = texture;
}


void Engine::Model3D::Load(const std::string& filepath, Engine::DescriptorSetLayout& materialSetLayout, Engine::DescriptorPool& descriptorPool) {
    Assimp::Importer importer;
    const aiScene* scene = importer.ReadFile(filepath,
        aiProcess_Triangulate | aiProcess_GenSmoothNormals | aiProcess_GenUVCoords | aiProcess_CalcTangentSpace | aiProcess_JoinIdenticalVertices | aiProcess_OptimizeMeshes | aiProcess_ConvertToLeftHanded);

    if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
        throw std::runtime_error("Assimp failed to load model: " + std::string(importer.GetErrorString()));
    }

    std::filesystem::path filePath(filepath);
    std::string directoryPath = filePath.parent_path().string();
    path = directoryPath;

    std::unordered_map<std::string, std::shared_ptr<Engine::Texture2D>> textureCache;

    vertices.clear();
    indices.clear();

    std::function<void(aiNode*, const aiScene*)> ProcessNode;
    ProcessNode = [&](aiNode* node, const aiScene* scene) {
        for (unsigned int i = 0; i < node->mNumMeshes; i++) {
            aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];

            uint32_t vertexOffset = vertices.size();
            uint32_t indexOffset = indices.size();

            for (unsigned int j = 0; j < mesh->mNumVertices; j++) {
                Vertex vertex{};
                vertex.pos = glm::vec4(mesh->mVertices[j].x, mesh->mVertices[j].y, mesh->mVertices[j].z, 1.0f);
                vertex.normals = glm::normalize(mesh->HasNormals() ? glm::vec3(mesh->mNormals[j].x, mesh->mNormals[j].y, mesh->mNormals[j].z) : glm::vec3(0.0f));
                vertex.texcoord = mesh->HasTextureCoords(0) ? glm::vec2(mesh->mTextureCoords[0][j].x, mesh->mTextureCoords[0][j].y) : glm::vec2(0.0f);
                vertex.tangent = mesh->HasTangentsAndBitangents() ? glm::normalize(glm::vec3(mesh->mTangents[j].x, mesh->mTangents[j].y, mesh->mTangents[j].z)) : glm::vec3(0.0f);
                vertices.push_back(vertex);
            }

            for (unsigned int j = 0; j < mesh->mNumFaces; j++) {
                aiFace face = mesh->mFaces[j];
                for (unsigned int k = 0; k < face.mNumIndices; k++) {
                    indices.push_back(face.mIndices[k] + vertexOffset);
                }
            }

            auto defaultTex = Engine::Texture2D::create(device);
            defaultTex->CreateTexture(std::string(TEXTURES_PATH) + "/TemplateGrid_albedo.png");

            ECS::PBRMaterial material{};
            if (mesh->mMaterialIndex >= 0) {
                aiMaterial* mat = scene->mMaterials[mesh->mMaterialIndex];

                auto LoadTexture = [&](aiTextureType type, Texture2D::TextureType fallbackType) -> std::shared_ptr<Engine::Texture2D> {
                    if (mat->GetTextureCount(type) == 0) {
                        auto tex = Engine::Texture2D::create(device);
                        tex->UseFallbackTextures(fallbackType);
                        return tex;
                    }

                    aiString texPath;
                    if (mat->GetTexture(type, 0, &texPath) != AI_SUCCESS) {
                        auto tex = Engine::Texture2D::create(device);
                        tex->UseFallbackTextures(fallbackType);
                        return tex;
                    }

                    // stripping any Assimp-specific suffixes like "*0"
                    std::string rawPath = texPath.C_Str();
                    std::cout << "Raw texture path from Assimp: " << rawPath << std::endl;
                    std::string cleanPath = rawPath.substr(0, rawPath.find_first_of('*'));
                    std::string fullPath = directoryPath + "/" + cleanPath;

                    std::cout << "Loading texture: " << fullPath << std::endl;

                    if (textureCache.find(fullPath) != textureCache.end()) {
                        return textureCache[fullPath];
                    }

                    auto tex = Engine::Texture2D::create(device);
                    tex->CreateTexture(fullPath);
                    textureCache[fullPath] = tex;
                    return tex;
                };



                material.albedoTexture = LoadTexture(aiTextureType_DIFFUSE, Texture2D::TextureType::Albedo);
                material.metallicRoughnessTexture = LoadTexture(aiTextureType_METALNESS, Texture2D::TextureType::MetallicRoughness);
                material.normalTexture = LoadTexture(aiTextureType_NORMALS, Texture2D::TextureType::Normal);
                material.emissiveTexture = LoadTexture(aiTextureType_EMISSIVE, Texture2D::TextureType::Emissive);
                material.aoTexture = LoadTexture(aiTextureType_AMBIENT_OCCLUSION, Texture2D::TextureType::AO);

                aiColor4D color;
                if (AI_SUCCESS == aiGetMaterialColor(mat, AI_MATKEY_COLOR_DIFFUSE, &color)) {
                    material.albedoColor = glm::vec3(color.r, color.g, color.b);
                } else {
                    material.albedoColor = glm::vec3(1.0f);
                }

                float metallic = 0.0f, roughness = 0.5f;
                aiGetMaterialFloat(mat, AI_MATKEY_METALLIC_FACTOR, &metallic);
                aiGetMaterialFloat(mat, AI_MATKEY_ROUGHNESS_FACTOR, &roughness);
                material.metallic = metallic;
                material.roughness = roughness;

                if (AI_SUCCESS == aiGetMaterialColor(mat, AI_MATKEY_COLOR_EMISSIVE, &color)) {
                    material.emissionColor = glm::vec3(color.r, color.g, color.b);
                    material.emissiveIntensity = color.a;
                } else {
                    material.emissionColor = glm::vec3(0.0f);
                    material.emissiveIntensity = 1.0f;
                }

                material.aoIntensity = 1.0f;
            } else {
                material.albedoTexture = defaultTex;
                material.normalTexture = defaultTex;
                material.metallicRoughnessTexture = defaultTex;
                material.emissiveTexture = defaultTex;
                material.aoTexture = defaultTex;
                material.albedoColor = glm::vec3(1.0f);
                material.metallic = 0.0f;
                material.roughness = 0.5f;
                material.emissionColor = glm::vec3(0.0f);
                material.emissiveIntensity = 1.0f;
                material.aoIntensity = 1.0f;
            }

            materialBuffer.map();
            this->pbrMaterial = material;

            VkDescriptorImageInfo albedoImageInfo = material.albedoTexture->GetDescriptorSetInfo();
            VkDescriptorImageInfo normalImageInfo = material.normalTexture->GetDescriptorSetInfo();
            VkDescriptorImageInfo metallicRoughnessImageInfo = material.metallicRoughnessTexture->GetDescriptorSetInfo();
            VkDescriptorImageInfo emissiveImageInfo = material.emissiveTexture->GetDescriptorSetInfo();
            VkDescriptorImageInfo aoImageInfo = material.aoTexture->GetDescriptorSetInfo();
            VkDescriptorBufferInfo materialBufferInfo = materialBuffer.vk_DescriptorInfo();

            DescriptorWriter writer(materialSetLayout, descriptorPool);
            writer.writeImage(0, &albedoImageInfo);
            writer.writeImage(1, &normalImageInfo);
            writer.writeImage(2, &metallicRoughnessImageInfo);
            writer.writeImage(3, &emissiveImageInfo);
            writer.writeImage(4, &aoImageInfo);
            writer.writeBuffer(6, &materialBufferInfo);
            writer.build(material.descriptorSet);

            Primitive prim{};
            prim.firstVertex = vertexOffset;
            prim.vertexCount = mesh->mNumVertices;
            prim.indexCount = mesh->mNumFaces * 3;
            prim.firstIndex = indexOffset;
            prim.material = material;
            primitives.push_back(prim);
        }

        for (unsigned int i = 0; i < node->mNumChildren; i++) {
            ProcessNode(node->mChildren[i], scene);
        }
    };

    ProcessNode(scene->mRootNode, scene);

    vk_CreateVertexBuffers(vertices);
    vk_CreateIndexBuffer(indices);
}

void Engine::Model3D::CreateCube() {
    vertices.clear();
    indices.clear();
    vertices = Primitives::CreateCube(1.0f);
    indices.resize(vertices.size());
    for (uint32_t i = 0; i < vertices.size(); ++i) {
        indices[i] = i;
    }
    vk_CreateVertexBuffers(vertices);
    vk_CreateIndexBuffer(indices);

    Primitive prim{};
    prim.firstVertex = 0;
    prim.vertexCount = static_cast<uint32_t>(vertices.size());
    prim.indexCount = static_cast<uint32_t>(indices.size());
    prim.firstIndex = 0;
    primitives.push_back(prim);
}

void Engine::Model3D::CreatePrimitive(Primitives::PrimitiveType type, float size, Engine::DescriptorSetLayout& materialSetLayout, Engine::DescriptorPool& descriptorPool) {
    vertices.clear();
    indices.clear();

    // Generate primitive geometry
    if (type == Primitives::PrimitiveType::Cube) {
        vertices = Primitives::CreateCube(size);
    }
    else if (type == Primitives::PrimitiveType::Sphere) {
        vertices = Primitives::CreateSphere(size, 64, 32);
    }

    // Generate index buffer
    for (uint32_t i = 0; i < vertices.size(); ++i) {
        indices.push_back(i);
    }

    vk_CreateVertexBuffers(vertices);
    vk_CreateIndexBuffer(indices);

    // Create a default texture
    auto defaultTex = Engine::Texture2D::create(device);
    defaultTex->UseFallbackTextures(Texture2D::TextureType::Albedo);

    auto defaultNormalTex = Engine::Texture2D::create(device);
    defaultNormalTex->UseFallbackTextures(Texture2D::TextureType::Normal);

    auto defaultMetallicRoughnessTex = Engine::Texture2D::create(device);
    defaultMetallicRoughnessTex->UseFallbackTextures(Texture2D::TextureType::MetallicRoughness);

    auto defaultAoTex = Engine::Texture2D::create(device);
    defaultAoTex->UseFallbackTextures(Texture2D::TextureType::AO);

    auto defaultEmissionTex = Texture2D::create(device);
    defaultEmissionTex->UseFallbackTextures(Texture2D::TextureType::Emissive);

    // Create material
    ECS::PBRMaterial material{};
    material.albedoTexture = defaultTex;
    material.normalTexture = defaultNormalTex;
    material.metallicRoughnessTexture = defaultMetallicRoughnessTex;
    material.aoTexture = defaultAoTex;
    material.emissiveTexture = defaultEmissionTex;

    // Set default material properties
    material.albedoColor = glm::vec3(1.0f);
    material.metallic = 0.0f;
    material.roughness = 1.0f;
    material.emissionColor = glm::vec3(0.0f);
    material.emissiveIntensity = 0.0f;
    material.aoIntensity = 1.0f;
    material.tiling = glm::vec2(1.0f);
    material.offset = glm::vec2(0.0f);

    materialBuffer.map();

    // Create descriptor set and write all texture bindings
    VkDescriptorImageInfo albedoImageInfo = material.albedoTexture->GetDescriptorSetInfo();
    VkDescriptorImageInfo normalImageInfo = material.normalTexture->GetDescriptorSetInfo();
    VkDescriptorImageInfo metallicRoughnessImageInfo = material.metallicRoughnessTexture->GetDescriptorSetInfo();
    VkDescriptorImageInfo aoImageInfo = material.aoTexture->GetDescriptorSetInfo();
    VkDescriptorImageInfo emissiveImageInfo = material.emissiveTexture->GetDescriptorSetInfo();
    VkDescriptorBufferInfo materialBufferInfo = materialBuffer.vk_DescriptorInfo();

    DescriptorWriter writer(materialSetLayout, descriptorPool);
    writer.writeImage(0, &albedoImageInfo);
    writer.writeImage(1, &normalImageInfo);
    writer.writeImage(2, &metallicRoughnessImageInfo);
    writer.writeImage(3, &aoImageInfo);
    writer.writeImage(4, &emissiveImageInfo);
    writer.writeBuffer(6, &materialBufferInfo);
    writer.build(material.descriptorSet);

    this->pbrMaterial = material;

    // Create primitive and assign material
    Primitive prim{};
    prim.firstVertex = 0;
    prim.vertexCount = static_cast<uint32_t>(vertices.size());
    prim.indexCount = static_cast<uint32_t>(indices.size());
    prim.firstIndex = 0;
    prim.material = material;
    primitives.push_back(prim);
}


void Engine::Model3D::CreateQuad(float size) {
    vertices.clear();
    indices.clear();

    std::vector<Vertex> quadVertices = {
        // Change y positions for horizontal alignment
        {glm::vec4(-size / 2.0f, 0.0f, -size / 2.0f, 1.0f), glm::vec3(1.0f), glm::vec3(0.0f, 0.0f, 1.0f), glm::vec2(0.0f, 0.0f)}, // Bottom-left
        {glm::vec4( size / 2.0f, 0.0f, -size / 2.0f, 1.0f), glm::vec3(1.0f), glm::vec3(0.0f, 0.0f, 1.0f), glm::vec2(1.0f, 0.0f)}, // Bottom-right
        {glm::vec4(-size / 2.0f, 0.0f,  size / 2.0f, 1.0f), glm::vec3(1.0f), glm::vec3(0.0f, 0.0f, 1.0f), glm::vec2(0.0f, 1.0f)}, // Top-left

        {glm::vec4( size / 2.0f, 0.0f, -size / 2.0f, 1.0f), glm::vec3(1.0f), glm::vec3(0.0f, 0.0f, 1.0f), glm::vec2(1.0f, 0.0f)}, // Bottom-right
        {glm::vec4( size / 2.0f, 0.0f,  size / 2.0f, 1.0f), glm::vec3(1.0f), glm::vec3(0.0f, 0.0f, 1.0f), glm::vec2(1.0f, 1.0f)}, // Top-right
        {glm::vec4(-size / 2.0f, 0.0f,  size / 2.0f, 1.0f), glm::vec3(1.0f), glm::vec3(0.0f, 0.0f, 1.0f), glm::vec2(0.0f, 1.0f)}  // Top-left
    };


    std::vector<uint32_t> quadIndices = {
        0, 1, 2,
        3, 4, 5
    };

    vk_CreateVertexBuffers(quadVertices);
    vk_CreateIndexBuffer(quadIndices);

    // Create a primitive and link it to the material
    Primitive prim{};
    prim.firstVertex = 0;
    prim.vertexCount = static_cast<uint32_t>(quadVertices.size());
    prim.indexCount = static_cast<uint32_t>(quadIndices.size());
    prim.firstIndex = 0;
    primitives.push_back(prim);
}



void Engine::Model3D::vk_CreateVertexBuffers(const std::vector<Vertex> &vertices) {
    vertexCount = static_cast<uint32_t>(vertices.size());
    assert(vertexCount >= 3 && "Vertex count must be at least 3");
    VkDeviceSize bufferSize = sizeof(vertices[0]) * vertexCount;
    uint32_t vertexSize = sizeof(vertices[0]);

    Buffer stagingBuffer{
        device,
        vertexSize,
        vertexCount,
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
     };

    stagingBuffer.map();
    stagingBuffer.vk_WriteToBuffer((void*)vertices.data());

    vertexBuffer = std::make_unique<Buffer>(device,
        vertexSize,
        vertexCount,
        VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
    );

    //Move data from staging buffer to vertex buffer
    device.vk_CopyBuffer(stagingBuffer.vk_GetBuffer(),
        vertexBuffer->vk_GetBuffer(),
        bufferSize);
}

void Engine::Model3D::vk_UpdateVertexBuffer(std::vector<Vertex>& vertices) {
    if (!vertexBuffer) {
        throw std::runtime_error("Cannot update vertex buffer: Buffer not created!");
    }

    VkDeviceSize bufferSize = sizeof(vertices[0]) * vertices.size();
    uint32_t vertexSize = sizeof(vertices[0]);

    // Create a temporary staging buffer (CPU accessible)
    Buffer stagingBuffer{
        device,
        vertexSize,
        static_cast<uint32_t>(vertices.size()),
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
    };

    // Map memory and copy updated vertex data
    stagingBuffer.map();
    stagingBuffer.vk_WriteToBuffer((void*)vertices.data());

    // Copy updated data from staging buffer to GPU vertex buffer
    device.vk_CopyBuffer(stagingBuffer.vk_GetBuffer(),
        vertexBuffer->vk_GetBuffer(),
        bufferSize);
}


void Engine::Model3D::vk_CreateIndexBuffer(const std::vector<uint32_t> &indices) {
    indexCount = static_cast<uint32_t>(indices.size());
    hasIndexBuffer = indexCount > 0;

    if (!hasIndexBuffer) return;

    VkDeviceSize bufferSize = sizeof(indices[0]) * indexCount;
    uint32_t indexSize = sizeof(indices[0]);

    Buffer stagingBuffer{
        device,
        indexSize,
        indexCount,
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
    };

    stagingBuffer.map();
    stagingBuffer.vk_WriteToBuffer((void*)indices.data());

    indexBuffer = std::make_unique<Buffer>(device,
        indexSize,
        indexCount,
        VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
    );

    //Move data from staging buffer to index buffer
    device.vk_CopyBuffer(stagingBuffer.vk_GetBuffer(), indexBuffer->vk_GetBuffer(), bufferSize);
}

void Engine::Model3D::draw(VkCommandBuffer commandBuffer, VkDescriptorSet descriptorSet, VkPipelineLayout pipelineLayout) const {
    if (!primitives.empty()) {
        for (auto& primitive : primitives) {
            if (hasIndexBuffer) {
                if (primitive.material.descriptorSet) {
                    // Only bind the material descriptor set if it exists
                    std::array<VkDescriptorSet, 2> sets{descriptorSet, primitive.material.descriptorSet};
                    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout,
                        0, static_cast<uint32_t>(sets.size()), sets.data(), 0, nullptr);
                } else {
                    // If no material descriptor set, just bind the default descriptor set
                    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout,
                        0, 1, &descriptorSet, 0, nullptr);
                }
                vkCmdDrawIndexed(commandBuffer, primitive.indexCount,
                    1, primitive.firstIndex, primitive.firstVertex, 0);
            } else {
                // If no index buffer, just draw the vertices
                vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout,
                    0, 1, &descriptorSet, 0, nullptr);
                vkCmdDraw(commandBuffer, primitive.vertexCount, 1, 0, 0);
            }
        }
    } else {
        throw std::runtime_error("No primitives found");
    }
}


std::shared_ptr<Engine::Model3D> Engine::Model3D::create(Device& device) {
    return std::make_shared<Engine::Model3D>(device);
}


void Engine::Model3D::bind(VkCommandBuffer commandBuffer) const {
    VkBuffer buffers[] = {vertexBuffer->vk_GetBuffer()};
    VkDeviceSize offsets[] = {0};
    vkCmdBindVertexBuffers(commandBuffer, 0, 1, buffers, offsets);

    if (hasIndexBuffer)
        vkCmdBindIndexBuffer(commandBuffer, indexBuffer->vk_GetBuffer(), 0, VK_INDEX_TYPE_UINT32);
}
