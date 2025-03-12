#pragma once
#include "Texture2D.h"
#include "Engine/Device.h"

namespace Engine {
    class RenderTarget {
    public:
        // VkImage colorImage{};            // Color image
        // VkImageView colorImageView{};    // Color image view
        // VkSampler colorSampler{};        // Sampler for color image
        // VkDeviceMemory colorMemory{};    // Memory for color image
        
        // VkImage depthImage{};            // Depth image
        // VkImageView depthImageView{};    // Depth image view
        // VkSampler depthSampler{};        // Depth sampler
        // VkDeviceMemory depthMemory{};    // Depth image memory

        std::shared_ptr<Texture2D> colorTexture{};
        std::shared_ptr<Texture2D> depthTexture{};

        enum class AttachmentType {
            COLOR,
            DEPTH,
            BOTH
        };

        RenderTarget(Device& dev, uint32_t width, uint32_t height, VkFormat colorFormat, VkImageUsageFlags colorUsage,
                     VkFormat depthFormat, VkImageUsageFlags depthUsage, AttachmentType attachmentType = AttachmentType::BOTH)
            : device(dev) {

            //TODO - this drives me insane, without this line, it does not work, why?
            colorTexture = Texture2D::create(dev);
            depthTexture = Texture2D::create(dev);

            if (attachmentType == AttachmentType::COLOR || attachmentType == AttachmentType::BOTH) {
                CreateColorTexture(width, height, colorFormat, colorUsage);
            }

            if (attachmentType == AttachmentType::DEPTH || attachmentType == AttachmentType::BOTH) {
                CreateDepthTexture(width, height, depthFormat, depthUsage);
            }
        }

        void CreateColorTexture(uint32_t width, uint32_t height, VkFormat format, VkImageUsageFlags usage) {
            colorTexture = Texture2D::Builder(device)
                .setSize(width, height)
                .setFormat(format)
                .setUsage(usage)
                .build();
        }

        void CreateDepthTexture(uint32_t width, uint32_t height, VkFormat format, VkImageUsageFlags usage) {
            depthTexture = Texture2D::Builder(device)
                .setSize(width, height)
                .setFormat(format)
                .setUsage(usage)
                .build();
        }

    private:
        Device& device;
    };
}
