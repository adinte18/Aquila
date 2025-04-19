#pragma once

#ifndef Buffer_H
#define Buffer_H

#include <cstdint>

#include "Common.h"
#include "Engine/Device.h"

namespace Engine
{
    class Buffer
    {
    public:
        Buffer(
            Device& device,
            VkDeviceSize instanceSize,
            uint32_t instanceCount,
            VkBufferUsageFlags usageFlags,
            VkMemoryPropertyFlags memoryPropertyFlags,
            VkDeviceSize minOffsetAlignment = 1);
        ~Buffer();

        Buffer(const Buffer&) = delete;
        Buffer& operator=(const Buffer&) = delete;

        VkResult map(VkDeviceSize size = VK_WHOLE_SIZE, VkDeviceSize offset = 0);
        void unmap();

        void vk_WriteToBuffer(void* data, VkDeviceSize size = VK_WHOLE_SIZE, VkDeviceSize offset = 0);
        VkResult vk_Flush(VkDeviceSize size = VK_WHOLE_SIZE, VkDeviceSize offset = 0);
        VkDescriptorBufferInfo vk_DescriptorInfo(VkDeviceSize size = VK_WHOLE_SIZE, VkDeviceSize offset = 0);
        VkResult vk_Invalidate(VkDeviceSize size = VK_WHOLE_SIZE, VkDeviceSize offset = 0);

        void vk_WriteToIndex(void* data, int index);
        VkResult vk_FlushIndex(int index);
        VkDescriptorBufferInfo vk_DescriptorInfoForIndex(int index);
        VkResult vk_InvalidateIndex(int index);

        [[nodiscard]] VkBuffer vk_GetBuffer() const { return m_Buffer; }
        [[nodiscard]] void* vk_GetMappedMemory() const { return m_Mapped; }
        [[nodiscard]] uint32_t vk_GetInstanceCount() const { return m_InstanceCount; }
        [[nodiscard]] VkDeviceSize vk_GetInstanceSize() const { return m_InstanceSize; }
        [[nodiscard]] VkDeviceSize vk_GetAlignmentSize() const { return m_InstanceSize; }
        [[nodiscard]] VkBufferUsageFlags vk_GetUsageFlags() const { return m_UsageFlags; }
        [[nodiscard]] VkMemoryPropertyFlags vk_GetMemoryPropertyFlags() const { return m_MemoryPropertyFlags; }
        [[nodiscard]] VkDeviceSize vk_GetBufferSize() const { return m_BufferSize; }

    private:
        static VkDeviceSize vk_GetAlignment(VkDeviceSize instanceSize, VkDeviceSize minOffsetAlignment);

        Device& m_Device;
        void* m_Mapped = nullptr;
        VkBuffer m_Buffer = VK_NULL_HANDLE;
        VkDeviceMemory m_Memory = VK_NULL_HANDLE;

        VkDeviceSize m_BufferSize;
        uint32_t m_InstanceCount;
        VkDeviceSize m_InstanceSize;
        VkDeviceSize m_AlignmentSize;
        VkBufferUsageFlags m_UsageFlags;
        VkMemoryPropertyFlags m_MemoryPropertyFlags;
    };

};

#endif //Buffer_H
