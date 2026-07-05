#include "Aquila/RHI/Vulkan/VulkanGPUMesh.h"
#include "Aquila/RHI/Vulkan/VulkanCommandList.h"
#include "Aquila/RHI/Vulkan/VulkanDevice.h"

namespace Aquila::RHI {

VulkanGPUMesh::VulkanGPUMesh(VulkanDevice &device, const GPUMeshDesc &desc)
	: m_device(device), m_debug_name(desc.debug_name) {
	if (desc.primitives) {
		m_primitives = *desc.primitives;
	}

	if (desc.vertices && !desc.vertices->empty()) {
		m_vertex_count = static_cast<Uint32>(desc.vertices->size());
		upload_vertex_buffer(*desc.vertices);
	}

	if (desc.indices && !desc.indices->empty()) {
		m_index_count = static_cast<Uint32>(desc.indices->size());
		m_has_index_buffer = true;
		upload_index_buffer(*desc.indices);
	}

	if (m_primitives.empty() && m_vertex_count > 0) {
		GPUMeshPrimitive prim{};
		prim.first_vertex = 0;
		prim.vertex_count = m_vertex_count;
		prim.first_index = 0;
		prim.index_count = m_index_count;
		m_primitives.push_back(prim);
	}
}

VulkanGPUMesh::~VulkanGPUMesh() {
	if (m_vertex_allocation.is_valid()) {
		vmaDestroyBuffer(m_device.get_allocator(), m_vertex_allocation.buffer, m_vertex_allocation.allocation);
	}
	if (m_index_allocation.is_valid()) {
		vmaDestroyBuffer(m_device.get_allocator(), m_index_allocation.buffer, m_index_allocation.allocation);
	}
}

void VulkanGPUMesh::upload_vertex_buffer(const std::vector<Vertex> &vertices) {
	VkDeviceSize size = sizeof(Vertex) * vertices.size();

	auto staging = m_device.CreateBuffer<MemoryDomain::CpuToGpu>(size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
																   (m_debug_name + "_VtxStaging").c_str());

	memcpy(staging.mapped_ptr, vertices.data(), size);

	m_vertex_allocation = m_device.CreateBuffer<MemoryDomain::GpuOnly>(
		size, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, (m_debug_name + "_Vtx").c_str());

	m_device.execute_transfer_commands([&](VkCommandBuffer cmd) {
		VkBufferCopy copy{ 0, 0, size };
		vkCmdCopyBuffer(cmd, staging.buffer, m_vertex_allocation.buffer, 1, &copy);
	});

	vmaDestroyBuffer(m_device.get_allocator(), staging.buffer, staging.allocation);
}

void VulkanGPUMesh::upload_index_buffer(const std::vector<Uint32> &indices) {
	VkDeviceSize size = sizeof(Uint32) * indices.size();

	auto staging = m_device.CreateBuffer<MemoryDomain::CpuToGpu>(size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
																   (m_debug_name + "_IdxStaging").c_str());

	memcpy(staging.mapped_ptr, indices.data(), size);

	m_index_allocation = m_device.CreateBuffer<MemoryDomain::GpuOnly>(
		size, VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, (m_debug_name + "_Idx").c_str());

	m_device.execute_transfer_commands([&](VkCommandBuffer cmd) {
		VkBufferCopy copy{ 0, 0, size };
		vkCmdCopyBuffer(cmd, staging.buffer, m_index_allocation.buffer, 1, &copy);
	});

	vmaDestroyBuffer(m_device.get_allocator(), staging.buffer, staging.allocation);
}

void VulkanGPUMesh::bind(IRHICommandList &cmd) const {
	auto &vk_cmd = static_cast<VulkanCommandList &>(cmd);
	VkBuffer buf = m_vertex_allocation.buffer;
	VkDeviceSize offset = 0;
	vkCmdBindVertexBuffers(vk_cmd.get_handle(), 0, 1, &buf, &offset);
	if (m_has_index_buffer) {
		vkCmdBindIndexBuffer(vk_cmd.get_handle(), m_index_allocation.buffer, 0, VK_INDEX_TYPE_UINT32);
	}
}

void VulkanGPUMesh::draw(IRHICommandList &cmd) const {
	auto &vk_cmd = static_cast<VulkanCommandList &>(cmd);
	for (const auto &prim : m_primitives) {
		if (m_has_index_buffer) {
			vkCmdDrawIndexed(vk_cmd.get_handle(), prim.index_count, 1, prim.first_index, prim.first_vertex, 0);
		} else {
			vkCmdDraw(vk_cmd.get_handle(), prim.vertex_count, 1, prim.first_vertex, 0);
		}
	}
}

} // namespace Aquila::RHI
