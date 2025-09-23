//
// Created by alexa on 16/04/2025.
//

#include "Engine/Renderer/Renderpass.h"

namespace Engine {
Renderpass::~Renderpass() {}

VkDescriptorImageInfo Renderpass::GetImageInfo(VkImageLayout layout) const {
  VkDescriptorImageInfo info = {};

  if (m_ColorAttachment && m_ColorAttachment->HasImageView() &&
      m_ColorAttachment->HasSampler()) {
    info.imageView = m_ColorAttachment->GetTextureImageView();
    info.sampler = m_ColorAttachment->GetTextureSampler();
  } else if (m_DepthAttachment && m_DepthAttachment->HasImageView() &&
             m_DepthAttachment->HasSampler()) {
    info.imageView = m_DepthAttachment->GetTextureImageView();
    info.sampler = m_DepthAttachment->GetTextureSampler();
  }
  info.imageLayout = layout;
  return info;
}

void Renderpass::WriteToDescriptorSet() {
  VkDescriptorImageInfo descriptorInfo{};
  if (m_ColorAttachment && m_ColorAttachment->HasImageView()) {
    descriptorInfo.sampler = m_ColorAttachment->GetTextureSampler();
    descriptorInfo.imageView = m_ColorAttachment->GetTextureImageView();
    descriptorInfo.imageLayout =
        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL; // For color
  } else if (m_DepthAttachment && m_DepthAttachment->HasImageView()) {
    descriptorInfo.sampler = m_DepthAttachment->GetTextureSampler();
    descriptorInfo.imageView = m_DepthAttachment->GetTextureImageView();
    descriptorInfo.imageLayout =
        VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL; // For depth
  }

  Engine::DescriptorWriter writer(
      *m_DescriptorSetLayout, *Engine::DescriptorAllocator::GetSharedPool());
  writer.writeImage(0, &descriptorInfo);
  writer.overwrite(m_DescriptorSet);
}
} // namespace Engine
