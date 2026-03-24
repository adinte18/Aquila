#ifndef AQUILA_MESH_H
#define AQUILA_MESH_H

#include "Aquila/Graphics/Core/Device.h"
#include "Aquila/Graphics/Core/DeletionManager.h"
#include "Aquila/Graphics/Resources/Buffer.h"
#include "Aquila/Graphics/Resources/Vertex.h"

#include "assimp/Importer.hpp"
#include "assimp/postprocess.h"
#include "assimp/scene.h"

namespace Aquila::Graphics::Resources {

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
	void CreateVertexBufferWithCmd(VkCommandBuffer cmd, const std::vector<Vertex> &vertices);
	void CreateIndexBufferWithCmd(VkCommandBuffer cmd, const std::vector<uint32> &indices);

	bool m_GPUResourcesCreated = false;

	std::string m_DebugName;

	Device &m_Device;

	Unique<Buffer> m_VertexBuffer;
	uint32 m_VertexCount;

	bool m_HasIndexBuffer = false;
	Unique<Buffer> m_IndexBuffer;
	uint32 m_IndexCount;

	std::vector<Primitive> m_Primitives;

	std::string m_Path;

	std::vector<Vertex> m_Vertices;
	std::vector<uint32> m_Indices;

	Unique<Buffer> m_VertexStagingBuffer;
	Unique<Buffer> m_IndexStagingBuffer;

  public:
	Mesh(Device &device, const std::string &debugName);
	~Mesh();

	Mesh(const Mesh &) = delete;
	Mesh &operator=(const Mesh &) = delete;

	void UpdateVertexBuffer(const std::vector<Vertex> &vertices) const;
	void UploadToGPU(VkCommandBuffer cmd);
	void Load(const std::string &filepath);
	void LoadData(const std::string &filepath);
	void LoadFromData(const MeshData &meshData);

	void CenterMeshAtOrigin();

	void CleanupStagingResources() {
		m_VertexStagingBuffer.reset();
		m_IndexStagingBuffer.reset();
	}

	static MeshData GenerateCube(f32 size);
	static MeshData GenerateSphere(f32 radius, uint32 segments, uint32 rings);
	static MeshData GenerateCylinder(f32 radius, f32 height, uint32 segments);
	static MeshData GeneratePlane(f32 width, f32 height, uint32 widthSegments, uint32 heightSegments);

	void Bind(VkCommandBuffer commandBuffer) const;
	void Draw(VkCommandBuffer commandBuffer);

	[[nodiscard]] std::string GetPath() const { return m_Path; }
	[[nodiscard]] std::string GetDebugName() const { return m_DebugName; }

	[[nodiscard]] std::vector<Vertex> GetVertices() const { return m_Vertices; }
	[[nodiscard]] std::vector<uint32> GetIndices() const { return m_Indices; }

	[[nodiscard]] uint32 GetVertexCount() const { return m_VertexCount; }
	[[nodiscard]] uint32 GetIndexCount() const { return m_IndexCount; }

	[[nodiscard]] VkBuffer GetVertexBuffer() const { return m_VertexBuffer->GetBuffer(); }
	[[nodiscard]] VkBuffer GetIndexBuffer() const { return m_IndexBuffer->GetBuffer(); }
	[[nodiscard]] bool HasIndexBuffer() const { return m_HasIndexBuffer; }

	[[nodiscard]] std::vector<Primitive> GetPrimitives() const { return m_Primitives; }
};
} // namespace Aquila::Graphics::Resources

#endif // AQUILA_MESH_H
