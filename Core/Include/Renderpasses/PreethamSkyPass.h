//
// Created by alexa on 21/04/2025.
//

#ifndef PREETHAMSKYPASS_H
#define PREETHAMSKYPASS_H

#include "Engine/Renderpass.h"

namespace Engine {
    class PreethamSkyPass : public Renderpass {
    public:
        PreethamSkyPass(Device& device, const VkExtent2D& extent, const Ref<DescriptorSetLayout>& descriptorSetLayout)
            : Renderpass(device, extent, descriptorSetLayout){}

        ~PreethamSkyPass() override {
            for (const auto& framebuffer : m_Framebuffers) {
                if (framebuffer != VK_NULL_HANDLE) {
                    vkDestroyFramebuffer(m_Device.vk_GetDevice(), framebuffer, nullptr);
                }
            }

            for (const auto& imageView : m_CubemapFaceViews) {
                if (imageView != VK_NULL_HANDLE) {
                    vkDestroyImageView(m_Device.vk_GetDevice(), imageView, nullptr);
                }
            }

            if (colorAttachment) colorAttachment->Destroy();
            if (depthAttachment) depthAttachment->Destroy();

            if (m_RenderPass != VK_NULL_HANDLE) vkDestroyRenderPass(m_Device.vk_GetDevice(), m_RenderPass, nullptr);
        }

        static Ref<PreethamSkyPass> Initialize(Device& device, VkExtent2D extent, Ref<DescriptorSetLayout>& descriptorSetLayout) {
            auto pass = std::make_shared<PreethamSkyPass>(device, extent, descriptorSetLayout);
            pass->CreateClearValues();
            if (!pass->CreateRenderTarget()) return nullptr;
            if (!pass->CreateRenderPass()) return nullptr;
            if (!pass->CreateFramebuffer()) return nullptr;
            return pass;
        }

        [[nodiscard]] VkFramebuffer GetFramebuffer(const int face) const { return m_Framebuffers[face]; }

    private:
        bool CreateRenderTarget() override;
        bool CreateRenderPass() override;
        bool CreateFramebuffer() override;
        void CreateClearValues() override;

        std::vector<VkFramebuffer> m_Framebuffers{6};
        std::vector<VkImageView> m_CubemapFaceViews{6};
    };
}



#endif //PREETHAMSKYPASS_H
