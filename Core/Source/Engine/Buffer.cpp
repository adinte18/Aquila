/*
 * Encapsulates a vulkan buffer
 *
 * Initially based off VulkanBuffer by Sascha Willems -
 * https://github.com/SaschaWillems/Vulkan/blob/master/base/VulkanBuffer.h
 */
#include "AquilaCore.h"

#include "Engine/Buffer.h"

#include <cstring>

namespace Engine {

/**
 * Returns the minimum instance size required to be compatible with devices minOffsetAlignment
 *
 * @param instanceSize The size of an instance
 * @param minOffsetAlignment The minimum required alignment, in bytes, for the offset member (eg
 * minUniformBufferOffsetAlignment)
 *
 * @return VkResult of the buffer mapping call
 */
VkDeviceSize Buffer::vk_GetAlignment(VkDeviceSize instanceSize, VkDeviceSize minOffsetAlignment) {
  if (minOffsetAlignment > 0) {
    return (instanceSize + minOffsetAlignment - 1) & ~(minOffsetAlignment - 1);
  }
  return instanceSize;
}

Buffer::Buffer(
    Device &device,
    VkDeviceSize instanceSize,
    uint32_t instanceCount,
    VkBufferUsageFlags usageFlags,
    VkMemoryPropertyFlags memoryPropertyFlags,
    VkDeviceSize minOffsetAlignment)
    : m_Device{device},
      m_InstanceSize{instanceSize},
      m_InstanceCount{instanceCount},
      m_UsageFlags{usageFlags},
      m_MemoryPropertyFlags{memoryPropertyFlags} {
  m_AlignmentSize = vk_GetAlignment(instanceSize, minOffsetAlignment);
  m_BufferSize = m_AlignmentSize * instanceCount;
  device.CreateBuffer(m_BufferSize, usageFlags, memoryPropertyFlags, m_Buffer, m_Memory);
}

Buffer::~Buffer() {
  unmap();
  vkDestroyBuffer(m_Device.vk_GetDevice(), m_Buffer, nullptr);
  vkFreeMemory(m_Device.vk_GetDevice(), m_Memory, nullptr);
}

/**
 * Map a memory range of this buffer. If successful, mapped points to the specified buffer range.
 *
 * @param size (Optional) Size of the memory range to map. Pass VK_WHOLE_SIZE to map the complete
 * buffer range.
 * @param offset (Optional) Byte offset from beginning
 *
 * @return VkResult of the buffer mapping call
 */
VkResult Buffer::map(VkDeviceSize size, VkDeviceSize offset) {
  AQUILA_CORE_ASSERT(m_Buffer && m_Memory && "Called map on buffer before create");
  return vkMapMemory(m_Device.vk_GetDevice(), m_Memory, offset, size, 0, &m_Mapped);
}

/**
 * Unmap a mapped memory range
 *
 * @note Does not return a result as vkUnmapMemory can't fail
 */
void Buffer::unmap() {
  if (m_Mapped) {
    vkUnmapMemory(m_Device.vk_GetDevice(), m_Memory);
    m_Mapped = nullptr;
  }
}

/**
 * Copies the specified data to the mapped buffer. Default value writes whole buffer range
 *
 * @param data Pointer to the data to copy
 * @param size (Optional) Size of the data to copy. Pass VK_WHOLE_SIZE to flush the complete buffer
 * range.
 * @param offset (Optional) Byte offset from beginning of mapped region
 *
 */
void Buffer::vk_WriteToBuffer(void *data, VkDeviceSize size, VkDeviceSize offset) {
  AQUILA_CORE_ASSERT(m_Mapped && "Cannot copy to unmapped buffer");

  if (size == VK_WHOLE_SIZE) {
    memcpy(m_Mapped, data, m_BufferSize);
  } else {
    char *memOffset = (char *)m_Mapped;
    memOffset += offset;
    memcpy(memOffset, data, size);
  }
}

/**
 * Flush a memory range of the buffer to make it visible to the device
 *
 * @note Only required for non-coherent memory
 *
 * @param size (Optional) Size of the memory range to flush. Pass VK_WHOLE_SIZE to flush the
 * complete buffer range.
 * @param offset (Optional) Byte offset from beginning
 *
 * @return VkResult of the flush call
 */
VkResult Buffer::vk_Flush(VkDeviceSize size, VkDeviceSize offset) {
  VkMappedMemoryRange mappedRange = {};
  mappedRange.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
  mappedRange.memory = m_Memory;
  mappedRange.offset = offset;
  mappedRange.size = size;
  return vkFlushMappedMemoryRanges(m_Device.vk_GetDevice(), 1, &mappedRange);
}

/**
 * Invalidate a memory range of the buffer to make it visible to the host
 *
 * @note Only required for non-coherent memory
 *
 * @param size (Optional) Size of the memory range to invalidate. Pass VK_WHOLE_SIZE to invalidate
 * the complete buffer range.
 * @param offset (Optional) Byte offset from beginning
 *
 * @return VkResult of the invalidate call
 */
VkResult Buffer::vk_Invalidate(VkDeviceSize size, VkDeviceSize offset) {
  VkMappedMemoryRange mappedRange = {};
  mappedRange.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
  mappedRange.memory = m_Memory;
  mappedRange.offset = offset;
  mappedRange.size = size;
  return vkInvalidateMappedMemoryRanges(m_Device.vk_GetDevice(), 1, &mappedRange);
}

/**
 * Create a buffer info descriptor
 *
 * @param size (Optional) Size of the memory range of the descriptor
 * @param offset (Optional) Byte offset from beginning
 *
 * @return VkDescriptorBufferInfo of specified offset and range
 */
VkDescriptorBufferInfo Buffer::vk_DescriptorInfo(VkDeviceSize size, VkDeviceSize offset) {
  return VkDescriptorBufferInfo{
      m_Buffer,
      offset,
      size,
  };
}

/**
 * Copies "instanceSize" bytes of data to the mapped buffer at an offset of index * alignmentSize
 *
 * @param data Pointer to the data to copy
 * @param index Used in offset calculation
 *
 */
void Buffer::vk_WriteToIndex(void *data, int index) {
  vk_WriteToBuffer(data, m_InstanceSize, index * m_AlignmentSize);
}

/**
 *  Flush the memory range at index * alignmentSize of the buffer to make it visible to the device
 *
 * @param index Used in offset calculation
 *
 */
VkResult Buffer::vk_FlushIndex(int index) { return vk_Flush(m_AlignmentSize, index * m_AlignmentSize); }

/**
 * Create a buffer info descriptor
 *
 * @param index Specifies the region given by index * alignmentSize
 *
 * @return VkDescriptorBufferInfo for instance at index
 */
VkDescriptorBufferInfo Buffer::vk_DescriptorInfoForIndex(int index) {
  return vk_DescriptorInfo(m_AlignmentSize, index * m_AlignmentSize);
}

/**
 * Invalidate a memory range of the buffer to make it visible to the host
 *
 * @note Only required for non-coherent memory
 *
 * @param index Specifies the region to invalidate: index * alignmentSize
 *
 * @return VkResult of the invalidate call
 */
VkResult Buffer::vk_InvalidateIndex(int index) {
  return vk_Invalidate(m_AlignmentSize, index * m_AlignmentSize);
}

}  // namespace lve
