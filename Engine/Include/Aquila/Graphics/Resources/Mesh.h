#ifndef AQUILA_MESH_H
#define AQUILA_MESH_H

#include "Aquila/RHI/Vertex.h"
#include "Aquila/RHI/GPUMesh.h"
#include "Aquila/Platform/Filesystem/VirtualFileSystem.h"

#include "Aquila/Foundation/Math/Math.h"

namespace Aquila::Graphics::Resources {

struct MeshData {
	std::vector<RHI::Vertex> vertices;
	std::vector<uint32> indices;
	std::string path;
};

class Mesh {
  public:
	explicit Mesh(const std::string &debugName);
	~Mesh() = default;

	AQUILA_NONCOPYABLE(Mesh);

	// Loading — CPU side only, no GPU involvement
	void Load(const std::string &filepath);
	void LoadFromData(const MeshData &meshData);

	// CPU-side transforms
	void CenterMeshAtOrigin();

	// Procedural generators
	static MeshData GenerateCube(f32 size);
	static MeshData GenerateSphere(f32 radius, uint32 segments, uint32 rings);
	static MeshData GenerateCylinder(f32 radius, f32 height, uint32 segments);
	static MeshData GeneratePlane(f32 width, f32 height, uint32 widthSegments, uint32 heightSegments);

	[[nodiscard]] RHI::GPUMeshDesc ToGPUDesc() const {
		RHI::GPUMeshDesc desc;
		desc.vertices = &m_Vertices;
		desc.indices = m_HasIndexBuffer ? &m_Indices : nullptr;
		desc.primitives = &m_Primitives;
		desc.debugName = m_DebugName;
		return desc;
	}

	// Getters
	[[nodiscard]] const std::string &GetPath() const { return m_Path; }
	[[nodiscard]] const std::string &GetDebugName() const { return m_DebugName; }
	[[nodiscard]] const std::vector<RHI::Vertex> &GetVertices() const { return m_Vertices; }
	[[nodiscard]] const std::vector<uint32> &GetIndices() const { return m_Indices; }
	[[nodiscard]] const std::vector<RHI::GPUMeshPrimitive> &GetPrimitives() const { return m_Primitives; }
	[[nodiscard]] uint32 GetVertexCount() const { return m_VertexCount; }
	[[nodiscard]] uint32 GetIndexCount() const { return m_IndexCount; }
	[[nodiscard]] bool HasIndexBuffer() const { return m_HasIndexBuffer; }

  private:
	std::string m_DebugName;
	std::string m_Path;

	std::vector<RHI::Vertex> m_Vertices;
	std::vector<uint32> m_Indices;
	std::vector<RHI::GPUMeshPrimitive> m_Primitives;

	uint32 m_VertexCount = 0;
	uint32 m_IndexCount = 0;
	bool m_HasIndexBuffer = false;
};

} // namespace Aquila::Graphics::Resources
#endif
