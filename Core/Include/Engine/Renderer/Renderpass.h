//
// Created by alexa on 15/04/2025.
//

#ifndef RENDERPASS_BASE_H
#define RENDERPASS_BASE_H

#include "Engine/Renderer/Framebuffer.h"
#include "Engine/Renderer/Rendertarget.h"

namespace Engine {
class Renderpass {
public:
  Renderpass(Device &device, const VkExtent2D &extent,
             const Ref<DescriptorSetLayout> &descriptorSetLayout)
      : m_Device(device), m_Extent(extent),
        m_DescriptorSetLayout(descriptorSetLayout) {
    DescriptorAllocator::Allocate(descriptorSetLayout->GetDescriptorSetLayout(),
                                  m_DescriptorSet);
  }

  virtual ~Renderpass() = 0;

  [[nodiscard]] Ref<DescriptorSetLayout> &GetDescriptorSetLayout() {
    return m_DescriptorSetLayout;
  }

  [[nodiscard]] VkImageView GetRenderTargetColorImage() const {
    return m_ColorAttachment->GetTextureImageView();
  }
  [[nodiscard]] VkImageView GetRenderTargetDepthImage() const {
    return m_DepthAttachment->GetTextureImageView();
  }
  [[nodiscard]] VkRenderPass GetRenderPass() const { return m_RenderPass; }
  [[nodiscard]] VkDescriptorSet &GetDescriptorSet() { return m_DescriptorSet; }
  [[nodiscard]] VkDescriptorImageInfo GetImageInfo(VkImageLayout) const;
  [[nodiscard]] std::array<VkClearValue, 2> GetClearValues() const {
    return m_ClearValues;
  }
  [[nodiscard]] VkExtent2D GetExtent() const { return m_Extent; }
  [[nodiscard]] VkImage GetFinalImage() const {
    return m_ColorAttachment->GetTextureImage();
  }
  [[nodiscard]] Ref<Framebuffer> GetFramebuffers(int index = 0) const {
    return m_Framebuffers.at(index);
  }

protected:
  // Create render target
  virtual bool CreateRenderTarget() = 0;
  // Create render pass
  virtual bool CreateRenderPass() = 0;
  // Create framebuffer
  virtual bool CreateFramebuffer() = 0;

  virtual void CreateClearValues() = 0;

  void WriteToDescriptorSet();

  // Members
  Device &m_Device;
  Ref<Texture2D> m_ColorAttachment;
  Ref<Texture2D> m_DepthAttachment;

  // Descriptor sets to write images to. They will be sampled in the shader
  // (shadow, post-processing, etc.)
  VkDescriptorSet m_DescriptorSet{};
  Ref<DescriptorSetLayout> m_DescriptorSetLayout{};

  // Vulkan stuff
  VkRenderPass m_RenderPass{};
  VkExtent2D m_Extent{};
  std::array<VkClearValue, 2> m_ClearValues{};

  std::vector<Ref<Framebuffer>> m_Framebuffers;
};
} // namespace Engine

#endif // RENDERPASS_BASE_H
