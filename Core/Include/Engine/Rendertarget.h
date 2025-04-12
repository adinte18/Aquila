#pragma once
#include "Texture2D.h"
#include "Engine/Device.h"

namespace Engine {
    class RenderTarget {
    public:
        std::shared_ptr<Texture2D> colorTexture{};
        std::shared_ptr<Texture2D> depthTexture{};
        std::vector<VkImageView> cubemapFaceViews; // Only used for cubemaps

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

            // These allocations are required early to avoid null in builders
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
            if (target == TargetType::CUBEMAP) {
                colorTexture = Texture2D::Builder(device)
                    .setSize(width, height)
                    .setFormat(format)
                    .setUsage(usage | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT)
                    .asCubemap()
                    .build();

                CreateCubemapFaceViews(format);
            } else {
                colorTexture = Texture2D::Builder(device)
                    .setSize(width, height)
                    .setFormat(format)
                    .setUsage(usage)
                    .build();
            }
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
        TargetType target;

        void CreateCubemapFaceViews(VkFormat format) {
            cubemapFaceViews.resize(6);

            for (uint32_t i = 0; i < 6; ++i) {
                VkImageViewCreateInfo viewInfo{};
                viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
                viewInfo.image = colorTexture->GetTextureImage();
                viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
                viewInfo.format = format;
                viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
                viewInfo.subresourceRange.baseMipLevel = 0;
                viewInfo.subresourceRange.levelCount = 1;
                viewInfo.subresourceRange.baseArrayLayer = i;
                viewInfo.subresourceRange.layerCount = 1;

                if (vkCreateImageView(device.vk_GetDevice(), &viewInfo, nullptr, &cubemapFaceViews[i]) != VK_SUCCESS) {
                    throw std::runtime_error("Failed to create cubemap face view!");
                }
            }
        }
    };
}
