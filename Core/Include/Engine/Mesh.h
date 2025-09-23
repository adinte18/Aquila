//
// Created by adinte on 7/5/24.
//

#ifndef VK_APP_VK_MODEL_H
#define VK_APP_VK_MODEL_H

#include "AquilaCore.h"

#include "Engine/Renderer/Buffer.h"
#include "Engine/Renderer/Device.h"
#include "Engine/Renderer/Vertex.h"

#include "assimp/Importer.hpp"
#include "assimp/postprocess.h"
#include "assimp/scene.h"

namespace Engine {

struct MeshData {
  std::vector<Vertex> vertices;
  std::vector<uint32> indices;
  std::string path;

  void Load(const std::string &filepath);
};

class Mesh {
  struct Primitive {
    uint32 firstIndex;
    uint32 firstVertex;
    uint32 indexCount;
    uint32 vertexCount;
  };

private:
  void CreateVertexBuffer(const std::vector<Vertex> &vertices);
  void CreateIndexBuffer(const std::vector<uint32> &indices);

  std::string m_DebugName;

  Device &m_Device;
  Unique<Buffer> m_VertexBuffer;
  uint32 m_VertexCount;

  bool m_HasIndexBuffer = false;
  Unique<Buffer> m_IndexBuffer;
  uint32 m_IndexCount;

  std::vector<Primitive> m_Primitives{};

  std::string m_Path;

  std::vector<Vertex> m_Vertices{};
  std::vector<uint32> m_Indices{};

public:
  Mesh(Device &device, const std::string &debugName);
  ~Mesh();

  Mesh(const Mesh &) = delete;
  Mesh &operator=(const Mesh &) = delete;

  void UpdateVertexBuffer(std::vector<Vertex> &vertices) const;

  void Load(const std::string &filepath);
  void LoadFromData(const MeshData &meshData);

  MeshData GenerateCube(f32 size);
  MeshData GenerateSphere(f32 radius = 1.0f, uint32 segments = 32,
                          uint32 rings = 16);
  MeshData GenerateCylinder(f32 radius = 1.0f, f32 height = 2.0f,
                            uint32 segments = 32);
  MeshData GeneratePlane(f32 width = 2.0f, f32 height = 2.0f,
                         uint32 widthSegments = 1, uint32 heightSegments = 1);

  void Bind(VkCommandBuffer commandBuffer) const;
  void Draw(VkCommandBuffer commandBuffer, VkPipelineLayout pipelineLayout,
            VkDescriptorSet descriptorSet);

  [[nodiscard]] std::string GetPath() const { return m_Path; }
  [[nodiscard]] std::string GetDebugName() const { return m_DebugName; }
};
} // namespace Engine

#endif // VK_APP_VK_MODEL_H
