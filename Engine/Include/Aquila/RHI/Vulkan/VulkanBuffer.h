#ifndef AQUILA_VULKAN_BUFFER_H
#define AQUILA_VULKAN_BUFFER_H

#include "GraphicsPCH.h"
#include "Aquila/Foundation/PrimitiveTypes.h"
#include "Aquila/RHI/Backend/IRHIBuffer.h"
#include "Aquila/RHI/Vulkan/VulkanTypes.h"

namespace Aquila::RHI {

class VulkanDevice;

class VulkanBuffer final : public IRHIBuffer {
  public:
	VulkanBuffer(VulkanDevice &device, const std::string &debugName, VkDeviceSize instanceSize, uint32_t instanceCount,
				 VkBufferUsageFlags usageFlags, MemoryDomain domain, VkDeviceSize minOffsetAlignment);
	~VulkanBuffer() override;

	AQUILA_NONCOPYABLE(VulkanBuffer);
	AQUILA_NONMOVEABLE(VulkanBuffer);

	// IRHIBuffer
	void Write(const void *data, uint64 size, uint64 offset = 0) override;
	void *Map() override;
	void Unmap() override;
	void Flush(uint64 size = 0, uint64 offset = 0) override;

	void DestroyImmediate() override;

	[[nodiscard]] uint64 GetSize() const override { return m_BufferSize; }
	[[nodiscard]] uint32 GetInstanceCount() const override { return m_InstanceCount; }
	[[nodiscard]] bool IsMapped() const override { return m_MappedPtr != nullptr; }

	// Extended API (used internally by other Vulkan classes)
	[[nodiscard]] VkResult Flush(VkDeviceSize size = VK_WHOLE_SIZE, VkDeviceSize offset = 0) const;
	[[nodiscard]] VkResult Invalidate(VkDeviceSize size = VK_WHOLE_SIZE, VkDeviceSize offset = 0) const;
	[[nodiscard]] VkDescriptorBufferInfo DescriptorInfo(VkDeviceSize size = VK_WHOLE_SIZE,
														VkDeviceSize offset = 0) const;
	[[nodiscard]] VkDescriptorBufferInfo DescriptorInfoForIndex(int index) const;
	void WriteToIndex(const void *data, int index) const;
	[[nodiscard]] VkResult FlushIndex(int index) const;
	[[nodiscard]] VkResult InvalidateIndex(int index) const;

	[[nodiscard]] VkBuffer GetBuffer() const { return m_Buffer; }
	[[nodiscard]] void *GetMappedMemory() const { return m_MappedPtr; }
	[[nodiscard]] VkDeviceSize GetInstanceSize() const { return m_InstanceSize; }
	[[nodiscard]] VkDeviceSize GetAlignmentSize() const { return m_AlignmentSize; }
	[[nodiscard]] VkBufferUsageFlags GetUsageFlags() const { return m_UsageFlags; }

	static VkDeviceSize GetAlignment(VkDeviceSize instanceSize, VkDeviceSize minOffsetAlignment);

  private:
	VulkanDevice &m_Device;
	VkBuffer m_Buffer = VK_NULL_HANDLE;
	VmaAllocation m_Allocation = VK_NULL_HANDLE;
	void *m_MappedPtr = nullptr;
	bool m_PersistentMap = false;

	VkDeviceSize m_BufferSize = 0;
	VkDeviceSize m_InstanceSize = 0;
	VkDeviceSize m_AlignmentSize = 0;
	uint32_t m_InstanceCount = 0;
	VkBufferUsageFlags m_UsageFlags = 0;
	MemoryDomain m_Domain;
};

} // namespace Aquila::RHI
#endif
