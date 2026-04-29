#ifndef AQUILA_VULKAN_GPU_MESH_H
#define AQUILA_VULKAN_GPU_MESH_H

#include "GraphicsPCH.h"
#include "Aquila/Foundation/PrimitiveTypes.h"
#include "Aquila/RHI/Backend/IRHIGPUMesh.h"
#include "Aquila/RHI/Vulkan/VulkanTypes.h"
#include "Aquila/RHI/Vulkan/VulkanVertex.h"

namespace Aquila::RHI {
class VulkanDevice;
class VulkanCommandList;

class VulkanGPUMesh final : public IRHIGPUMesh {
  public:
	VulkanGPUMesh(VulkanDevice &device, const GPUMeshDesc &desc);
	~VulkanGPUMesh() override;
	AQUILA_NONCOPYABLE(VulkanGPUMesh);
	AQUILA_NONMOVEABLE(VulkanGPUMesh);

	// IRHIGPUMesh
	void Bind(IRHICommandList &cmd) const override;
	void Draw(IRHICommandList &cmd) const override;

	[[nodiscard]] bool IsValid() const override { return m_VertexAllocation.IsValid(); }
	[[nodiscard]] uint32 GetVertexCount() const override { return m_VertexCount; }
	[[nodiscard]] uint32 GetIndexCount() const override { return m_IndexCount; }
	[[nodiscard]] bool HasIndexBuffer() const override { return m_HasIndexBuffer; }

  private:
	void UploadVertexBuffer(const std::vector<Vertex> &vertices);
	void UploadIndexBuffer(const std::vector<uint32> &indices);

	VulkanDevice &m_Device;
	std::string m_DebugName;
	BufferAllocation m_VertexAllocation{};
	BufferAllocation m_IndexAllocation{};
	std::vector<GPUMeshPrimitive> m_Primitives;
	uint32 m_VertexCount = 0;
	uint32 m_IndexCount = 0;
	bool m_HasIndexBuffer = false;
};

} // namespace Aquila::RHI
#endif
