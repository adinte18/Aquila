//
// Created by alexa on 16/04/2025.
//

#ifndef SHADOWPASS_H
#define SHADOWPASS_H

#include "Engine/Renderer/Renderpass.h"

namespace Engine{
    class ShadowPass : public Renderpass {
    public:
        ShadowPass(Device& device, const VkExtent2D& extent, const Ref<DescriptorSetLayout>& descriptorSetLayout)
            : Renderpass(device, extent, descriptorSetLayout){}

        ~ShadowPass() override {
            for (const auto& framebuffer : m_Framebuffers) {
                framebuffer->Destroy();
            }

            if (colorAttachment) colorAttachment->Destroy();
            if (depthAttachment) depthAttachment->Destroy();

            if (m_RenderPass != VK_NULL_HANDLE) vkDestroyRenderPass(m_Device.GetDevice(), m_RenderPass, nullptr);
        }

        static Ref<ShadowPass> Initialize(Device& device, VkExtent2D extent, Ref<DescriptorSetLayout>& descriptorSetLayout) {
            auto pass = std::make_shared<ShadowPass>(device, extent, descriptorSetLayout);
            pass->CreateClearValues();
            if (!pass->CreateRenderTarget()) return nullptr;
            if (!pass->CreateRenderPass()) return nullptr;
            if (!pass->CreateFramebuffer()) return nullptr;
            return pass;
        }

    private:
        bool CreateRenderTarget() override;
        bool CreateRenderPass() override;
        bool CreateFramebuffer() override;
        void CreateClearValues() override;
    };
};




#endif //SHADOWPASS_H
