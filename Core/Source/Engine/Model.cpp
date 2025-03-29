
#include <cstring>

#define TINYGLTF_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "Engine/Descriptor.h"
#include "Engine/Model.h"

#include "Components.h"
#include "External/tiny_gltf.h"

Engine::Model3D::Model3D(Device &device) : device{device}, vertexCount(0), indexCount(0) {}

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


void Engine::Model3D::Load(const std::string& filepath, Engine::DescriptorSetLayout& materialSetLayout,Engine::DescriptorPool& descriptorPool) {
    tinygltf::Model model;
    tinygltf::TinyGLTF loader;
    std::string err, warn;

    bool loaded = loader.LoadASCIIFromFile(&model, &err, &warn, filepath);
    if (!loaded) {
        throw std::runtime_error(err);
    }

    // Ensure images vector has enough space for all textures
    images.resize(model.images.size());
    std::cout << "Images vector resized to: " << images.size() << std::endl;

    std::filesystem::path filePath(filepath);
    std::string directoryPath = filePath.parent_path().string();
    path = directoryPath;

    std::mutex imageMutex;
    std::vector<std::thread> threads;

    for (size_t i = 0; i < model.images.size(); ++i) {
        std::cout << "Starting to load texture " << i << " of " << model.images.size() << std::endl;
        std::string uri = directoryPath + "/" + model.images[i].uri;
        threads.push_back(std::thread([this, uri, i, &imageMutex]() {
            LoadTextureAsync(uri, std::ref(images), i, std::ref(imageMutex));
        }));
    }

    for (auto& thread : threads) {
        thread.join();
    }

    threads.clear();

    vertices.clear();
    indices.clear();

    for (auto& scene : model.scenes) {
        for(size_t i = 0; i < scene.nodes.size(); i++) {
            auto& node = model.nodes[scene.nodes[i]];
            uint32_t vertexOffset = 0;
            uint32_t indexOffset = 0;
            if (node.mesh < model.meshes.size()) {
                for (auto& primitive : model.meshes[node.mesh].primitives) {
                    uint32_t vertexCount = 0;
                    uint32_t indexCount = 0;

                    const float *positionBuffer = nullptr;
                    const float *normalsBuffer = nullptr;
                    const float *texCoordsBuffer = nullptr;
                    const float *tangentsBuffer = nullptr;

                    if (primitive.attributes.find("POSITION") != primitive.attributes.end()) {
                        const tinygltf::Accessor& accessor = model.accessors[primitive.attributes.find("POSITION")->second];
                        const tinygltf::BufferView& view = model.bufferViews[accessor.bufferView];
                        positionBuffer = reinterpret_cast<const float*>(&(model.buffers[view.buffer].data[accessor.byteOffset + view.byteOffset]));
                        vertexCount = accessor.count;
                    }

                    if (primitive.attributes.find("NORMAL") != primitive.attributes.end()) {
                        const tinygltf::Accessor& accessor = model.accessors[primitive.attributes.find("NORMAL")->second];
                        const tinygltf::BufferView& view = model.bufferViews[accessor.bufferView];
                        normalsBuffer = reinterpret_cast<const float*>(&(model.buffers[view.buffer].data[accessor.byteOffset + view.byteOffset]));
                    }

                    if (primitive.attributes.find("TEXCOORD_0") != primitive.attributes.end()) {
                        const tinygltf::Accessor& accessor = model.accessors[primitive.attributes.find("TEXCOORD_0")->second];
                        const tinygltf::BufferView& view = model.bufferViews[accessor.bufferView];
                        texCoordsBuffer = reinterpret_cast<const float*>(&(model.buffers[view.buffer].data[accessor.byteOffset + view.byteOffset]));
                    }

                    if (primitive.attributes.find("TANGENT") != primitive.attributes.end()) {
                        const tinygltf::Accessor& accessor = model.accessors[primitive.attributes.find("TANGENT")->second];
                        const tinygltf::BufferView& view = model.bufferViews[accessor.bufferView];
                        tangentsBuffer = reinterpret_cast<const float*>(&(model.buffers[view.buffer].data[accessor.byteOffset + view.byteOffset]));
                    }

                    for (size_t v = 0; v < vertexCount; v++) {
                        Vertex vertex{};
                        vertex.pos = glm::vec4(glm::make_vec3(&positionBuffer[v * 3]), 1.0f);
                        vertex.color = glm::vec3(1.0);
                        vertex.normals = glm::normalize(
                                glm::vec3(normalsBuffer ? glm::make_vec3(&normalsBuffer[v * 3]) : glm::vec3(0.0f)));
                        vertex.tangent = glm::vec4(
                                tangentsBuffer ? glm::make_vec4(&tangentsBuffer[v * 4]) : glm::vec4(0.0f));;
                        vertex.texcoord = texCoordsBuffer ? glm::make_vec2(&texCoordsBuffer[v * 2]) : glm::vec2(0.0f);
                        vertices.push_back(vertex);
                    }

                    const tinygltf::Accessor& accessor = model.accessors[primitive.indices];
                    const tinygltf::BufferView& bufferView = model.bufferViews[accessor.bufferView];
                    const tinygltf::Buffer& buffer = model.buffers[bufferView.buffer];

                    indexCount += static_cast<uint32_t>(accessor.count);

                    switch (accessor.componentType) {
                    case TINYGLTF_PARAMETER_TYPE_UNSIGNED_INT: {
                            const uint32_t* buf = reinterpret_cast<const uint32_t*>(&buffer.data[accessor.byteOffset + bufferView.byteOffset]);
                            for (size_t index = 0; index < accessor.count; index++) {
                                indices.push_back(buf[index]);
                            }
                            break;
                    }
                    case TINYGLTF_PARAMETER_TYPE_UNSIGNED_SHORT: {
                            const uint16_t* buf = reinterpret_cast<const uint16_t*>(&buffer.data[accessor.byteOffset + bufferView.byteOffset]);
                            for (size_t index = 0; index < accessor.count; index++) {
                                indices.push_back(buf[index]);
                            }
                            break;
                    }
                    case TINYGLTF_PARAMETER_TYPE_UNSIGNED_BYTE: {
                            const uint8_t* buf = reinterpret_cast<const uint8_t*>(&buffer.data[accessor.byteOffset + bufferView.byteOffset]);
                            for (size_t index = 0; index < accessor.count; index++) {
                                indices.push_back(buf[index]);
                            }
                            break;
                    }
                    default:
                        std::cerr << "Index component type " << accessor.componentType << " not supported!" << std::endl;
                        return;
                    }

                    auto defaultTex = Engine::Texture2D::create(device);
                    defaultTex->CreateTexture(std::string(TEXTURES_PATH) + "/TemplateGrid_albedo.png");

                    Material material{};

                    if (primitive.material != -1) {
                        tinygltf::Material &primitiveMaterial = model.materials[primitive.material];

                        if (primitiveMaterial.pbrMetallicRoughness.baseColorTexture.index != -1) {
                            uint32_t textureIndex = primitiveMaterial.pbrMetallicRoughness.baseColorTexture.index;
                            uint32_t imageIndex = model.textures[textureIndex].source;
                            material.albedoTexture = images[imageIndex];
                        } else {
                            material.albedoTexture = defaultTex;
                        }

                        if (primitiveMaterial.pbrMetallicRoughness.metallicRoughnessTexture.index != -1) {
                            uint32_t textureIndex = primitiveMaterial.pbrMetallicRoughness.metallicRoughnessTexture.index;
                            uint32_t imageIndex = model.textures[textureIndex].source;
                            material.metallicRoughnessTexture = images[imageIndex];
                        } else {
                            material.metallicRoughnessTexture = defaultTex;
                        }

                        if (primitiveMaterial.normalTexture.index != -1) {
                            uint32_t textureIndex = primitiveMaterial.normalTexture.index;
                            uint32_t imageIndex = model.textures[textureIndex].source;
                            material.normalTexture = images[imageIndex];
                        } else {
                            material.normalTexture = defaultTex;
                        }
                    } else {
                        material.albedoTexture = defaultTex;
                        material.normalTexture = defaultTex;
                        material.metallicRoughnessTexture = defaultTex;
                    }

                    this->material = material;

                    VkDescriptorImageInfo albedoImageInfo = material.albedoTexture->GetDescriptorSetInfo();
                    VkDescriptorImageInfo normalImageInfo = material.normalTexture->GetDescriptorSetInfo();
                    VkDescriptorImageInfo metallicRoughnessImageInfo = material.metallicRoughnessTexture->GetDescriptorSetInfo();

                    DescriptorWriter writer(materialSetLayout, descriptorPool);
                    writer.writeImage(0, &albedoImageInfo);
                    writer.writeImage(1, &normalImageInfo);
                    writer.writeImage(2, &metallicRoughnessImageInfo);
                    writer.build(material.descriptorSet);

                    Primitive prim{};
                    prim.firstVertex = vertexOffset;
                    prim.vertexCount = vertexCount;
                    prim.indexCount = indexCount;
                    prim.firstIndex = indexOffset;
                    prim.material = material;
                    primitives.push_back(prim);

                    vertexOffset += vertexCount;
                    indexOffset += indexCount;
                }
            }
        }
    }

    vk_CreateVertexBuffers(vertices);
    vk_CreateIndexBuffer(indices);
}

void Engine::Model3D::CreatePrimitive(Primitives::PrimitiveType type, float size, Engine::DescriptorSetLayout& materialSetLayout, Engine::DescriptorPool& descriptorPool) {
    vertices.clear();
    indices.clear();

    if (type == Primitives::PrimitiveType::Cube) {
        vertices = Primitives::CreateCube(size);
        for (uint32_t i = 0; i < vertices.size(); ++i) {
            indices.push_back(i);
        }
    }
    else if (type == Primitives::PrimitiveType::Sphere) {
        vertices = Primitives::CreateSphere(size, 36, 18);
        for (uint32_t i = 0; i < vertices.size(); ++i) {
            indices.push_back(i);
        }
    }

    vk_CreateVertexBuffers(vertices);
    vk_CreateIndexBuffer(indices);

    // Create a default texture
    auto defaultTex = Engine::Texture2D::create(device);
    defaultTex->CreateTexture(std::string(TEXTURES_PATH) + "/TemplateGrid_albedo.png");

    // Create a material and descriptor set for the primitive
    Material material{};
    material.albedoTexture = defaultTex;
    material.normalTexture = defaultTex;
    material.metallicRoughnessTexture = defaultTex;
    this->material = material;

    VkDescriptorImageInfo albedoImageInfo = material.albedoTexture->GetDescriptorSetInfo();
    VkDescriptorImageInfo normalImageInfo = material.normalTexture->GetDescriptorSetInfo();
    VkDescriptorImageInfo metallicRoughnessImageInfo = material.metallicRoughnessTexture->GetDescriptorSetInfo();

    DescriptorWriter writer(materialSetLayout, descriptorPool);
    writer.writeImage(0, &albedoImageInfo);
    writer.writeImage(1, &normalImageInfo);
    writer.writeImage(2, &metallicRoughnessImageInfo);
    writer.build(material.descriptorSet);

    // Create a primitive and link it to the material
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
