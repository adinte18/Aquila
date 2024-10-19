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

        [[nodiscard]] VkBuffer vk_GetBuffer() const { return buffer; }
        [[nodiscard]] void* vk_GetMappedMemory() const { return mapped; }
        [[nodiscard]] uint32_t vk_GetInstanceCount() const { return instanceCount; }
        [[nodiscard]] VkDeviceSize vk_GetInstanceSize() const { return instanceSize; }
        [[nodiscard]] VkDeviceSize vk_GetAlignmentSize() const { return instanceSize; }
        [[nodiscard]] VkBufferUsageFlags vk_GetUsageFlags() const { return usageFlags; }
        [[nodiscard]] VkMemoryPropertyFlags vk_GetMemoryPropertyFlags() const { return memoryPropertyFlags; }
        [[nodiscard]] VkDeviceSize vk_GetBufferSize() const { return bufferSize; }

    private:
        static VkDeviceSize vk_GetAlignment(VkDeviceSize instanceSize, VkDeviceSize minOffsetAlignment);

        Device& lveDevice;
        void* mapped = nullptr;
        VkBuffer buffer = VK_NULL_HANDLE;
        VkDeviceMemory memory = VK_NULL_HANDLE;

        VkDeviceSize bufferSize;
        uint32_t instanceCount;
        VkDeviceSize instanceSize;
        VkDeviceSize alignmentSize;
        VkBufferUsageFlags usageFlags;
        VkMemoryPropertyFlags memoryPropertyFlags;
    };

};

#endif //Buffer_H
