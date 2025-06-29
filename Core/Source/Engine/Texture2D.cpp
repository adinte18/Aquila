#include "Engine/Texture2D.h"
#include "Engine/DescriptorAllocator.h"
#include "Engine/Device.h"
#include <Engine/Buffer.h>


#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"


namespace Engine {
    Texture2D::Texture2D(Device& device) : m_Device(device){
        textureImage = VK_NULL_HANDLE;
        textureSampler = VK_NULL_HANDLE;
        textureImageView = VK_NULL_HANDLE;
        textureImageMemory = VK_NULL_HANDLE;

        descriptorSetLayout = DescriptorSetLayout::Builder(m_Device)
            .addBinding(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT)
            .build();

        DescriptorAllocator::Allocate(descriptorSetLayout->GetDescriptorSetLayout(), descriptorSet);
    };

    Texture2D::~Texture2D() {
        DestroyAll();
    }


    void Texture2D::DestroyAll() {
        if (textureImageView != VK_NULL_HANDLE) {
            vkDestroyImageView(m_Device.vk_GetDevice(), textureImageView, nullptr);
            textureImageView = VK_NULL_HANDLE;
        }
        if (textureImage != VK_NULL_HANDLE) {
            vkDestroyImage(m_Device.vk_GetDevice(), textureImage, nullptr);
            textureImage = VK_NULL_HANDLE;
        }
        if (textureSampler != VK_NULL_HANDLE) {
            vkDestroySampler(m_Device.vk_GetDevice(), textureSampler, nullptr);
            textureSampler = VK_NULL_HANDLE;
        }
        if (textureImageMemory != VK_NULL_HANDLE) {
            vkFreeMemory(m_Device.vk_GetDevice(), textureImageMemory, nullptr);
            textureImageMemory = VK_NULL_HANDLE;
        }
    }

    void Texture2D::Destroy() {
        VkDevice device = m_Device.vk_GetDevice();
        if (textureSampler != VK_NULL_HANDLE) {
            vkDestroySampler(device, textureSampler, nullptr);
            textureSampler = VK_NULL_HANDLE;
        }
        if (textureImageView != VK_NULL_HANDLE) {
            vkDestroyImageView(device, textureImageView, nullptr);
            textureImageView = VK_NULL_HANDLE;
        }
        if (textureImage != VK_NULL_HANDLE) {
            vkDestroyImage(device, textureImage, nullptr);
            textureImage = VK_NULL_HANDLE;
        }
        if (textureImageMemory != VK_NULL_HANDLE) {
            vkFreeMemory(device, textureImageMemory, nullptr);
            textureImageMemory = VK_NULL_HANDLE;
        }

        if (descriptorSet != VK_NULL_HANDLE) {
            std::vector<VkDescriptorSet> sets{ descriptorSet };
            DescriptorAllocator::Release(sets);
            descriptorSet = VK_NULL_HANDLE;
        }

        if (descriptorSetLayout) {
            descriptorSetLayout.reset();
        }
    }


    void Texture2D::CreateHDRTexture(const std::string &filepath, VkFormat format) {
        vk_CreateHDRTextureImage(filepath);
        vk_CreateHDRTextureImageView(format);
        vk_CreateHDRTextureSampler();
        vk_WriteToDescriptorSet();
    }

    void Texture2D::CreateCubeMap(uint32_t width, uint32_t height, VkFormat format, VkImageUsageFlags usage) {
        vk_CreateCubemapImage(width, height, format, usage);
        vk_CreateCubemapImageView(format);
        vk_CreateCubemapSampler(); // maybe i dont need this, i wont sample it for IBL i guess
        vk_WriteToDescriptorSet();
    }

    void Texture2D::GenerateMipmap(VkImage image, VkFormat imageFormat, int32_t texWidth, int32_t texHeight, uint32_t mipLevels){
            VkFormatProperties formatProperties;
        vkGetPhysicalDeviceFormatProperties(m_Device.vk_GetPhysicalDevice(), imageFormat, &formatProperties);
        if (!(formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT)) {
            throw std::runtime_error("Texture image format does not support linear blitting!");
        }

        VkCommandBuffer commandBuffer = m_Device.vk_BeginSingleTimeCommands();

        VkImageMemoryBarrier barrier{};
        barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barrier.image = image;
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        barrier.subresourceRange.baseArrayLayer = 0;
        barrier.subresourceRange.layerCount = 1;
        barrier.subresourceRange.levelCount = 1;

        int32_t mipWidth = texWidth;
        int32_t mipHeight = texHeight;

        for (uint32_t i = 1; i < mipLevels; i++) {
            barrier.subresourceRange.baseMipLevel = i - 1;
            barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
            barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
            barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

            vkCmdPipelineBarrier(commandBuffer,
                VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0,
                0, nullptr,
                0, nullptr,
                1, &barrier);

            VkImageBlit blit{};
            blit.srcOffsets[0] = {0, 0, 0};
            blit.srcOffsets[1] = {mipWidth, mipHeight, 1};
            blit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            blit.srcSubresource.mipLevel = i - 1;
            blit.srcSubresource.baseArrayLayer = 0;
            blit.srcSubresource.layerCount = 1;
            blit.dstOffsets[0] = {0, 0, 0};
            blit.dstOffsets[1] = { mipWidth > 1 ? mipWidth / 2 : 1, mipHeight > 1 ? mipHeight / 2 : 1, 1 };
            blit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            blit.dstSubresource.mipLevel = i;
            blit.dstSubresource.baseArrayLayer = 0;
            blit.dstSubresource.layerCount = 1;

            vkCmdBlitImage(commandBuffer,
                image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                1, &blit,
                VK_FILTER_LINEAR);

            barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
            barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
            barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

            vkCmdPipelineBarrier(commandBuffer,
                VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
                0, nullptr,
                0, nullptr,
                1, &barrier);

            if (mipWidth > 1) mipWidth /= 2;
            if (mipHeight > 1) mipHeight /= 2;
        }

        barrier.subresourceRange.baseMipLevel = mipLevels - 1;
        barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

        vkCmdPipelineBarrier(commandBuffer,
            VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
            0, nullptr,
            0, nullptr,
            1, &barrier);

        m_Device.vk_EndSingleTimeCommands(commandBuffer);
    }

    void Texture2D::CreateMipMappedCubemap(uint32_t width, uint32_t height, VkFormat format,
        VkImageUsageFlags usage) {
        m_MipLevels = static_cast<uint32_t>(std::floor(std::log2(std::max(width, height)))) + 1;

        VkImageCreateInfo imageInfo{};
        imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        imageInfo.imageType = VK_IMAGE_TYPE_2D;
        imageInfo.format = VK_FORMAT_R32G32B32A32_SFLOAT;
        imageInfo.extent.width = width;
        imageInfo.extent.height = height;
        imageInfo.extent.depth = 1;
        imageInfo.mipLevels = m_MipLevels;
        imageInfo.arrayLayers = 6;
        imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
        imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
        imageInfo.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
        imageInfo.flags = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;
        imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

        if (vkCreateImage(m_Device.vk_GetDevice(), &imageInfo, nullptr, &textureImage) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create cubemap image!");
        }

        VkMemoryRequirements memRequirements;
        vkGetImageMemoryRequirements(m_Device.vk_GetDevice(), textureImage, &memRequirements);

        VkMemoryAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocInfo.allocationSize = memRequirements.size;
        allocInfo.memoryTypeIndex = m_Device.vk_FindMemoryType(memRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

        if (vkAllocateMemory(m_Device.vk_GetDevice(), &allocInfo, nullptr, &textureImageMemory) != VK_SUCCESS) {
            throw std::runtime_error("Failed to allocate cubemap memory!");
        }

        vkBindImageMemory(m_Device.vk_GetDevice(), textureImage, textureImageMemory, 0);

        VkImageViewCreateInfo viewInfo{};
        viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        viewInfo.image = textureImage;
        viewInfo.viewType = VK_IMAGE_VIEW_TYPE_CUBE;
        viewInfo.format = VK_FORMAT_R32G32B32A32_SFLOAT;
        viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        viewInfo.subresourceRange.baseMipLevel = 0;
        viewInfo.subresourceRange.levelCount = m_MipLevels;
        viewInfo.subresourceRange.baseArrayLayer = 0;
        viewInfo.subresourceRange.layerCount = 6;

        if (vkCreateImageView(m_Device.vk_GetDevice(), &viewInfo, nullptr, &textureImageView) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create cubemap image view!");
        }

        VkSamplerCreateInfo samplerInfo{};
        samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
        samplerInfo.magFilter = VK_FILTER_LINEAR;
        samplerInfo.minFilter = VK_FILTER_LINEAR;
        samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
        samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        samplerInfo.minLod = 0.0f;
        samplerInfo.maxLod = static_cast<float>(m_MipLevels);
        samplerInfo.mipLodBias = 0.0f;
        samplerInfo.anisotropyEnable = VK_FALSE;

        if (vkCreateSampler(m_Device.vk_GetDevice(), &samplerInfo, nullptr, &textureSampler) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create cubemap sampler!");
        }

        vk_WriteToDescriptorSet();

        std::cout << "Created mip-mapped cubemap" << std::endl;
    }


    void Texture2D::CreateTexture(const std::string& filepath, VkFormat format) {
        vk_CreateTextureImage(filepath);
        vk_CreateTextureImageView(format);
        vk_CreateTextureSampler();
        vk_WriteToDescriptorSet();
    }

    void Texture2D::CreateTexture(const uint32_t width, const uint32_t height,
                                            const VkFormat format, const VkImageUsageFlags usage, const VkSampleCountFlagBits samples) {
        vk_CreateTextureImage(width, height, format, usage, samples);
        vk_CreateTextureImageView(format);
        vk_CreateTextureSampler();
        vk_WriteToDescriptorSet();
    }


    void Texture2D::vk_WriteToDescriptorSet() {
        VkDescriptorImageInfo imageInfo{};
        imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        imageInfo.imageView = textureImageView;
        imageInfo.sampler = textureSampler;

        DescriptorWriter writer(*descriptorSetLayout, *DescriptorAllocator::GetSharedPool());
        writer.writeImage(0, &imageInfo);
        writer.overwrite(descriptorSet);
    }

    VkDescriptorImageInfo Texture2D::GetDescriptorSetInfo() const {
        VkDescriptorImageInfo imageInfo{};
        imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        imageInfo.imageView = GetTextureImageView();
        imageInfo.sampler = GetTextureSampler();
        return imageInfo;
    }

    void Texture2D::TransitionImageLayout(
        VkImage image,
        VkFormat format,
        VkImageLayout oldLayout,
        VkImageLayout newLayout,
        uint32_t layers)
    {
        VkCommandBuffer commandBuffer = m_Device.vk_BeginSingleTimeCommands();

        VkImageMemoryBarrier barrier{};
        barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barrier.oldLayout = oldLayout;
        barrier.newLayout = newLayout;
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.image = image;
        barrier.subresourceRange.aspectMask = (format == VK_FORMAT_D32_SFLOAT) ? VK_IMAGE_ASPECT_DEPTH_BIT : VK_IMAGE_ASPECT_COLOR_BIT;
        barrier.subresourceRange.baseMipLevel = 0;
        barrier.subresourceRange.levelCount = m_MipLevels;
        barrier.subresourceRange.baseArrayLayer = 0;
        barrier.subresourceRange.layerCount = layers;

        VkPipelineStageFlags sourceStage;
        VkPipelineStageFlags destinationStage;

        switch (oldLayout) {
            case VK_IMAGE_LAYOUT_UNDEFINED:
                barrier.srcAccessMask = 0;
                sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
                break;
            case VK_IMAGE_LAYOUT_PREINITIALIZED:
                barrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT;
                sourceStage = VK_PIPELINE_STAGE_HOST_BIT;
                break;
            case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
                barrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
                sourceStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
                break;
            case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
                barrier.srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
                sourceStage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
                break;
            case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
                barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
                sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
                break;
            case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
                barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
                sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
                break;
            case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
                barrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
                sourceStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
                break;
            default:
                throw std::invalid_argument("unsupported layout transition!");
        }

        switch (newLayout) {
            case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
                barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
                destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
                break;
            case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
                barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
                destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
                break;
            case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
                barrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
                destinationStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
                break;
            case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
                barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
                destinationStage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
                break;
            case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
                barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
                destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
                break;
            default:
                throw std::invalid_argument("unsupported layout transition!");
        }

        vkCmdPipelineBarrier(
            commandBuffer,
            sourceStage, destinationStage,
            0,
            0, nullptr,
            0, nullptr,
            1, &barrier
        );

        m_Device.vk_EndSingleTimeCommands(commandBuffer);
    }

    void Texture2D::vk_CreateCubemapImage(uint32_t width, uint32_t height, VkFormat format, VkImageUsageFlags usage) {
        VkImageCreateInfo imageInfo{};
        imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        imageInfo.flags = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;
        imageInfo.imageType = VK_IMAGE_TYPE_2D;
        imageInfo.extent.width = width;
        imageInfo.extent.height = height;
        imageInfo.extent.depth = 1;
        imageInfo.mipLevels = 1;
        imageInfo.arrayLayers = 6;
        imageInfo.format = format;
        imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
        imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        imageInfo.usage = usage | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
        imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
        imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        if (vkCreateImage(m_Device.vk_GetDevice(), &imageInfo, nullptr, &textureImage) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create cubemap image!");
        }

        VkMemoryRequirements memRequirements;
        vkGetImageMemoryRequirements(m_Device.vk_GetDevice(), textureImage, &memRequirements);

        VkMemoryAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocInfo.allocationSize = memRequirements.size;
        allocInfo.memoryTypeIndex = m_Device.vk_FindMemoryType(memRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

        if (vkAllocateMemory(m_Device.vk_GetDevice(), &allocInfo, nullptr, &textureImageMemory) != VK_SUCCESS) {
            throw std::runtime_error("Failed to allocate cubemap memory!");
        }

        vkBindImageMemory(m_Device.vk_GetDevice(), textureImage, textureImageMemory, 0);

        TransitionImageLayout(textureImage, VK_FORMAT_R32G32B32A32_SFLOAT,
                        VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, 6);

    }

    void Texture2D::vk_CreateCubemapImageView(VkFormat format) {
        VkImageViewCreateInfo viewInfo{};
        viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        viewInfo.image = textureImage;
        viewInfo.viewType = VK_IMAGE_VIEW_TYPE_CUBE; // CUBEMAP view type
        viewInfo.format = format;
        viewInfo.subresourceRange.aspectMask = (format == VK_FORMAT_D32_SFLOAT) ? VK_IMAGE_ASPECT_DEPTH_BIT : VK_IMAGE_ASPECT_COLOR_BIT;
        viewInfo.subresourceRange.baseMipLevel = 0;
        viewInfo.subresourceRange.levelCount = 1;
        viewInfo.subresourceRange.baseArrayLayer = 0;
        viewInfo.subresourceRange.layerCount = 6;

        if (vkCreateImageView(m_Device.vk_GetDevice(), &viewInfo, nullptr, &textureImageView) != VK_SUCCESS) {
            throw std::runtime_error("failed to create cubemap image view!");
        }
    }
    void Texture2D::vk_CreateCubemapSampler() {
        VkPhysicalDeviceProperties properties{};
        vkGetPhysicalDeviceProperties(m_Device.vk_GetPhysicalDevice(), &properties);

        VkSamplerCreateInfo samplerInfo{};
        samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
        samplerInfo.magFilter = VK_FILTER_LINEAR;
        samplerInfo.minFilter = VK_FILTER_LINEAR;
        samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        samplerInfo.anisotropyEnable = VK_TRUE;
        samplerInfo.maxAnisotropy = properties.limits.maxSamplerAnisotropy;
        samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
        samplerInfo.unnormalizedCoordinates = VK_FALSE;
        samplerInfo.compareEnable = VK_FALSE;
        samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
        samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;

        if (vkCreateSampler(m_Device.vk_GetDevice(), &samplerInfo, nullptr, &textureSampler) != VK_SUCCESS) {
            throw std::runtime_error("failed to create cubemap sampler!");
        }
    }


    void Texture2D::CreateImage(uint32_t width, uint32_t height, VkFormat format,
        VkImageTiling tiling,
        VkImageUsageFlags usage,
        VkMemoryPropertyFlags properties,
        VkImage& image,
        VkDeviceMemory& imageMemory) {

        m_MipLevels = static_cast<uint32_t>(std::floor(std::log2(std::max(width, height)))) + 1;

        VkImageCreateInfo imageInfo{};
        imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        imageInfo.imageType = VK_IMAGE_TYPE_2D;
        imageInfo.extent.width = width;
        imageInfo.extent.height = height;
        imageInfo.extent.depth = 1;
        imageInfo.mipLevels = m_MipLevels;
        imageInfo.arrayLayers = 1;
        imageInfo.format = format;
        imageInfo.tiling = tiling;
        imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        imageInfo.usage = usage;
        imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
        imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        if (vkCreateImage(m_Device.vk_GetDevice(), &imageInfo, nullptr, &image) != VK_SUCCESS) {
            throw std::runtime_error("failed to create image!");
        }

        VkMemoryRequirements memRequirements;
        vkGetImageMemoryRequirements(m_Device.vk_GetDevice(), image, &memRequirements);

        VkMemoryAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocInfo.allocationSize = memRequirements.size;
        allocInfo.memoryTypeIndex = m_Device.vk_FindMemoryType(memRequirements.memoryTypeBits, properties);

        if (vkAllocateMemory(m_Device.vk_GetDevice(), &allocInfo, nullptr, &imageMemory) != VK_SUCCESS) {
            throw std::runtime_error("failed to allocate image memory!");
        }

        vkBindImageMemory(m_Device.vk_GetDevice(), image, imageMemory, 0);
    }


    void Texture2D::vk_CreateTextureImageView(VkFormat format) {
        VkImageViewCreateInfo viewInfo{};
        viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        viewInfo.image = textureImage;
        viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        viewInfo.format = format;
        viewInfo.subresourceRange.aspectMask = (format == VK_FORMAT_D32_SFLOAT) ? VK_IMAGE_ASPECT_DEPTH_BIT : VK_IMAGE_ASPECT_COLOR_BIT;
        viewInfo.subresourceRange.baseMipLevel = 0;
        viewInfo.subresourceRange.levelCount = 1;
        viewInfo.subresourceRange.baseArrayLayer = 0;
        viewInfo.subresourceRange.layerCount = 1;

        if (vkCreateImageView(m_Device.vk_GetDevice(), &viewInfo, nullptr, &textureImageView) != VK_SUCCESS) {
            throw std::runtime_error("failed to create texture image view!");
        }
    }

    void Texture2D::vk_CreateTextureSampler() {
        VkPhysicalDeviceProperties properties{};
        vkGetPhysicalDeviceProperties(m_Device.vk_GetPhysicalDevice(), &properties);

        VkSamplerCreateInfo samplerInfo{};
        samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
        samplerInfo.magFilter = VK_FILTER_LINEAR;
        samplerInfo.minFilter = VK_FILTER_LINEAR;
        samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        samplerInfo.anisotropyEnable = VK_TRUE;
        samplerInfo.maxAnisotropy = properties.limits.maxSamplerAnisotropy;
        samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
        samplerInfo.unnormalizedCoordinates = VK_FALSE;
        samplerInfo.compareEnable = VK_FALSE;
        samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
        samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;

        if (vkCreateSampler(m_Device.vk_GetDevice(), &samplerInfo, nullptr, &textureSampler) != VK_SUCCESS) {
            throw std::runtime_error("failed to create texture sampler!");
        }
    }

    void Texture2D::vk_CopyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height) const {
        VkCommandBuffer commandBuffer = m_Device.vk_BeginSingleTimeCommands();

        VkBufferImageCopy region{};
        region.bufferOffset = 0;
        region.bufferRowLength = 0;
        region.bufferImageHeight = 0;
        region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        region.imageSubresource.mipLevel = 0;
        region.imageSubresource.baseArrayLayer = 0;
        region.imageSubresource.layerCount = 1;
        region.imageOffset = {0, 0, 0};
        region.imageExtent = {
            width,
            height,
            1
        };

        vkCmdCopyBufferToImage(commandBuffer, buffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

        m_Device.vk_EndSingleTimeCommands(commandBuffer);
    }

    void Texture2D::vk_CreateSolidColorCubemap(glm::vec4 color) {
        const int textureSize = 1;
        uint8_t pixelData[4] = {
            static_cast<uint8_t>(color.r * 255.0f),
            static_cast<uint8_t>(color.g * 255.0f),
            static_cast<uint8_t>(color.b * 255.0f),
            static_cast<uint8_t>(color.a * 255.0f)
        };

        VkDeviceSize imageSize = sizeof(pixelData) * 6;

        Buffer stagingBuffer{
            m_Device,
            imageSize,
            1,
            VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT
        };

        stagingBuffer.map();
        for (int i = 0; i < 6; ++i) {
            stagingBuffer.vk_WriteToBuffer((void*)pixelData);
        }

        vk_CreateCubemapImage(textureSize, textureSize, VK_FORMAT_R32G32B32A32_SFLOAT,
                                VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT);

        TransitionImageLayout(textureImage, VK_FORMAT_R32G32B32A32_SFLOAT,
                            VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 6);

        vk_CopyBufferToImage(stagingBuffer.vk_GetBuffer(), textureImage, textureSize, textureSize);

        TransitionImageLayout(textureImage, VK_FORMAT_R32G32B32A32_SFLOAT,
                            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, 6);

        stagingBuffer.unmap();

        vk_CreateCubemapImageView(VK_FORMAT_R32G32B32A32_SFLOAT);

        vk_CreateCubemapSampler();
    }


    void Texture2D::vk_CreateSolidColorTexture(glm::vec4 color) {

        uint8_t pixelData[4] = {
            static_cast<uint8_t>(color.r * 255.0f),
            static_cast<uint8_t>(color.g * 255.0f),
            static_cast<uint8_t>(color.b * 255.0f),
            static_cast<uint8_t>(color.a * 255.0f)
        };

        VkDeviceSize imageSize = sizeof(pixelData);

        Buffer stagingBuffer{
            m_Device,
            imageSize,
            1,
            VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT
        };

        stagingBuffer.map();
        stagingBuffer.vk_WriteToBuffer((void*)pixelData);

        CreateImage(1, 1, VK_FORMAT_R8G8B8A8_UNORM,
                    VK_IMAGE_TILING_OPTIMAL,
                    VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
                    VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                    textureImage,
                    textureImageMemory);

        TransitionImageLayout(textureImage, VK_FORMAT_R8G8B8A8_UNORM,
                            VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

        vk_CopyBufferToImage(stagingBuffer.vk_GetBuffer(), textureImage, 1, 1);

        TransitionImageLayout(textureImage, VK_FORMAT_R8G8B8A8_UNORM,
                            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

        stagingBuffer.unmap();

        vk_CreateTextureImageView(VK_FORMAT_R8G8B8A8_UNORM);

        vk_CreateTextureSampler();
    }


    void Texture2D::UseFallbackTextures(TextureType type) {
        switch (type) {
            case TextureType::Albedo:
                vk_CreateSolidColorTexture({1.0f, 1.0f, 1.0f, 1.0f});
                break;
            case TextureType::Normal:
                vk_CreateSolidColorTexture({0.5f, 0.5f, 1.0f, 1.0f});
                break;
            case TextureType::Emissive:
                vk_CreateSolidColorTexture({1.0f, 1.0f, 1.0f, 1.0f});
                break;
            case TextureType::MetallicRoughness:
                vk_CreateSolidColorTexture({1.0f, 0.5f, 0.0f, 1.0f});
                break;
            case TextureType::AO:
                vk_CreateSolidColorTexture({1.0f, 1.0f, 1.0f, 1.0f});
                break;
            case TextureType::Cubemap:
                vk_CreateSolidColorCubemap({1.0f,1.0f,1.0f,1.0f});
            default:
                break;
        }

        vk_WriteToDescriptorSet();
    }


    void Texture2D::vk_CreateTextureImage(uint32_t width, uint32_t height, VkFormat format, VkImageUsageFlags usage, VkSampleCountFlagBits samples) {
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
        imageInfo.usage = usage | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
        imageInfo.samples = samples;
        imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        if (vkCreateImage(m_Device.vk_GetDevice(), &imageInfo, nullptr, &textureImage) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create color attachment image!");
        }

        VkMemoryRequirements memRequirements;
        vkGetImageMemoryRequirements(m_Device.vk_GetDevice(), textureImage, &memRequirements);

        VkMemoryAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocInfo.allocationSize = memRequirements.size;
        allocInfo.memoryTypeIndex = m_Device.vk_FindMemoryType(memRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

        if (vkAllocateMemory(m_Device.vk_GetDevice(), &allocInfo, nullptr, &textureImageMemory) != VK_SUCCESS) {
            throw std::runtime_error("Failed to allocate color attachment memory!");
        }

        vkBindImageMemory(m_Device.vk_GetDevice(), textureImage, textureImageMemory, 0);
    }

    void Texture2D::vk_CreateHDRTextureImage(const std::string& filepath) {
        stbi_set_flip_vertically_on_load(false);  // Disable automatic flip by stb_image

        int texWidth, texHeight, texChannels;
        float* pixels = stbi_loadf(filepath.c_str(), &texWidth, &texHeight, &texChannels, 4);
        if (!pixels) {
            throw std::runtime_error("failed to load HDR texture image!");
        }

        // fixes the stbi_set_flip_vertically_on_load flipping my UVs
        float* flippedPixels = new float[texWidth * texHeight * 4];

        for (int y = 0; y < texHeight; ++y) {
            for (int x = 0; x < texWidth; ++x) {
                int originalIdx = (y * texWidth + x) * 4;
                int flippedIdx = ((texHeight - 1 - y) * texWidth + x) * 4;

                flippedPixels[flippedIdx] = pixels[originalIdx];        // R
                flippedPixels[flippedIdx + 1] = pixels[originalIdx + 1]; // G
                flippedPixels[flippedIdx + 2] = pixels[originalIdx + 2]; // B
                flippedPixels[flippedIdx + 3] = pixels[originalIdx + 3]; // A
            }
        }

        VkDeviceSize imageSize = texWidth * texHeight * 4 * sizeof(float);

        Buffer stagingBuffer{
            m_Device,
            imageSize,
            1,
            VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT
        };

        stagingBuffer.map();
        stagingBuffer.vk_WriteToBuffer((void*)flippedPixels);
        stagingBuffer.unmap();

        delete[] flippedPixels;

        stbi_image_free(pixels);

        CreateImage(
            texWidth,
            texHeight,
            VK_FORMAT_R32G32B32A32_SFLOAT,
            VK_IMAGE_TILING_OPTIMAL,
            VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
            textureImage,
            textureImageMemory);

        TransitionImageLayout(textureImage,
            VK_FORMAT_R32G32B32A32_SFLOAT,
            VK_IMAGE_LAYOUT_UNDEFINED,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

        vk_CopyBufferToImage(stagingBuffer.vk_GetBuffer(),
            textureImage,
            static_cast<uint32_t>(texWidth),
            static_cast<uint32_t>(texHeight));

        GenerateMipmap(textureImage, VK_FORMAT_R32G32B32A32_SFLOAT, texWidth, texHeight, m_MipLevels);
    }

    void Texture2D::vk_CreateHDRTextureImageView(VkFormat format) {
        VkImageViewCreateInfo viewInfo{};
        viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        viewInfo.image = textureImage;
        viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        viewInfo.format = format;
        viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        viewInfo.subresourceRange.baseMipLevel = 0;
        viewInfo.subresourceRange.levelCount = 1;
        viewInfo.subresourceRange.baseArrayLayer = 0;
        viewInfo.subresourceRange.layerCount = 1;

        if (vkCreateImageView(m_Device.vk_GetDevice(), &viewInfo, nullptr, &textureImageView) != VK_SUCCESS) {
            throw std::runtime_error("failed to create HDR texture image view!");
        }
    }

    void Texture2D::vk_CreateHDRTextureSampler() {
        VkPhysicalDeviceProperties properties{};
        vkGetPhysicalDeviceProperties(m_Device.vk_GetPhysicalDevice(), &properties);

        VkSamplerCreateInfo samplerInfo{};
        samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
        samplerInfo.magFilter = VK_FILTER_LINEAR;
        samplerInfo.minFilter = VK_FILTER_LINEAR;
        samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        samplerInfo.anisotropyEnable = VK_TRUE;
        samplerInfo.maxAnisotropy = properties.limits.maxSamplerAnisotropy;
        samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
        samplerInfo.unnormalizedCoordinates = VK_FALSE;
        samplerInfo.compareEnable = VK_FALSE;
        samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
        samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;

        if (vkCreateSampler(m_Device.vk_GetDevice(), &samplerInfo, nullptr, &textureSampler) != VK_SUCCESS) {
            throw std::runtime_error("failed to create HDR texture sampler!");
        }
    }

    void Texture2D::vk_CreateTextureImage(const std::string& filepath) {
        int texWidth, texHeight, texChannels;
        stbi_uc* pixels = stbi_load(filepath.c_str(), &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
        VkDeviceSize imageSize = texWidth * texHeight * 4;

        if (!pixels) {
            throw std::runtime_error("failed to load texture image!");
        }

        Buffer stagingBuffer{
            m_Device,
            imageSize,
            1,
            VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT
        };

        stagingBuffer.map();
        stagingBuffer.vk_WriteToBuffer((void*)pixels);

        stbi_image_free(pixels);

        CreateImage(
            texWidth,
            texHeight,
            VK_FORMAT_R8G8B8A8_UNORM,
            VK_IMAGE_TILING_OPTIMAL,
            VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
            textureImage,
            textureImageMemory);

        TransitionImageLayout(textureImage,
            VK_FORMAT_R8G8B8A8_UNORM,
            VK_IMAGE_LAYOUT_UNDEFINED,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

        vk_CopyBufferToImage(stagingBuffer.vk_GetBuffer(),
            textureImage,
            static_cast<uint32_t>(texWidth),
            static_cast<uint32_t>(texHeight));

        stagingBuffer.unmap();
    
        GenerateMipmap(textureImage, VK_FORMAT_R8G8B8A8_UNORM, texWidth, texHeight, m_MipLevels);
    }

}