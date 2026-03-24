#ifndef BUFFER_H
#define BUFFER_H

#include "Aquila/Core/AquilaCore.h"
#include "Aquila/Graphics/Core/DeletionManager.h"
#include "Aquila/Graphics/Core/Device.h"

namespace Aquila::Graphics::Resources {

using namespace Aquila::Foundation;

class Buffer {
  public:
	Buffer(Device &device, const std::string &debugName, VkDeviceSize instanceSize, uint32_t instanceCount,
		   VkBufferUsageFlags usageFlags, VkMemoryPropertyFlags memoryPropertyFlags,
		   VkDeviceSize minOffsetAlignment = 1);
	~Buffer();

	Buffer(const Buffer &) = delete;
	Buffer &operator=(const Buffer &) = delete;

	VkResult Map(VkDeviceSize size = VK_WHOLE_SIZE, VkDeviceSize offset = 0);
	void UnMap();

	void Write(const void *data, VkDeviceSize size = VK_WHOLE_SIZE, VkDeviceSize offset = 0) const;
	[[nodiscard]] VkResult Flush(VkDeviceSize size = VK_WHOLE_SIZE, VkDeviceSize offset = 0) const;
	[[nodiscard]] VkDescriptorBufferInfo DescriptorInfo(VkDeviceSize size = VK_WHOLE_SIZE,
														VkDeviceSize offset = 0) const;
	[[nodiscard]] VkResult Invalidate(VkDeviceSize size = VK_WHOLE_SIZE, VkDeviceSize offset = 0) const;

	void WriteToIndex(const void *data, int index) const;
	[[nodiscard]] VkResult FlushIndex(int index) const;
	[[nodiscard]] VkDescriptorBufferInfo DescriptorInfoForIndex(int index) const;
	[[nodiscard]] VkResult InvalidateIndex(int index) const;

	[[nodiscard]] VkBuffer GetBuffer() const { return m_Buffer; }
	[[nodiscard]] void *GetMappedMemory() const { return m_Mapped; }
	[[nodiscard]] uint32_t GetInstanceCount() const { return m_InstanceCount; }
	[[nodiscard]] VkDeviceSize GetInstanceSize() const { return m_InstanceSize; }
	[[nodiscard]] VkDeviceSize GetAlignmentSize() const { return m_InstanceSize; }
	[[nodiscard]] VkBufferUsageFlags GetUsageFlags() const { return m_UsageFlags; }
	[[nodiscard]] VkMemoryPropertyFlags GetMemoryPropertyFlags() const { return m_MemoryPropertyFlags; }
	[[nodiscard]] VkDeviceSize GetBufferSize() const { return m_BufferSize; }

	void DestroyImmediate();

  private:
	static VkDeviceSize GetAlignment(VkDeviceSize instanceSize, VkDeviceSize minOffsetAlignment);

	Device &m_Device;
	void *m_Mapped = nullptr;
	VkBuffer m_Buffer = VK_NULL_HANDLE;
	VkDeviceMemory m_Memory = VK_NULL_HANDLE;

	VkDeviceSize m_BufferSize;
	uint32_t m_InstanceCount;
	VkDeviceSize m_InstanceSize;
	VkDeviceSize m_AlignmentSize;
	VkBufferUsageFlags m_UsageFlags;
	VkMemoryPropertyFlags m_MemoryPropertyFlags;
};

}; // namespace Aquila::Graphics::Resources

#endif // Buffer_H
