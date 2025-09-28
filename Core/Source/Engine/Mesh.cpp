
#include "Engine/Mesh.h"
#include "AquilaCore.h"
#include "Platform/Filesystem/VirtualFileSystem.h"

namespace Engine {
Mesh::Mesh(Device &device, const std::string &debugName)
    : m_Device{device}, m_DebugName(debugName), m_VertexCount(0),
      m_IndexCount(0) {}

Mesh::~Mesh() = default;

void Mesh::Load(const std::string &filepath) {
  Assimp::Importer importer;

  auto file = VFS::VirtualFileSystem::Get()->OpenFile(filepath, "rb");
  if (!file || !file->IsValid()) {
    throw std::runtime_error("Failed to open mesh via VFS: " + filepath);
  }

  file->Seek(0, SEEK_SET);

  int64 fileSize = file->Size();
  if (fileSize <= 0) {
    throw std::runtime_error("Mesh file is empty: " + filepath);
  }

  std::vector<uint8> buffer(fileSize);
  size_t bytesRead = file->Read(buffer.data(), buffer.size());
  if (bytesRead != buffer.size()) {
    throw std::runtime_error("Failed to read entire mesh buffer from VFS");
  }

  const aiScene *scene = importer.ReadFileFromMemory(
      buffer.data(), buffer.size(),
      aiProcess_Triangulate | aiProcess_GenSmoothNormals |
          aiProcess_GenUVCoords | aiProcess_CalcTangentSpace |
          aiProcess_JoinIdenticalVertices | aiProcess_OptimizeMeshes |
          aiProcess_ConvertToLeftHanded);

  if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE ||
      !scene->mRootNode) {
    throw std::runtime_error("Assimp failed to load model: " +
                             std::string(importer.GetErrorString()));
  }

  m_Path = filepath;

  m_Vertices.clear();
  m_Indices.clear();
  m_Primitives.clear();

  Delegate<void(aiNode *, const aiScene *)>
      ProcessNode; // lambda function to process nodes

  ProcessNode = [&](aiNode *node, const aiScene *scene) {
    for (unsigned int i = 0; i < node->mNumMeshes; i++) {
      aiMesh *mesh = scene->mMeshes[node->mMeshes[i]];

      uint32 vertexOffset = m_Vertices.size();
      uint32 indexOffset = m_Indices.size();

      for (unsigned int j = 0; j < mesh->mNumVertices; j++) {
        Vertex vertex{};
        vertex.pos = glm::vec4(mesh->mVertices[j].x, mesh->mVertices[j].y,
                               mesh->mVertices[j].z, 1.0f);
        vertex.normals = glm::normalize(
            mesh->HasNormals() ? vec3(mesh->mNormals[j].x, mesh->mNormals[j].y,
                                      mesh->mNormals[j].z)
                               : vec3(0.0f));
        vertex.texcoord = mesh->HasTextureCoords(0)
                              ? vec2(mesh->mTextureCoords[0][j].x,
                                     mesh->mTextureCoords[0][j].y)
                              : vec2(0.0f);
        vertex.tangent = mesh->HasTangentsAndBitangents()
                             ? glm::normalize(vec3(mesh->mTangents[j].x,
                                                   mesh->mTangents[j].y,
                                                   mesh->mTangents[j].z))
                             : vec3(0.0f);
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
  m_VertexCount = static_cast<uint32>(vertices.size());
  AQUILA_ASSERT(m_VertexCount >= 3, "Vertex count must be at least 3");
  VkDeviceSize bufferSize = sizeof(vertices[0]) * m_VertexCount;
  uint32 vertexSize = sizeof(vertices[0]);

  Buffer stagingBuffer{m_Device,
                       std::string(m_DebugName + "_VertexStagingBuffer"),
                       vertexSize,
                       m_VertexCount,
                       VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                       VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                           VK_MEMORY_PROPERTY_HOST_COHERENT_BIT};

  stagingBuffer.Map();
  stagingBuffer.Write((void *)vertices.data());

  m_VertexBuffer = CreateUnique<Buffer>(
      m_Device, std::string(m_DebugName + "_VertexBuffer"), vertexSize,
      m_VertexCount,
      VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
      VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

  m_Device.CopyBuffer(stagingBuffer.GetBuffer(), m_VertexBuffer->GetBuffer(),
                      bufferSize);
}

void Mesh::UpdateVertexBuffer(std::vector<Vertex> &vertices) const {
  if (!m_VertexBuffer) {
    throw std::runtime_error(
        "Cannot update vertex buffer: Buffer not created!");
  }

  const VkDeviceSize bufferSize = sizeof(vertices[0]) * vertices.size();
  const uint32 vertexSize = sizeof(vertices[0]);

  Buffer stagingBuffer{m_Device,
                       std::string(m_DebugName + "_VertexStagingBuffer"),
                       vertexSize,
                       static_cast<uint32>(vertices.size()),
                       VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                       VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                           VK_MEMORY_PROPERTY_HOST_COHERENT_BIT};

  stagingBuffer.Map();
  stagingBuffer.Write(vertices.data());

  m_Device.CopyBuffer(stagingBuffer.GetBuffer(), m_VertexBuffer->GetBuffer(),
                      bufferSize);
}

void Mesh::CreateIndexBuffer(const std::vector<uint32> &indices) {
  m_IndexCount = static_cast<uint32>(indices.size());
  m_HasIndexBuffer = m_IndexCount > 0;

  if (!m_HasIndexBuffer)
    return;

  const VkDeviceSize bufferSize = sizeof(indices[0]) * m_IndexCount;
  uint32 indexSize = sizeof(indices[0]);

  Buffer stagingBuffer{m_Device,
                       std::string(m_DebugName + "_IndexStagingBuffer"),
                       indexSize,
                       m_IndexCount,
                       VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                       VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                           VK_MEMORY_PROPERTY_HOST_COHERENT_BIT};

  stagingBuffer.Map();
  stagingBuffer.Write((void *)indices.data());

  m_IndexBuffer = CreateUnique<Buffer>(
      m_Device, std::string(m_DebugName + "_IndexBuffer"), indexSize,
      m_IndexCount,
      VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
      VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

  // Move data from staging buffer to index buffer
  m_Device.CopyBuffer(stagingBuffer.GetBuffer(), m_IndexBuffer->GetBuffer(),
                      bufferSize);
}

void Mesh::LoadFromData(const MeshData &meshData) {
  if (meshData.vertices.empty()) {
    AQUILA_LOG_ERROR("Mesh data contains no vertices: {}", m_DebugName);
    return;
  }

  m_Vertices = meshData.vertices;
  m_Indices = meshData.indices;
  m_Path = meshData.path;

  m_VertexCount = static_cast<uint32>(meshData.vertices.size());

  CreateVertexBuffer(meshData.vertices);

  if (!meshData.indices.empty()) {
    m_HasIndexBuffer = true;
    m_IndexCount = static_cast<uint32>(meshData.indices.size());
    CreateIndexBuffer(meshData.indices);
  } else {
    m_HasIndexBuffer = false;
    m_IndexCount = 0;
  }

  if (!m_Vertices.empty()) {
    Primitive primitive;
    primitive.firstVertex = 0;
    primitive.vertexCount = m_VertexCount;

    if (m_HasIndexBuffer) {
      primitive.firstIndex = 0;
      primitive.indexCount = m_IndexCount;
    } else {
      primitive.firstIndex = 0;
      primitive.indexCount = 0;
    }

    m_Primitives.clear();
    m_Primitives.push_back(primitive);
  }

  AQUILA_LOG_INFO("Loaded mesh data '{}' - {} vertices, {} indices",
                  m_DebugName, m_VertexCount, m_IndexCount);
}

void Mesh::Draw(VkCommandBuffer commandBuffer, VkPipelineLayout pipelineLayout,
                VkDescriptorSet descriptorSet) {
  if (!m_Primitives.empty()) {
    for (auto &primitive : m_Primitives) {
      if (m_HasIndexBuffer) {
        vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                                pipelineLayout, 0, 1, &descriptorSet, 0,
                                nullptr);
        vkCmdDrawIndexed(commandBuffer, primitive.indexCount, 1,
                         primitive.firstIndex, primitive.firstVertex, 0);
      } else {
        vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                                pipelineLayout, 0, 1, &descriptorSet, 0,
                                nullptr);
        vkCmdDraw(commandBuffer, primitive.vertexCount, 1, 0, 0);
      }
    }
  }
}

MeshData Mesh::GenerateCube(f32 size) {
  MeshData meshData;
  meshData.path = "procedural://cube";

  vec3 white = {1.0f, 1.0f, 1.0f};

  meshData.vertices = {{{-size, -size, size},
                        white,
                        {0.0f, 0.0f, 1.0f},
                        {0.0f, 0.0f},
                        {1.0f, 0.0f, 0.0f}},
                       {{size, -size, size},
                        white,
                        {0.0f, 0.0f, 1.0f},
                        {1.0f, 0.0f},
                        {1.0f, 0.0f, 0.0f}},
                       {{size, size, size},
                        white,
                        {0.0f, 0.0f, 1.0f},
                        {1.0f, 1.0f},
                        {1.0f, 0.0f, 0.0f}},
                       {{-size, size, size},
                        white,
                        {0.0f, 0.0f, 1.0f},
                        {0.0f, 1.0f},
                        {1.0f, 0.0f, 0.0f}},

                       {{size, -size, -size},
                        white,
                        {0.0f, 0.0f, -1.0f},
                        {0.0f, 0.0f},
                        {-1.0f, 0.0f, 0.0f}},
                       {{-size, -size, -size},
                        white,
                        {0.0f, 0.0f, -1.0f},
                        {1.0f, 0.0f},
                        {-1.0f, 0.0f, 0.0f}},
                       {{-size, size, -size},
                        white,
                        {0.0f, 0.0f, -1.0f},
                        {1.0f, 1.0f},
                        {-1.0f, 0.0f, 0.0f}},
                       {{size, size, -size},
                        white,
                        {0.0f, 0.0f, -1.0f},
                        {0.0f, 1.0f},
                        {-1.0f, 0.0f, 0.0f}},

                       {{-size, -size, -size},
                        white,
                        {-1.0f, 0.0f, 0.0f},
                        {0.0f, 0.0f},
                        {0.0f, 0.0f, 1.0f}},
                       {{-size, -size, size},
                        white,
                        {-1.0f, 0.0f, 0.0f},
                        {1.0f, 0.0f},
                        {0.0f, 0.0f, 1.0f}},
                       {{-size, size, size},
                        white,
                        {-1.0f, 0.0f, 0.0f},
                        {1.0f, 1.0f},
                        {0.0f, 0.0f, 1.0f}},
                       {{-size, size, -size},
                        white,
                        {-1.0f, 0.0f, 0.0f},
                        {0.0f, 1.0f},
                        {0.0f, 0.0f, 1.0f}},

                       {{size, -size, size},
                        white,
                        {1.0f, 0.0f, 0.0f},
                        {0.0f, 0.0f},
                        {0.0f, 0.0f, -1.0f}},
                       {{size, -size, -size},
                        white,
                        {1.0f, 0.0f, 0.0f},
                        {1.0f, 0.0f},
                        {0.0f, 0.0f, -1.0f}},
                       {{size, size, -size},
                        white,
                        {1.0f, 0.0f, 0.0f},
                        {1.0f, 1.0f},
                        {0.0f, 0.0f, -1.0f}},
                       {{size, size, size},
                        white,
                        {1.0f, 0.0f, 0.0f},
                        {0.0f, 1.0f},
                        {0.0f, 0.0f, -1.0f}},

                       {{-size, size, size},
                        white,
                        {0.0f, 1.0f, 0.0f},
                        {0.0f, 0.0f},
                        {1.0f, 0.0f, 0.0f}},
                       {{size, size, size},
                        white,
                        {0.0f, 1.0f, 0.0f},
                        {1.0f, 0.0f},
                        {1.0f, 0.0f, 0.0f}},
                       {{size, size, -size},
                        white,
                        {0.0f, 1.0f, 0.0f},
                        {1.0f, 1.0f},
                        {1.0f, 0.0f, 0.0f}},
                       {{-size, size, -size},
                        white,
                        {0.0f, 1.0f, 0.0f},
                        {0.0f, 1.0f},
                        {1.0f, 0.0f, 0.0f}},

                       {{-size, -size, -size},
                        white,
                        {0.0f, -1.0f, 0.0f},
                        {0.0f, 0.0f},
                        {1.0f, 0.0f, 0.0f}},
                       {{size, -size, -size},
                        white,
                        {0.0f, -1.0f, 0.0f},
                        {1.0f, 0.0f},
                        {1.0f, 0.0f, 0.0f}},
                       {{size, -size, size},
                        white,
                        {0.0f, -1.0f, 0.0f},
                        {1.0f, 1.0f},
                        {1.0f, 0.0f, 0.0f}},
                       {{-size, -size, size},
                        white,
                        {0.0f, -1.0f, 0.0f},
                        {0.0f, 1.0f},
                        {1.0f, 0.0f, 0.0f}}};

  meshData.indices = {0,  1,  2,  2,  3,  0,  4,  5,  6,  6,  7,  4,
                      8,  9,  10, 10, 11, 8,  12, 13, 14, 14, 15, 12,
                      16, 17, 18, 18, 19, 16, 20, 21, 22, 22, 23, 20};

  return meshData;
}

MeshData Mesh::GenerateSphere(f32 radius, uint32 segments, uint32 rings) {
  MeshData meshData;
  meshData.path = "procedural://sphere";

  vec3 white = {1.0f, 1.0f, 1.0f};

  for (uint32 ring = 0; ring <= rings; ring++) {
    f32 phi = Utility::Math::PI * ring / rings;
    for (uint32 segment = 0; segment <= segments; segment++) {
      f32 theta = 2.0f * Utility::Math::PI * segment / segments;

      vec3 position = {radius * sin(phi) * cos(theta), radius * cos(phi),
                       radius * sin(phi) * sin(theta)};

      vec3 normal = glm::normalize(position);
      glm::vec2 texCoord = {(f32)segment / segments, (f32)ring / rings};

      vec3 tangent = glm::normalize(vec3(-sin(theta), 0.0f, cos(theta)));

      meshData.vertices.push_back({position, white, normal, texCoord, tangent});
    }
  }

  // Generate sphere indices
  for (uint32 ring = 0; ring < rings; ring++) {
    for (uint32 segment = 0; segment < segments; segment++) {
      uint32 current = ring * (segments + 1) + segment;
      uint32 next = current + segments + 1;

      meshData.indices.push_back(current);
      meshData.indices.push_back(next);
      meshData.indices.push_back(current + 1);

      meshData.indices.push_back(current + 1);
      meshData.indices.push_back(next);
      meshData.indices.push_back(next + 1);
    }
  }

  return meshData;
}

MeshData Mesh::GenerateCylinder(f32 radius, f32 height, uint32 segments) {
  MeshData meshData;
  meshData.path = "procedural://cylinder";

  f32 halfHeight = height * 0.5f;
  vec3 white = {1.0f, 1.0f, 1.0f};

  // Center vertices for caps
  meshData.vertices.push_back({{0.0f, halfHeight, 0.0f},
                               white,
                               {0.0f, 1.0f, 0.0f},
                               {0.5f, 0.5f},
                               {1.0f, 0.0f, 0.0f}});
  meshData.vertices.push_back({{0.0f, -halfHeight, 0.0f},
                               white,
                               {0.0f, -1.0f, 0.0f},
                               {0.5f, 0.5f},
                               {1.0f, 0.0f, 0.0f}});

  // Generate side vertices
  for (uint32 i = 0; i <= segments; i++) {
    f32 angle = 2.0f * Utility::Math::PI * i / segments;
    f32 x = radius * cos(angle);
    f32 z = radius * sin(angle);

    vec3 normal = glm::normalize(vec3(x, 0.0f, z));
    vec3 tangent = glm::normalize(vec3(-sin(angle), 0.0f, cos(angle)));
    f32 u = (f32)i / segments;

    meshData.vertices.push_back(
        {{x, halfHeight, z}, white, normal, {u, 0.0f}, tangent});
    meshData.vertices.push_back(
        {{x, -halfHeight, z}, white, normal, {u, 1.0f}, tangent});

    vec3 capTangent = glm::normalize(vec3(-sin(angle), 0.0f, cos(angle)));
    meshData.vertices.push_back(
        {{x, halfHeight, z},
         white,
         {0.0f, 1.0f, 0.0f},
         {0.5f + 0.5f * cos(angle), 0.5f + 0.5f * sin(angle)},
         capTangent});
    meshData.vertices.push_back(
        {{x, -halfHeight, z},
         white,
         {0.0f, -1.0f, 0.0f},
         {0.5f + 0.5f * cos(angle), 0.5f + 0.5f * sin(angle)},
         capTangent});
  }

  for (uint32 i = 0; i < segments; i++) {
    uint32 topCurrent = 2 + i * 4;
    uint32 bottomCurrent = 3 + i * 4;
    uint32 topNext = 2 + ((i + 1) % segments) * 4;
    uint32 bottomNext = 3 + ((i + 1) % segments) * 4;

    meshData.indices.insert(meshData.indices.end(),
                            {topCurrent, bottomCurrent, topNext, topNext,
                             bottomCurrent, bottomNext});

    meshData.indices.insert(meshData.indices.end(),
                            {0, 4 + i * 4, 4 + ((i + 1) % segments) * 4});

    meshData.indices.insert(meshData.indices.end(),
                            {1, 5 + ((i + 1) % segments) * 4, 5 + i * 4});
  }

  return meshData;
}

MeshData Mesh::GeneratePlane(f32 width, f32 height, uint32 widthSegments,
                             uint32 heightSegments) {
  MeshData meshData;
  meshData.path = "procedural://plane";

  f32 halfWidth = width * 0.5f;
  f32 halfHeight = height * 0.5f;
  vec3 white = {1.0f, 1.0f, 1.0f};

  // Generate vertices
  for (uint32 y = 0; y <= heightSegments; y++) {
    for (uint32 x = 0; x <= widthSegments; x++) {
      f32 u = (f32)x / widthSegments;
      f32 v = (f32)y / heightSegments;

      vec3 position = {(u - 0.5f) * width, 0.0f, (v - 0.5f) * height};

      meshData.vertices.push_back(
          {position, white, {0.0f, 1.0f, 0.0f}, {u, v}, {1.0f, 0.0f, 0.0f}});
    }
  }

  for (uint32 y = 0; y < heightSegments; y++) {
    for (uint32 x = 0; x < widthSegments; x++) {
      uint32 topLeft = y * (widthSegments + 1) + x;
      uint32 topRight = topLeft + 1;
      uint32 bottomLeft = (y + 1) * (widthSegments + 1) + x;
      uint32 bottomRight = bottomLeft + 1;

      meshData.indices.insert(
          meshData.indices.end(),
          {topLeft, bottomLeft, topRight, topRight, bottomLeft, bottomRight});
    }
  }

  return meshData;
}

// void Mesh::Draw(VkCommandBuffer commandBuffer, VkDescriptorSet descriptorSet,
// VkPipelineLayout pipelineLayout) const {
//     if (!m_Primitives.empty()) {
//         for (auto& primitive : m_Primitives) {
//             if (m_HasIndexBuffer) {
//                 if (primitive.material.descriptorSet) {
//                     // Only bind the material descriptor set if it exists
//                     std::array<VkDescriptorSet, 2> sets{descriptorSet,
//                     primitive.material.descriptorSet};
//                     vkCmdBindDescriptorSets(commandBuffer,
//                     VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout,
//                         0, static_cast<uint32>(sets.size()), sets.data(),
//                         0, nullptr);
//                 } else {
//                     // If no material descriptor set, just bind the default
//                     descriptor set vkCmdBindDescriptorSets(commandBuffer,
//                     VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout,
//                         0, 1, &descriptorSet, 0, nullptr);
//                 }
//                 vkCmdDrawIndexed(commandBuffer, primitive.indexCount,
//                     1, primitive.firstIndex, primitive.firstVertex, 0);
//             } else {
//                 // If no index buffer, just draw the vertices
//                 vkCmdBindDescriptorSets(commandBuffer,
//                 VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout,
//                     0, 1, &descriptorSet, 0, nullptr);
//                 vkCmdDraw(commandBuffer, primitive.vertexCount, 1, 0, 0);
//             }
//         }
//     } else {
//         throw std::runtime_error("No primitives found");
//     }
// }

void Mesh::Bind(VkCommandBuffer commandBuffer) const {
  const VkBuffer buffers[] = {m_VertexBuffer->GetBuffer()};
  constexpr VkDeviceSize offsets[] = {0};
  vkCmdBindVertexBuffers(commandBuffer, 0, 1, buffers, offsets);

  if (m_HasIndexBuffer)
    vkCmdBindIndexBuffer(commandBuffer, m_IndexBuffer->GetBuffer(), 0,
                         VK_INDEX_TYPE_UINT32);
}

}; // namespace Engine
