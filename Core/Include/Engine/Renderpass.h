//
// Created by alexa on 15/04/2025.
//

#ifndef RENDERPASS_BASE_H
#define RENDERPASS_BASE_H

#include "Engine/Rendertarget.h"

class Renderpass {
public:
    Renderpass(Engine::Device& device, const VkExtent2D& extent) : m_Device(device), m_Extent(extent) {
        // allocate space for 1 image
        m_DescriptorPool = Engine::DescriptorPool::Builder(m_Device)
        .addPoolSize(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1)
        .setMaxSets(1)
        .build();

        // define what are we binding - here 1 image (pass result)
        m_DescriptorSetLayout = Engine::DescriptorSetLayout::Builder(m_Device)
        .addBinding(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT)
        .build();

        // allocate descriptor set
        m_DescriptorPool->allocateDescriptor(m_DescriptorSetLayout->getDescriptorSetLayout(), m_DescriptorSet);
    }

    virtual ~Renderpass() = 0;

    [[nodiscard]] std::unique_ptr<Engine::DescriptorSetLayout>& GetDescriptorSetLayout() { return m_DescriptorSetLayout; }
    [[nodiscard]] std::unique_ptr<Engine::DescriptorPool>&      GetDescriptorPool()      {return m_DescriptorPool; }

    [[nodiscard]] VkImageView               GetRenderTargetColorImage() const { return colorAttachment->GetTextureImageView(); }
    [[nodiscard]] VkImageView               GetRenderTargetDepthImage() const { return depthAttachment->GetTextureImageView(); }
    [[nodiscard]] VkRenderPass              GetRenderPass()             const { return m_RenderPass; }
    [[nodiscard]] VkDescriptorSet&          GetDescriptorSet()                { return m_DescriptorSet; }
    [[nodiscard]] VkDescriptorImageInfo     GetImageInfo(VkImageLayout) const;
    [[nodiscard]] std::array<VkClearValue, 2> GetClearValues()          const { return m_ClearValues; }
    [[nodiscard]] VkExtent2D                GetExtent()                 const { return m_Extent; }
    [[nodiscard]] VkImage                   GetFinalImage()             const { return colorAttachment->GetTextureImage(); }


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
    std::shared_ptr<Engine::Texture2D> colorAttachment;
    std::shared_ptr<Engine::Texture2D> depthAttachment;

    // Descriptor sets to write images to. They will be sampled in the shader (shadow, post-processing, etc.)
    VkDescriptorSet m_DescriptorSet{};
    std::unique_ptr<Engine::DescriptorPool> m_DescriptorPool{};
    std::unique_ptr<Engine::DescriptorSetLayout> m_DescriptorSetLayout{};

    // Vulkan stuff
    VkRenderPass m_RenderPass{};
    VkExtent2D m_Extent{};
    std::array<VkClearValue, 2> m_ClearValues{};
};



#endif //RENDERPASS_BASE_H
