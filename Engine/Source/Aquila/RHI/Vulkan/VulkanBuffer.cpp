#include "Aquila/RHI/Vulkan/VulkanBuffer.h"
#include "Aquila/Foundation/Macros.h"
#include "Aquila/RHI/Vulkan/VulkanDeletionQueue.h"
#include "Aquila/RHI/Vulkan/VulkanDevice.h"

namespace Aquila::RHI {

VulkanBuffer::VulkanBuffer(VulkanDevice &device, const std::string &debugName, VkDeviceSize instanceSize,
						   uint32_t instanceCount, VkBufferUsageFlags usageFlags, MemoryDomain domain,
						   VkDeviceSize minOffsetAlignment)
	: m_Device(device), m_InstanceSize(instanceSize), m_AlignmentSize(GetAlignment(instanceSize, minOffsetAlignment)),
	  m_InstanceCount(instanceCount), m_UsageFlags(usageFlags), m_Domain(domain) {
	m_BufferSize = m_AlignmentSize * instanceCount;

	auto alloc = [&]() -> BufferAllocation {
		switch (m_Domain) {
		case MemoryDomain::GPU_ONLY:
			return device.CreateBuffer<MemoryDomain::GPU_ONLY>(m_BufferSize, usageFlags, debugName.c_str());
		case MemoryDomain::CPU_TO_GPU:
			return device.CreateBuffer<MemoryDomain::CPU_TO_GPU>(m_BufferSize, usageFlags, debugName.c_str());
		case MemoryDomain::GPU_TO_CPU:
			return device.CreateBuffer<MemoryDomain::GPU_TO_CPU>(m_BufferSize, usageFlags, debugName.c_str());
		case MemoryDomain::CPU_ONLY:
			return device.CreateBuffer<MemoryDomain::CPU_ONLY>(m_BufferSize, usageFlags, debugName.c_str());
		default:
			return device.CreateBuffer<MemoryDomain::GPU_ONLY>(m_BufferSize, usageFlags, debugName.c_str());
		}
	}();

	AQUILA_ASSERT(alloc.IsValid(), "Buffer allocation failed");

	m_Buffer = alloc.buffer;
	m_Allocation = alloc.allocation;
	m_MappedPtr = alloc.mappedPtr;
	m_PersistentMap = (m_MappedPtr != nullptr);
}

VulkanBuffer::~VulkanBuffer() {
	Unmap();
	m_Device.GetDeletionQueue().QueueDeletion(
		RHI::Deletion::VmaBufferDeletion{ .buffer = m_Buffer, .allocation = m_Allocation });
	m_Buffer = VK_NULL_HANDLE;
	m_Allocation = VK_NULL_HANDLE;
}

// IRHIBuffer
void VulkanBuffer::Write(const void *data, uint64 size, uint64 offset) {
	AQUILA_ASSERT(m_MappedPtr, "Cannot write to unmapped buffer");

	if (size == 0) {
		memcpy(m_MappedPtr, data, m_BufferSize);
	} else {
		char *dst = static_cast<char *>(m_MappedPtr) + offset;
		memcpy(dst, data, size);
	}
}

void *VulkanBuffer::Map() {
	if (m_PersistentMap) {
		return m_MappedPtr;
	}

	AQUILA_ASSERT(m_Buffer && m_Allocation, "Map called before buffer creation");
	vmaMapMemory(m_Device.GetAllocator(), m_Allocation, &m_MappedPtr);
	return m_MappedPtr;
}

void VulkanBuffer::Unmap() {
	if ((m_MappedPtr != nullptr) && !m_PersistentMap) {
		vmaUnmapMemory(m_Device.GetAllocator(), m_Allocation);
		m_MappedPtr = nullptr;
	}
}

void VulkanBuffer::DestroyImmediate() {
	Unmap();
	auto *allocator = m_Device.GetAllocator();
	vmaDestroyBuffer(allocator, m_Buffer, m_Allocation);
	m_Buffer = VK_NULL_HANDLE;
	m_Allocation = VK_NULL_HANDLE;
}

void VulkanBuffer::Flush(uint64 size, uint64 offset) {
	vmaFlushAllocation(m_Device.GetAllocator(), m_Allocation, static_cast<VkDeviceSize>(offset),
					   size == 0 ? VK_WHOLE_SIZE : static_cast<VkDeviceSize>(size));
}

// Extended Vulkan API
VkResult VulkanBuffer::Flush(VkDeviceSize size, VkDeviceSize offset) const {
	return vmaFlushAllocation(m_Device.GetAllocator(), m_Allocation, offset, size);
}

VkResult VulkanBuffer::Invalidate(VkDeviceSize size, VkDeviceSize offset) const {
	return vmaInvalidateAllocation(m_Device.GetAllocator(), m_Allocation, offset, size);
}

VkDescriptorBufferInfo VulkanBuffer::DescriptorInfo(VkDeviceSize size, VkDeviceSize offset) const {
	return VkDescriptorBufferInfo{ m_Buffer, offset, size };
}

VkDescriptorBufferInfo VulkanBuffer::DescriptorInfoForIndex(int index) const {
	return DescriptorInfo(m_AlignmentSize, index * m_AlignmentSize);
}

void VulkanBuffer::WriteToIndex(const void *data, int index) const {
	AQUILA_ASSERT(m_MappedPtr, "Cannot write to unmapped buffer");
	char *dst = static_cast<char *>(m_MappedPtr) + (index * m_AlignmentSize);
	memcpy(dst, data, m_InstanceSize);
}

VkResult VulkanBuffer::FlushIndex(int index) const {
	return Flush(m_AlignmentSize, index * m_AlignmentSize);
}

VkResult VulkanBuffer::InvalidateIndex(int index) const {
	return Invalidate(m_AlignmentSize, index * m_AlignmentSize);
}

VkDeviceSize VulkanBuffer::GetAlignment(VkDeviceSize instanceSize, VkDeviceSize minOffsetAlignment) {
	if (minOffsetAlignment > 0) {
		return (instanceSize + minOffsetAlignment - 1) & ~(minOffsetAlignment - 1);
	}
	return instanceSize;
}

} // namespace Aquila::RHI
