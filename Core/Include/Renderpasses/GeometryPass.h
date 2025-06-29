//
// Created by alexa on 15/04/2025.
//

#ifndef GEOMETRYPASS_H
#define GEOMETRYPASS_H

#include "Engine/Renderpass.h"

namespace Engine {
    class GeometryPass : public Renderpass {
    public:
        GeometryPass(Device& device, const VkExtent2D& extent, const Ref<DescriptorSetLayout>& descriptorSetLayout)
            : Renderpass(device, extent, descriptorSetLayout){}

        ~GeometryPass() override {
            if (m_Framebuffer != VK_NULL_HANDLE) vkDestroyFramebuffer(m_Device.vk_GetDevice(), m_Framebuffer, nullptr);

            if (colorAttachment) colorAttachment->Destroy();
            if (depthAttachment) depthAttachment->Destroy();

            if (m_RenderPass != VK_NULL_HANDLE) vkDestroyRenderPass(m_Device.vk_GetDevice(), m_RenderPass, nullptr);
        }

        static Ref<GeometryPass> Initialize(Device& device, VkExtent2D extent, Ref<DescriptorSetLayout>& descriptorSetLayout) {
            auto pass = std::make_shared<GeometryPass>(device, extent, descriptorSetLayout);
            pass->CreateClearValues();
            if (!pass->CreateRenderTarget()) return nullptr;
            if (!pass->CreateRenderPass()) return nullptr;
            if (!pass->CreateFramebuffer()) return nullptr;
            return pass;
        }

        void Invalidate(const VkExtent2D& extent) {
            m_Extent = extent;
            if (colorAttachment) colorAttachment->Destroy();
            if (depthAttachment) depthAttachment->Destroy();

            if (m_Framebuffer != VK_NULL_HANDLE) vkDestroyFramebuffer(m_Device.vk_GetDevice(), m_Framebuffer, nullptr);

            CreateRenderTarget();
            CreateFramebuffer();
        }

        [[nodiscard]] VkFramebuffer GetFramebuffer() const { return m_Framebuffer; }


    private:
        bool CreateRenderTarget() override;
        bool CreateRenderPass() override;
        bool CreateFramebuffer() override;
        void CreateClearValues() override;


        VkFramebuffer m_Framebuffer{};
    };
}



#endif //GEOMETRYPASS_H
