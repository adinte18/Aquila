//
// Created by alexa on 09/08/2024.
//

#ifndef TEXTURE2D_H
#define TEXTURE2D_H

#include <array>
#include <memory>
#include "Descriptor.h"
#include "Engine/Device.h"

namespace Engine {
    class Texture2D {
    public:
        class Builder {
        public:
            explicit Builder(Device& device) : device(device) {}

            Builder& setSize(uint32_t width, uint32_t height) {
                this->width = width;
                this->height = height;
                return *this;
            }

            Builder& setFormat(VkFormat format) {
                this->format = format;
                return *this;
            }

            Builder& setUsage(VkImageUsageFlags usage) {
                this->usage = usage;
                return *this;
            }

            Builder& setFilepath(const std::string& filepath) {
                this->filepath = filepath;
                return *this;
            }

            [[nodiscard]] std::shared_ptr<Texture2D> build() const {
                auto texture = std::make_shared<Texture2D>(device);
                if (!filepath.empty()) {
                    texture->CreateTexture(filepath, format);
                } else {
                    texture->CreateTexture(width, height, format, usage);
                }
                return texture;
            }

        private:
            Device& device;
            uint32_t width = 512;
            uint32_t height = 512;
            VkFormat format = VK_FORMAT_R8G8B8A8_SRGB;
            VkImageUsageFlags usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
            std::string filepath;
        };

        // Static function to create a shared Texture2D instance
        [[nodiscard]] static std::shared_ptr<Texture2D> create(Device& device) {
            return std::make_shared<Texture2D>(device);
        }

        void CreateTexture(const std::string& filepath, VkFormat format = VK_FORMAT_R8G8B8A8_SRGB);
        void CreateTexture(uint32_t width, uint32_t height, VkFormat format = VK_FORMAT_R8G8B8A8_SRGB,
                           VkImageUsageFlags usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT);

        // Getters
        [[nodiscard]] VkImage GetTextureImage() const { return textureImage; }
        [[nodiscard]] VkImageView GetTextureImageView() const { return textureImageView; }
        [[nodiscard]] VkSampler GetTextureSampler() const { return textureSampler; }
        [[nodiscard]] VkDeviceMemory GetTextureImageMemory() const { return textureImageMemory; }
        [[nodiscard]] VkDescriptorImageInfo GetDescriptorSetInfo() const;
        [[nodiscard]] VkDescriptorSet GetDescriptorSet() const { return descriptorSet; }


        [[nodiscard]] bool HasImage() const { return textureImage != VK_NULL_HANDLE; }
        [[nodiscard]] bool HasImageView() const { return textureImageView != VK_NULL_HANDLE; }
        [[nodiscard]] bool HasSampler() const { return textureSampler != VK_NULL_HANDLE; }
        [[nodiscard]] bool HasImageMemory() const { return textureImageMemory != VK_NULL_HANDLE; }

        void DestroyAll();

        void CreateImage(uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling,
                         VkImageUsageFlags usage, VkMemoryPropertyFlags properties,
                         VkImage& image, VkDeviceMemory& imageMemory);

        Texture2D(Device& device) : dev(device) {
            textureImage = VK_NULL_HANDLE;
            textureSampler = VK_NULL_HANDLE;
            textureImageView = VK_NULL_HANDLE;
            textureImageMemory = VK_NULL_HANDLE;
        }

        ~Texture2D() {
            DestroyAll();
        }


    private:

        Device& dev;
        VkImage textureImage{};
        VkSampler textureSampler{};
        VkImageView textureImageView{};
        VkDeviceMemory textureImageMemory{};

        std::unique_ptr<DescriptorPool> descriptorPool;
        std::unique_ptr<DescriptorSetLayout> descriptorSetLayout;
        VkDescriptorSet descriptorSet;

        void vk_CreateTextureImage(const std::string& filepath);
        void vk_CreateTextureImage(uint32_t width, uint32_t height, VkFormat format, VkImageUsageFlags usage);
        void vk_CreateTextureImageView(VkFormat format);
        void vk_CreateTextureSampler();
        void vk_CopyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height) const;
        void vk_WriteToDescriptorSet();
        void TransitionImageLayout(
            VkImage image,
            VkFormat format,
            VkImageLayout oldLayout,
            VkImageLayout newLayout);
    };
}

#endif // TEXTURE2D_H
