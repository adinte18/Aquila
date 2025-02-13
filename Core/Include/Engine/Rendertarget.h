#pragma once
#include "Engine/Device.h"

namespace Engine {
    class RenderTarget {
    public:
        VkImage colorImage{};            // Color image
        VkImageView colorImageView{};    // Color image view
        VkSampler colorSampler{};        // Sampler for color image
        VkDeviceMemory colorMemory{};    // Memory for color image
        
        VkImage depthImage{};            // Depth image
        VkImageView depthImageView{};    // Depth image view
        VkSampler depthSampler{};        // Depth sampler
        VkDeviceMemory depthMemory{};    // Depth image memory

        enum class AttachmentType {
            COLOR,
            DEPTH,
            BOTH
        };

        RenderTarget(Device& dev, uint32_t width, uint32_t height, VkFormat colorFormat, VkImageUsageFlags colorUsage,
                     VkFormat depthFormat, VkImageUsageFlags depthUsage, AttachmentType attachmentType = AttachmentType::BOTH)
            : device(dev) {
            CreateRenderTarget(width, height, colorFormat, colorUsage, depthFormat, depthUsage, attachmentType);
        }

        ~RenderTarget() {
            // Clean up the color attachment
            if (colorImageView != VK_NULL_HANDLE) {
                vkDestroyImageView(device.vk_GetDevice(), colorImageView, nullptr);
            }
            if (colorImage != VK_NULL_HANDLE) {
                vkDestroyImage(device.vk_GetDevice(), colorImage, nullptr);
            }
            if (colorMemory != VK_NULL_HANDLE) {
                vkFreeMemory(device.vk_GetDevice(), colorMemory, nullptr);
            }

            // Clean up depth attachment
            if (depthImageView != VK_NULL_HANDLE) {
                vkDestroyImageView(device.vk_GetDevice(), depthImageView, nullptr);
            }
            if (depthImage != VK_NULL_HANDLE) {
                vkDestroyImage(device.vk_GetDevice(), depthImage, nullptr);
            }
            if (depthMemory != VK_NULL_HANDLE) {
                vkFreeMemory(device.vk_GetDevice(), depthMemory, nullptr);
            }
        }

    private:
        Device& device;

        void CreateRenderTarget(const uint32_t width, const uint32_t height, VkFormat colorFormat, VkImageUsageFlags colorUsage,
                                VkFormat depthFormat, VkImageUsageFlags depthUsage, AttachmentType attachmentType) {
            if (attachmentType == AttachmentType::COLOR || attachmentType == AttachmentType::BOTH) {
                CreateColorAttachment(width, height, colorFormat, colorUsage);
            }

            if (attachmentType == AttachmentType::DEPTH || attachmentType == AttachmentType::BOTH) {
                CreateDepthAttachment(width, height, depthFormat, depthUsage);
            }
        }

        void CreateColorAttachment(const uint32_t width, const uint32_t height, VkFormat format, VkImageUsageFlags usage) {
            VkImageCreateInfo imageInfo{};
            imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
            imageInfo.imageType = VK_IMAGE_TYPE_2D;
            imageInfo.extent.width = width;
            imageInfo.extent.height = height;
            imageInfo.extent.depth = 1;
            imageInfo.mipLevels = 1;
            imageInfo.arrayLayers = 1;
            imageInfo.format = format;
            imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
            imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            imageInfo.usage = usage | VK_IMAGE_USAGE_SAMPLED_BIT;
            imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
            imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

            if (vkCreateImage(device.vk_GetDevice(), &imageInfo, nullptr, &colorImage) != VK_SUCCESS) {
                throw std::runtime_error("Failed to create color attachment image!");
            }

            VkMemoryRequirements memRequirements;
            vkGetImageMemoryRequirements(device.vk_GetDevice(), colorImage, &memRequirements);

            VkMemoryAllocateInfo allocInfo{};
            allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
            allocInfo.allocationSize = memRequirements.size;
            allocInfo.memoryTypeIndex = device.vk_FindMemoryType(memRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

            if (vkAllocateMemory(device.vk_GetDevice(), &allocInfo, nullptr, &colorMemory) != VK_SUCCESS) {
                throw std::runtime_error("Failed to allocate color attachment memory!");
            }

            vkBindImageMemory(device.vk_GetDevice(), colorImage, colorMemory, 0);

            // Create Image View for Color
            VkImageViewCreateInfo viewInfo{};
            viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
            viewInfo.image = colorImage;
            viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
            viewInfo.format = format;
            viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            viewInfo.subresourceRange.baseMipLevel = 0;
            viewInfo.subresourceRange.levelCount = 1;
            viewInfo.subresourceRange.baseArrayLayer = 0;
            viewInfo.subresourceRange.layerCount = 1;

            if (vkCreateImageView(device.vk_GetDevice(), &viewInfo, nullptr, &colorImageView) != VK_SUCCESS) {
                throw std::runtime_error("Failed to create color image view!");
            }

            // Create Sampler for Color Image
            VkSamplerCreateInfo samplerCreateInfo{};
            samplerCreateInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
            samplerCreateInfo.magFilter = VK_FILTER_LINEAR;
            samplerCreateInfo.minFilter = VK_FILTER_LINEAR;
            samplerCreateInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
            samplerCreateInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
            samplerCreateInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
            samplerCreateInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
            samplerCreateInfo.anisotropyEnable = VK_FALSE;
            samplerCreateInfo.maxAnisotropy = 1.0f;
            samplerCreateInfo.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
            samplerCreateInfo.unnormalizedCoordinates = VK_FALSE;

            if (vkCreateSampler(device.vk_GetDevice(), &samplerCreateInfo, nullptr, &colorSampler) != VK_SUCCESS) {
                throw std::runtime_error("Failed to create color image sampler!");
            }
        }

        void CreateDepthAttachment(const uint32_t width, const uint32_t height, VkFormat format, VkImageUsageFlags usage) {
            VkImageCreateInfo imageInfo{};
            imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
            imageInfo.imageType = VK_IMAGE_TYPE_2D;
            imageInfo.extent.width = width;
            imageInfo.extent.height = height;
            imageInfo.extent.depth = 1;
            imageInfo.mipLevels = 1;
            imageInfo.arrayLayers = 1;
            imageInfo.format = format;
            imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
            imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

            imageInfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;

            imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
            imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

            if (vkCreateImage(device.vk_GetDevice(), &imageInfo, nullptr, &depthImage) != VK_SUCCESS) {
                throw std::runtime_error("Failed to create depth attachment image!");
            }

            VkMemoryRequirements memRequirements;
            vkGetImageMemoryRequirements(device.vk_GetDevice(), depthImage, &memRequirements);

            VkMemoryAllocateInfo allocInfo{};
            allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
            allocInfo.allocationSize = memRequirements.size;
            allocInfo.memoryTypeIndex = device.vk_FindMemoryType(memRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

            if (vkAllocateMemory(device.vk_GetDevice(), &allocInfo, nullptr, &depthMemory) != VK_SUCCESS) {
                throw std::runtime_error("Failed to allocate depth attachment memory!");
            }

            vkBindImageMemory(device.vk_GetDevice(), depthImage, depthMemory, 0);

            // Create Depth Image View
            VkImageViewCreateInfo viewInfo{};
            viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
            viewInfo.image = depthImage;
            viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
            viewInfo.format = format;
            viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
            viewInfo.subresourceRange.baseMipLevel = 0;
            viewInfo.subresourceRange.levelCount = 1;
            viewInfo.subresourceRange.baseArrayLayer = 0;
            viewInfo.subresourceRange.layerCount = 1;

            if (vkCreateImageView(device.vk_GetDevice(), &viewInfo, nullptr, &depthImageView) != VK_SUCCESS) {
                throw std::runtime_error("Failed to create depth image view!");
            }

            VkSamplerCreateInfo samplerCreateInfo{};
            samplerCreateInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
            samplerCreateInfo.magFilter = VK_FILTER_NEAREST;
            samplerCreateInfo.minFilter = VK_FILTER_NEAREST;
            samplerCreateInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
            samplerCreateInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
            samplerCreateInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
            samplerCreateInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
            samplerCreateInfo.anisotropyEnable = VK_FALSE;
            samplerCreateInfo.maxAnisotropy = 1.0f;
            samplerCreateInfo.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_BLACK;
            samplerCreateInfo.unnormalizedCoordinates = VK_FALSE;

            if (vkCreateSampler(device.vk_GetDevice(), &samplerCreateInfo, nullptr, &depthSampler) != VK_SUCCESS) {
                throw std::runtime_error("Failed to create depth image sampler!");
            }
        }
    };
}
