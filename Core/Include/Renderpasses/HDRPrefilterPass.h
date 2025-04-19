//
// Created by alexa on 16/04/2025.
//

#ifndef HDRPREFILTERPASS_H
#define HDRPREFILTERPASS_H

#include "Engine/Renderpass.h"

namespace Engine {
    class HDRPrefilterPass : public Renderpass {
    public:
        HDRPrefilterPass(Device& device, const VkExtent2D& extent)
            : Renderpass(device, extent){}

        ~HDRPrefilterPass() override {
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

        static std::shared_ptr<HDRPrefilterPass> Initialize(Device& device, VkExtent2D extent) {
            auto pass = std::make_shared<HDRPrefilterPass>(device, extent);
            pass->CreateClearValues();
            if (!pass->CreateRenderTarget()) return nullptr;
            if (!pass->CreateRenderPass()) return nullptr;
            if (!pass->CreateFramebuffer()) return nullptr;
            return pass;
        }

        [[nodiscard]] VkFramebuffer GetFramebuffer(const int face) const { return m_Framebuffers[face]; }
        [[nodiscard]] uint32_t GetMipLevels() const { return colorAttachment->GetMipLevels(); }

    private:
        bool CreateRenderTarget() override;
        bool CreateRenderPass() override;
        bool CreateFramebuffer() override;
        void CreateClearValues() override;


        std::vector<VkFramebuffer> m_Framebuffers{6};
        std::vector<VkImageView> m_CubemapFaceViews{6};
    };
}




#endif //HDRPREFILTERPASS_H
