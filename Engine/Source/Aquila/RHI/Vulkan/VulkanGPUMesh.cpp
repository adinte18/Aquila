#include "Aquila/RHI/Vulkan/VulkanGPUMesh.h"
#include "Aquila/RHI/Vulkan/VulkanCommandList.h"
#include "Aquila/RHI/Vulkan/VulkanDevice.h"

namespace Aquila::RHI {

VulkanGPUMesh::VulkanGPUMesh(VulkanDevice &device, const GPUMeshDesc &desc)
	: m_Device(device), m_DebugName(desc.debugName) {
	if (desc.primitives) {
		m_Primitives = *desc.primitives;
	}

	if (desc.vertices && !desc.vertices->empty()) {
		m_VertexCount = static_cast<uint32>(desc.vertices->size());
		UploadVertexBuffer(*desc.vertices);
	}

	if (desc.indices && !desc.indices->empty()) {
		m_IndexCount = static_cast<uint32>(desc.indices->size());
		m_HasIndexBuffer = true;
		UploadIndexBuffer(*desc.indices);
	}

	if (m_Primitives.empty() && m_VertexCount > 0) {
		GPUMeshPrimitive prim{};
		prim.firstVertex = 0;
		prim.vertexCount = m_VertexCount;
		prim.firstIndex = 0;
		prim.indexCount = m_IndexCount;
		m_Primitives.push_back(prim);
	}
}

VulkanGPUMesh::~VulkanGPUMesh() {
	if (m_VertexAllocation.IsValid()) {
		vmaDestroyBuffer(m_Device.GetAllocator(), m_VertexAllocation.buffer, m_VertexAllocation.allocation);
	}
	if (m_IndexAllocation.IsValid()) {
		vmaDestroyBuffer(m_Device.GetAllocator(), m_IndexAllocation.buffer, m_IndexAllocation.allocation);
	}
}

void VulkanGPUMesh::UploadVertexBuffer(const std::vector<Vertex> &vertices) {
	VkDeviceSize size = sizeof(Vertex) * vertices.size();

	auto staging = m_Device.CreateBuffer<MemoryDomain::CPU_TO_GPU>(size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
																   (m_DebugName + "_VtxStaging").c_str());

	memcpy(staging.mappedPtr, vertices.data(), size);

	m_VertexAllocation = m_Device.CreateBuffer<MemoryDomain::GPU_ONLY>(
		size, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, (m_DebugName + "_Vtx").c_str());

	m_Device.ExecuteTransferCommands([&](VkCommandBuffer cmd) {
		VkBufferCopy copy{ 0, 0, size };
		vkCmdCopyBuffer(cmd, staging.buffer, m_VertexAllocation.buffer, 1, &copy);
	});

	vmaDestroyBuffer(m_Device.GetAllocator(), staging.buffer, staging.allocation);
}

void VulkanGPUMesh::UploadIndexBuffer(const std::vector<uint32> &indices) {
	VkDeviceSize size = sizeof(uint32) * indices.size();

	auto staging = m_Device.CreateBuffer<MemoryDomain::CPU_TO_GPU>(size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
																   (m_DebugName + "_IdxStaging").c_str());

	memcpy(staging.mappedPtr, indices.data(), size);

	m_IndexAllocation = m_Device.CreateBuffer<MemoryDomain::GPU_ONLY>(
		size, VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, (m_DebugName + "_Idx").c_str());

	m_Device.ExecuteTransferCommands([&](VkCommandBuffer cmd) {
		VkBufferCopy copy{ 0, 0, size };
		vkCmdCopyBuffer(cmd, staging.buffer, m_IndexAllocation.buffer, 1, &copy);
	});

	vmaDestroyBuffer(m_Device.GetAllocator(), staging.buffer, staging.allocation);
}

void VulkanGPUMesh::Bind(IRHICommandList &cmd) const {
	auto &vkCmd = static_cast<VulkanCommandList &>(cmd);
	VkBuffer buf = m_VertexAllocation.buffer;
	VkDeviceSize offset = 0;
	vkCmdBindVertexBuffers(vkCmd.GetHandle(), 0, 1, &buf, &offset);
	if (m_HasIndexBuffer) {
		vkCmdBindIndexBuffer(vkCmd.GetHandle(), m_IndexAllocation.buffer, 0, VK_INDEX_TYPE_UINT32);
	}
}

void VulkanGPUMesh::Draw(IRHICommandList &cmd) const {
	auto &vkCmd = static_cast<VulkanCommandList &>(cmd);
	for (const auto &prim : m_Primitives) {
		if (m_HasIndexBuffer) {
			vkCmdDrawIndexed(vkCmd.GetHandle(), prim.indexCount, 1, prim.firstIndex, prim.firstVertex, 0);
		} else {
			vkCmdDraw(vkCmd.GetHandle(), prim.vertexCount, 1, prim.firstVertex, 0);
		}
	}
}

} // namespace Aquila::RHI
