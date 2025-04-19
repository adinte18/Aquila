#pragma once
#include "Texture2D.h"
#include "Engine/Device.h"

namespace Engine {
    class RenderTarget {
    public:
        std::shared_ptr<Texture2D> colorTexture{};  // Shared pointer to color texture
        std::shared_ptr<Texture2D> depthTexture{};  // Shared pointer to depth texture

        enum class AttachmentType {
            COLOR,
            DEPTH,
            BOTH
        };

        enum class TargetType {
            TEXTURE_2D,
            CUBEMAP
        };

        RenderTarget(Device& dev,
                     uint32_t width, uint32_t height,
                     VkFormat colorFormat, VkImageUsageFlags colorUsage,
                     VkFormat depthFormat, VkImageUsageFlags depthUsage,
                     AttachmentType attachmentType = AttachmentType::BOTH,
                     TargetType targetType = TargetType::TEXTURE_2D)
            : device(dev), target(targetType) {

            if (attachmentType == AttachmentType::COLOR || attachmentType == AttachmentType::BOTH) {
                colorTexture = CreateColorTexture(dev, targetType, width, height, colorFormat, colorUsage);
            }

            if (attachmentType == AttachmentType::DEPTH || attachmentType == AttachmentType::BOTH) {
                depthTexture = CreateDepthTexture(dev, width, height, depthFormat, depthUsage);
            }
        }

        static std::shared_ptr<Texture2D> CreateColorTexture(Device& device,
                                                              TargetType target,
                                                              uint32_t width, uint32_t height,
                                                              VkFormat format, VkImageUsageFlags usage, VkSampleCountFlagBits samples = VK_SAMPLE_COUNT_1_BIT)
        {
            std::shared_ptr<Texture2D> texture;
            if (target == TargetType::CUBEMAP) {
                texture = Texture2D::Builder(device)
                    .setSize(width, height)
                    .setFormat(format)
                    .setUsage(usage | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT)
                    .asCubemap()
                    .build();
            } else {
                texture = Texture2D::Builder(device)
                    .setSize(width, height)
                    .setFormat(format)
                    .setUsage(usage)
                    .setSamples(samples)
                    .build();
            }
            return texture;
        }

        static std::shared_ptr<Texture2D> CreateDepthTexture(Device& device,
                                                              uint32_t width, uint32_t height,
                                                              VkFormat format, VkImageUsageFlags usage)
        {
            std::shared_ptr<Texture2D> texture = Texture2D::create(device);
            texture = Texture2D::Builder(device)
                .setSize(width, height)
                .setFormat(format)
                .setUsage(usage)
                .build();
            return texture;
        }

    private:
        Device& device;
        TargetType target;
    };
} // namespace Engine
