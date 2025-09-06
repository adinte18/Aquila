#ifndef RENDERTARGET_H
#define RENDERTARGET_H

#include "Texture2D.h"
#include "Engine/Renderer/Device.h"

namespace Engine {
    class RenderTarget {
    public:
        Ref<Texture2D> colorTexture{};  // Shared pointer to color texture
        Ref<Texture2D> depthTexture{};  // Shared pointer to depth texture

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
                     const std::string& debugName = "",
                     AttachmentType attachmentType = AttachmentType::BOTH,
                     TargetType targetType = TargetType::TEXTURE_2D)
            : device(dev), target(targetType) {

            if (attachmentType == AttachmentType::COLOR || attachmentType == AttachmentType::BOTH) {
                colorTexture = CreateColorTexture(dev, debugName, targetType, width, height, colorFormat, colorUsage);
            }

            if (attachmentType == AttachmentType::DEPTH || attachmentType == AttachmentType::BOTH) {
                depthTexture = CreateDepthTexture(dev, debugName, width, height, depthFormat, depthUsage);
            }
        }

        static Ref<Texture2D> CreateColorTexture(Device& device, const std::string& debugName,
                                                              TargetType target,
                                                              uint32_t width, uint32_t height,
                                                              VkFormat format, VkImageUsageFlags usage, VkSampleCountFlagBits samples = VK_SAMPLE_COUNT_1_BIT)
        {
            Ref<Texture2D> texture;
            if (target == TargetType::CUBEMAP) {
                texture = Texture2D::Builder(device)
                    .setSize(width, height)
                    .setFormat(format)
                    .setUsage(usage | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT)
                    .setDebugName(debugName)
                    .asCubemap()
                    .build();
            } else {
                texture = Texture2D::Builder(device)
                    .setSize(width, height)
                    .setFormat(format)
                    .setUsage(usage)
                    .setDebugName(debugName)
                    .setSamples(samples)
                    .build();
            }
            return texture;
        }

        static Ref<Texture2D> CreateDepthTexture(Device& device, const std::string& debugName,
                                                              uint32_t width, uint32_t height,
                                                              VkFormat format, VkImageUsageFlags usage)
        {
            Ref<Texture2D> texture;
            texture = Texture2D::Builder(device)
                .setSize(width, height)
                .setFormat(format)
                .setUsage(usage)
                .setDebugName(debugName)
                .build();
            return texture;
        }

    private:
        Device& device;
        TargetType target;
    };
} // namespace Engine

#endif