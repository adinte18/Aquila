//
// Created by alexa on 09/08/2024.
//

#ifndef TEXTURE2D_H
#define TEXTURE2D_H

#include <array>
#include <memory>
#include <glm/vec4.hpp>

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

            Builder& setSamples(VkSampleCountFlagBits samples) {
                this->samples = samples;
                return *this;
            }

            Builder& setFilepath(const std::string& filepath) {
                this->filepath = filepath;
                return *this;
            }

            Builder& asCubemap() {
                useCubemap = true;
                return *this;
            }


            [[nodiscard]] std::shared_ptr<Texture2D> build() const {
                auto texture = std::make_shared<Texture2D>(device);
                if (!filepath.empty()) {
                    texture->CreateTexture(filepath, format);
                } else {
                    if (useCubemap) {
                        texture->CreateCubeMap(width, height, format, usage);
                    } else {
                        texture->CreateTexture(width, height, format, usage, samples);
                    }
                }
                return texture;
            }

        private:
            bool useCubemap = false;
            Device& device;
            uint32_t width = 512;
            uint32_t height = 512;
            VkFormat format = VK_FORMAT_R8G8B8A8_SRGB;
            VkImageUsageFlags usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
            VkSampleCountFlagBits samples = VK_SAMPLE_COUNT_1_BIT;
            std::string filepath;
        };

        enum class TextureType {
            Albedo,
            Normal,
            Emissive,
            AO,
            MetallicRoughness,
            Cubemap
        };


        // Static function to create a shared Texture2D instance
        [[nodiscard]] static std::shared_ptr<Texture2D> create(Device& device) {
            return std::make_shared<Texture2D>(device);
        }

        // HDR textures
        void CreateHDRTexture(const std::string& filepath, VkFormat format = VK_FORMAT_R32G32B32A32_SFLOAT);

        void CreateCubeMap(uint32_t width, uint32_t height, VkFormat format, VkImageUsageFlags usage);
        void CreateMipMappedCubemap(uint32_t width, uint32_t height, VkFormat format, VkImageUsageFlags usage);

        // LDR textures
        void CreateTexture(const std::string& filepath, VkFormat format = VK_FORMAT_R8G8B8A8_UNORM);
        void CreateTexture(uint32_t width, uint32_t height, VkFormat format = VK_FORMAT_R8G8B8A8_UNORM,
                           VkImageUsageFlags usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VkSampleCountFlagBits samples = VK_SAMPLE_COUNT_1_BIT);
        void UseFallbackTextures(TextureType type);

        // Getters
        [[nodiscard]] VkImage GetTextureImage() const { return textureImage; }
        [[nodiscard]] VkImageView GetTextureImageView() const { return textureImageView; }
        [[nodiscard]] VkSampler GetTextureSampler() const { return textureSampler; }
        [[nodiscard]] VkDeviceMemory GetTextureImageMemory() const { return textureImageMemory; }
        [[nodiscard]] VkDescriptorImageInfo GetDescriptorSetInfo() const;
        [[nodiscard]] VkDescriptorSet GetDescriptorSet() const { return descriptorSet; }
        [[nodiscard]] uint32_t GetMipLevels() const { return m_MipLevels; }

        [[nodiscard]] bool HasImage() const { return textureImage != VK_NULL_HANDLE; }
        [[nodiscard]] bool HasImageView() const { return textureImageView != VK_NULL_HANDLE; }
        [[nodiscard]] bool HasSampler() const { return textureSampler != VK_NULL_HANDLE; }
        [[nodiscard]] bool HasImageMemory() const { return textureImageMemory != VK_NULL_HANDLE; }

        void Texture2D::MarkForDestruction() {
            isMarkedForDestruction = true;
        }

        bool isMarkedForDestruction = false;

        void DestroyAll();
        void Destroy();

        void CreateImage(uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling,
                         VkImageUsageFlags usage, VkMemoryPropertyFlags properties,
                         VkImage& image, VkDeviceMemory& imageMemory);


        Texture2D(Device& device) : m_Device(device) {
            textureImage = VK_NULL_HANDLE;
            textureSampler = VK_NULL_HANDLE;
            textureImageView = VK_NULL_HANDLE;
            textureImageMemory = VK_NULL_HANDLE;

            descriptorPool = Engine::DescriptorPool::Builder(m_Device)
                .setMaxSets(1)
                .setPoolFlags(VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT)
                .addPoolSize(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1)
                .build();

            descriptorSetLayout = Engine::DescriptorSetLayout::Builder(m_Device)
                    .addBinding(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT)
                    .build();

            descriptorPool->allocateDescriptor(descriptorSetLayout->getDescriptorSetLayout(), descriptorSet);

        }

        ~Texture2D() {
            DestroyAll();
        }


    private:
        uint32_t m_MipLevels;
        Device& m_Device;
        VkImage textureImage{};
        VkSampler textureSampler{};
        VkImageView textureImageView{};
        VkDeviceMemory textureImageMemory{};

        std::unique_ptr<DescriptorPool> descriptorPool;
        std::unique_ptr<DescriptorSetLayout> descriptorSetLayout;
        VkDescriptorSet descriptorSet;

        // HDR textures
        void vk_CreateHDRTextureImage(const std::string& filepath);
        void vk_CreateHDRTextureImageView(VkFormat format);
        void vk_CreateHDRTextureSampler();

        // Cubemap textures
        void vk_CreateCubemapImage(uint32_t width, uint32_t height, VkFormat format, VkImageUsageFlags usage);
        void vk_CreateCubemapImageView(VkFormat format);
        void vk_CreateCubemapSampler();
        void vk_CreateSolidColorCubemap(glm::vec4 color); // Fallback strategy



        // LDR textures
        void vk_CreateTextureImage(const std::string& filepath);
        void vk_CreateTextureImage(uint32_t width, uint32_t height, VkFormat format, VkImageUsageFlags usage, VkSampleCountFlagBits samples);
        void vk_CreateTextureImageView(VkFormat format);
        void vk_CreateTextureSampler();
        void vk_CopyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height) const;
        void vk_WriteToDescriptorSet();
        void vk_CreateSolidColorTexture(glm::vec4 color); // Fallback strategy
        void TransitionImageLayout(
            VkImage image,
            VkFormat format,
            VkImageLayout oldLayout,
            VkImageLayout newLayout,
            uint32_t layers = 1);
    };
}

#endif // TEXTURE2D_H
