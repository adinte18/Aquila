//
// Created by alexa on 15/04/2025.
//

#ifndef RENDERPASS_BASE_H
#define RENDERPASS_BASE_H

#include <array>
#include "DescriptorAllocator.h"
#include "Engine/Renderer/Rendertarget.h"
#include "Engine/Renderer/Framebuffer.h"

class Renderpass {
public:
    Renderpass(Engine::Device& device, const VkExtent2D& extent, const Ref<Engine::DescriptorSetLayout>& descriptorSetLayout)
    : m_Device(device), m_Extent(extent),
      m_DescriptorSetLayout(descriptorSetLayout)
    {
        Engine::DescriptorAllocator::Allocate(descriptorSetLayout->GetDescriptorSetLayout(),  m_DescriptorSet);
    }

    virtual ~Renderpass() = 0;

    [[nodiscard]] Ref<Engine::DescriptorSetLayout>& GetDescriptorSetLayout() { return m_DescriptorSetLayout; }

    [[nodiscard]] VkImageView               GetRenderTargetColorImage()     const { return colorAttachment->GetTextureImageView(); }
    [[nodiscard]] VkImageView               GetRenderTargetDepthImage()     const { return depthAttachment->GetTextureImageView(); }
    [[nodiscard]] VkRenderPass              GetRenderPass()                 const { return m_RenderPass; }
    [[nodiscard]] VkDescriptorSet&          GetDescriptorSet()                    { return m_DescriptorSet; }
    [[nodiscard]] VkDescriptorImageInfo     GetImageInfo(VkImageLayout)     const;
    [[nodiscard]] std::array<VkClearValue, 2> GetClearValues()              const { return m_ClearValues; }
    [[nodiscard]] VkExtent2D                GetExtent()                     const { return m_Extent; }
    [[nodiscard]] VkImage                   GetFinalImage()                 const { return colorAttachment->GetTextureImage(); }
    [[nodiscard]] Ref<Engine::Framebuffer> GetFramebuffers(int index = 0)   const { return m_Framebuffers.at(index); }

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
    Engine::Device& m_Device;
    Ref<Engine::Texture2D> colorAttachment;
    Ref<Engine::Texture2D> depthAttachment;

    // Descriptor sets to write images to. They will be sampled in the shader (shadow, post-processing, etc.)
    VkDescriptorSet m_DescriptorSet{};
    Ref<Engine::DescriptorSetLayout> m_DescriptorSetLayout{};

    // Vulkan stuff
    VkRenderPass m_RenderPass{};
    VkExtent2D m_Extent{};
    std::array<VkClearValue, 2> m_ClearValues{};

    std::vector<Ref<Engine::Framebuffer>> m_Framebuffers;
};



#endif //RENDERPASS_BASE_H
