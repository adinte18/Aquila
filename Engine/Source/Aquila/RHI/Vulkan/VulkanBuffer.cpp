#include "Aquila/RHI/Vulkan/VulkanBuffer.h"
#include "Aquila/Foundation/Macros.h"
#include "Aquila/RHI/Vulkan/VulkanDeletionQueue.h"
#include "Aquila/RHI/Vulkan/VulkanDevice.h"

namespace Aquila::RHI {

VulkanBuffer::VulkanBuffer(VulkanDevice &device, const std::string &debug_name, VkDeviceSize instance_size,
						   uint32_t instance_count, VkBufferUsageFlags usage_flags, MemoryDomain domain,
						   VkDeviceSize min_offset_alignment)
	: m_device(device), m_instance_size(instance_size), m_alignment_size(get_alignment(instance_size, min_offset_alignment)),
	  m_instance_count(instance_count), m_usage_flags(usage_flags), m_domain(domain) {
	m_buffer_size = m_alignment_size * instance_count;

	auto alloc = [&]() -> BufferAllocation {
		switch (m_domain) {
		case MemoryDomain::GpuOnly:
			return device.CreateBuffer<MemoryDomain::GpuOnly>(m_buffer_size, usage_flags, debug_name.c_str());
		case MemoryDomain::CpuToGpu:
			return device.CreateBuffer<MemoryDomain::CpuToGpu>(m_buffer_size, usage_flags, debug_name.c_str());
		case MemoryDomain::GpuToCpu:
			return device.CreateBuffer<MemoryDomain::GpuToCpu>(m_buffer_size, usage_flags, debug_name.c_str());
		case MemoryDomain::CpuOnly:
			return device.CreateBuffer<MemoryDomain::CpuOnly>(m_buffer_size, usage_flags, debug_name.c_str());
		default:
			return device.CreateBuffer<MemoryDomain::GpuOnly>(m_buffer_size, usage_flags, debug_name.c_str());
		}
	}();

	AQUILA_ASSERT(alloc.is_valid(), "Buffer allocation failed");

	m_buffer = alloc.buffer;
	m_allocation = alloc.allocation;
	m_mapped_ptr = alloc.mapped_ptr;
	m_persistent_map = (m_mapped_ptr != nullptr);
}

VulkanBuffer::~VulkanBuffer() {
	unmap();
	m_device.get_deletion_queue().queue_deletion(
		RHI::Deletion::VmaBufferDeletion{ .buffer = m_buffer, .allocation = m_allocation });
	m_buffer = VK_NULL_HANDLE;
	m_allocation = VK_NULL_HANDLE;
}

// IRHIBuffer
void VulkanBuffer::write(const void *data, Uint64 size, Uint64 offset) {
	AQUILA_ASSERT(m_mapped_ptr, "Cannot write to unmapped buffer");

	if (size == 0) {
		memcpy(m_mapped_ptr, data, m_buffer_size);
	} else {
		char *dst = static_cast<char *>(m_mapped_ptr) + offset;
		memcpy(dst, data, size);
	}
}

void *VulkanBuffer::map() {
	if (m_persistent_map) {
		return m_mapped_ptr;
	}

	AQUILA_ASSERT(m_buffer && m_allocation, "Map called before buffer creation");
	vmaMapMemory(m_device.get_allocator(), m_allocation, &m_mapped_ptr);
	return m_mapped_ptr;
}

void VulkanBuffer::unmap() {
	if ((m_mapped_ptr != nullptr) && !m_persistent_map) {
		vmaUnmapMemory(m_device.get_allocator(), m_allocation);
		m_mapped_ptr = nullptr;
	}
}

void VulkanBuffer::destroy_immediate() {
	unmap();
	auto *allocator = m_device.get_allocator();
	vmaDestroyBuffer(allocator, m_buffer, m_allocation);
	m_buffer = VK_NULL_HANDLE;
	m_allocation = VK_NULL_HANDLE;
}

void VulkanBuffer::flush(Uint64 size, Uint64 offset) {
	vmaFlushAllocation(m_device.get_allocator(), m_allocation, static_cast<VkDeviceSize>(offset),
					   size == 0 ? VK_WHOLE_SIZE : static_cast<VkDeviceSize>(size));
}

// Extended Vulkan API
VkResult VulkanBuffer::Flush(VkDeviceSize size, VkDeviceSize offset) const {
	return vmaFlushAllocation(m_device.get_allocator(), m_allocation, offset, size);
}

VkResult VulkanBuffer::invalidate(VkDeviceSize size, VkDeviceSize offset) const {
	return vmaInvalidateAllocation(m_device.get_allocator(), m_allocation, offset, size);
}

VkDescriptorBufferInfo VulkanBuffer::descriptor_info(VkDeviceSize size, VkDeviceSize offset) const {
	return VkDescriptorBufferInfo{ m_buffer, offset, size };
}

VkDescriptorBufferInfo VulkanBuffer::descriptor_info_for_index(int index) const {
	return descriptor_info(m_alignment_size, index * m_alignment_size);
}

void VulkanBuffer::write_to_index(const void *data, int index) const {
	AQUILA_ASSERT(m_mapped_ptr, "Cannot write to unmapped buffer");
	char *dst = static_cast<char *>(m_mapped_ptr) + (index * m_alignment_size);
	memcpy(dst, data, m_instance_size);
}

VkResult VulkanBuffer::flush_index(int index) const {
	return Flush(m_alignment_size, index * m_alignment_size);
}

VkResult VulkanBuffer::invalidate_index(int index) const {
	return invalidate(m_alignment_size, index * m_alignment_size);
}

VkDeviceSize VulkanBuffer::get_alignment(VkDeviceSize instance_size, VkDeviceSize min_offset_alignment) {
	if (min_offset_alignment > 0) {
		return (instance_size + min_offset_alignment - 1) & ~(min_offset_alignment - 1);
	}
	return instance_size;
}

} // namespace Aquila::RHI
