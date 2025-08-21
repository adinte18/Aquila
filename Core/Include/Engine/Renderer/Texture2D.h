//
// Created by alexa on 09/08/2024.
//

#ifndef TEXTURE2D_H
#define TEXTURE2D_H

#include <memory>
#include <glm/vec4.hpp>

#include "Descriptor.h"
#include "Engine/Renderer/Device.h"

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

            Builder& asHDR(){
                useHDR = true;
                return *this;
            }

            [[nodiscard]] Ref<Texture2D> build() const {
                if (useCubemap && useHDR) {
                    throw std::runtime_error("Cannot build a texture as both HDR and Cubemap. Choose only one.");
                }

                auto texture = CreateRef<Texture2D>(device);

                if (!filepath.empty()) {
                    texture->CreateTexture(filepath, format);
                } else {
                    if (useCubemap) {
                        texture->CreateCubeMap(width, height, format, usage);
                    } 
                    else if (useHDR) {
                        if (!filepath.empty()) 
                            texture->CreateHDRTexture(filepath);
                        else
                            throw std::runtime_error("HDR texture requires a filepath.");
                    } 
                    else {
                        texture->CreateTexture(width, height, format, usage, samples);
                    }
                }

                return texture;
            }

        private:
            bool useCubemap = false;
            bool useHDR = false;
            Device& device;
            uint32_t width = 512;
            uint32_t height = 512;
            VkFormat format = VK_FORMAT_R8G8B8A8_UNORM;
            VkImageUsageFlags usage = VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
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

        public:

        [[nodiscard]] static Ref<Texture2D> create(Device& device) {
            return CreateRef<Texture2D>(device);
        }

        void GenerateMipmap(VkImage image, VkFormat format, int32_t width, int32_t height, uint32_t mipLevels);

        void CreateHDRTexture(const std::string& filepath, VkFormat format = VK_FORMAT_R32G32B32A32_SFLOAT);
        void CreateCubeMap(uint32_t width, uint32_t height, VkFormat format, VkImageUsageFlags usage);
        void CreateMipMappedCubemap(uint32_t width, uint32_t height, VkFormat format, VkImageUsageFlags usage);
        void CreateTexture(const std::string& filepath, VkFormat format = VK_FORMAT_R8G8B8A8_UNORM);
        void CreateTexture(uint32_t width, uint32_t height, VkFormat format = VK_FORMAT_R8G8B8A8_UNORM,
                           VkImageUsageFlags usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
                           VkSampleCountFlagBits samples = VK_SAMPLE_COUNT_1_BIT);
        void UseFallbackTextures(TextureType type);

        // Inline Getters
        [[nodiscard]] VkImage GetTextureImage() const { return textureImage; }
        [[nodiscard]] VkImageView GetTextureImageView() const { return textureImageView; }
        [[nodiscard]] VkSampler GetTextureSampler() const { return textureSampler; }
        [[nodiscard]] VkDeviceMemory GetTextureImageMemory() const { return textureImageMemory; }
        [[nodiscard]] VkDescriptorSet GetDescriptorSet() const { return descriptorSet; }
        [[nodiscard]] uint32_t GetMipLevels() const { return m_MipLevels; }
        [[nodiscard]] VkFormat GetFormat() const { return m_Format; }

        [[nodiscard]] bool HasImage() const { return textureImage != VK_NULL_HANDLE; }
        [[nodiscard]] bool HasImageView() const { return textureImageView != VK_NULL_HANDLE; }
        [[nodiscard]] bool HasSampler() const { return textureSampler != VK_NULL_HANDLE; }
        [[nodiscard]] bool HasImageMemory() const { return textureImageMemory != VK_NULL_HANDLE; }

        [[nodiscard]] VkDescriptorImageInfo GetDescriptorSetInfo() const;

        void MarkForDestruction() { isMarkedForDestruction = true; }
        bool isMarkedForDestruction = false;

        void DestroyAll();
        void Destroy();

        void CreateImage(uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling,
                         VkImageUsageFlags usage, VkMemoryPropertyFlags properties,
                         VkImage& image, VkDeviceMemory& imageMemory);

        // Rule of five
        Texture2D(Device& device);
        ~Texture2D();

        Texture2D(const Texture2D&) = delete;
        Texture2D& operator=(const Texture2D&) = delete;
        Texture2D(Texture2D&&) noexcept = default;
        Texture2D& operator=(Texture2D&&) noexcept = delete;

    private:
        uint32_t m_MipLevels = 1;
        Device& m_Device;

        VkFormat m_Format = VK_FORMAT_R8G8B8A8_UNORM;

        VkImage textureImage = VK_NULL_HANDLE;
        VkSampler textureSampler = VK_NULL_HANDLE;
        VkImageView textureImageView = VK_NULL_HANDLE;
        VkDeviceMemory textureImageMemory = VK_NULL_HANDLE;

        Unique<DescriptorPool> descriptorPool;
        Unique<DescriptorSetLayout> descriptorSetLayout;
        VkDescriptorSet descriptorSet = VK_NULL_HANDLE;

        // HDR
        void vk_CreateHDRTextureImage(const std::string& filepath);
        void vk_CreateHDRTextureImageView(VkFormat format);
        void vk_CreateHDRTextureSampler();

        // Cubemap
        void vk_CreateCubemapImage(uint32_t width, uint32_t height, VkFormat format, VkImageUsageFlags usage);
        void vk_CreateCubemapImageView(VkFormat format);
        void vk_CreateCubemapSampler();
        void vk_CreateSolidColorCubemap(glm::vec4 color);

        // LDR
        void vk_CreateTextureImage(const std::string& filepath);
        void vk_CreateTextureImage(uint32_t width, uint32_t height, VkFormat format,
                                   VkImageUsageFlags usage, VkSampleCountFlagBits samples);
        void vk_CreateTextureImageView(VkFormat format);
        void vk_CreateTextureSampler();
        void vk_CopyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height) const;
        void vk_WriteToDescriptorSet();
        void vk_CreateSolidColorTexture(glm::vec4 color);

        void TransitionImageLayout(VkImage image, VkFormat format,
                                   VkImageLayout oldLayout, VkImageLayout newLayout,
                                   uint32_t layers = 1);
    };

} // namespace Engine

#endif // TEXTURE2D_H
