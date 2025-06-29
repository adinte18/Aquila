//
// Created by alexa on 16/04/2025.
//

#include "Engine/Renderpass.h"

Renderpass::~Renderpass() {}

VkDescriptorImageInfo Renderpass::GetImageInfo(VkImageLayout layout) const {
    VkDescriptorImageInfo info = {};

    if (colorAttachment && colorAttachment->HasImageView() && colorAttachment->HasSampler()) {
        info.imageView = colorAttachment->GetTextureImageView();
        info.sampler = colorAttachment->GetTextureSampler();
    } else if (depthAttachment && depthAttachment->HasImageView() && depthAttachment->HasSampler()) {
        info.imageView = depthAttachment->GetTextureImageView();
        info.sampler = depthAttachment->GetTextureSampler();
    }
    info.imageLayout = layout;
    return info;
}

void Renderpass::WriteToDescriptorSet() {
    VkDescriptorImageInfo descriptorInfo{};
    if (colorAttachment && colorAttachment->HasImageView()) {
        descriptorInfo.sampler = colorAttachment->GetTextureSampler();
        descriptorInfo.imageView = colorAttachment->GetTextureImageView();
        descriptorInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;  // For color
    } else if (depthAttachment && depthAttachment->HasImageView()) {
        descriptorInfo.sampler = depthAttachment->GetTextureSampler();
        descriptorInfo.imageView = depthAttachment->GetTextureImageView();
        descriptorInfo.imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;  // For depth
    }

    Engine::DescriptorWriter writer(*m_DescriptorSetLayout, *Engine::DescriptorAllocator::GetSharedPool());
    writer.writeImage(0, &descriptorInfo);
    writer.overwrite(m_DescriptorSet);
}

