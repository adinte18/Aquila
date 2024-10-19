//
// Created by alexa on 09/08/2024.
//

#ifndef Texture2D_H
#define Texture2D_H

#include <array>
#include <memory>

#include "Engine/Device.h"

namespace Engine
{
    class Texture2D {
    public:
        [[nodiscard]] static std::shared_ptr<Texture2D> create(Device& device);

        void vk_CreateCubeMap(const std::vector<std::string>& filepath);
        void vk_CreateCubeMap(const std::string& filepath);
        void vk_CreateDepthCubeMap();
        std::string GetName();
        void vk_CreateTexture(const std::string& filepath);
        [[nodiscard]] VkImage vk_GetTextureImage() const { return textureImage; }
        [[nodiscard]] VkImageView vk_GetTextureImageView() const { return textureImageView; }
        [[nodiscard]] VkSampler vk_GetTextureSampler() const { return textureSampler; }
        void vk_CreateShadowMap(uint32_t width, uint32_t height);
        void vk_TransitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout,
                              VkImageLayout newLayout, uint32_t layerCount);

        VkImageView vk_GetDepthImageView(uint32_t i) { return shadowCubeMapFaceImageViews[i]; }
        VkDeviceMemory vk_GetTextureImageMemory() { return textureImageMemory; };

        [[nodiscard]] VkDescriptorImageInfo vk_GetDescriptorImageInfo() const {
            VkDescriptorImageInfo imageInfo{};
            imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            imageInfo.imageView = vk_GetTextureImageView();
            imageInfo.sampler = vk_GetTextureSampler();
            return imageInfo;
        };


        Texture2D(Device& device) : dev(device) {}

        ~Texture2D()
        {
            vkDestroyImageView(dev.vk_GetDevice(), textureImageView, nullptr);

            vkDestroyImage(dev.vk_GetDevice(), textureImage, nullptr);
            vkDestroySampler(dev.vk_GetDevice(), textureSampler, nullptr);
            vkFreeMemory(dev.vk_GetDevice(), textureImageMemory, nullptr);
        }

        void vk_CreateImage(uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling,
                            VkImageUsageFlags usage,
                            VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& imageMemory,
                            uint32_t layerCount);

    private:
        Device& dev;

        VkImage textureImage;

        VkDeviceMemory textureImageMemory;

        VkImageView textureImageView;

        VkSampler textureSampler;

        std::string filename;

        VkImage shadowCubeMap;

        std::array<VkImageView, 6> shadowCubeMapFaceImageViews{};

        void vk_CreateTextureImage(const std::string& filepath);

        VkImageView vk_CreateImageView(VkImage image, VkFormat format, VkImageViewType type, uint32_t layerCount);
        void vk_CreateTextureImageView(VkImageViewType type, uint32_t layerCount);

        void vk_CopyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height, uint32_t layerCount);

        void vk_CreateTextureSampler(bool isCubemap);

        void vk_CreateCubeMapImage(const std::vector<std::string>& cubeMapFiles);
        void vk_CreateCubeMapImage(const std::string& hdrFile);
    };
}

#endif //Texture2D_H
