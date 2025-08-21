//
// Created by alexa on 15/04/2025.
//

#ifndef HDRITOCUBEMAPPASS_H
#define HDRITOCUBEMAPPASS_H

#include "Engine/Renderer/Descriptor.h"
#include "Engine/Renderer/Renderpass.h"


namespace Engine {
    class HDRiToCubemapPass : public Renderpass {
    public:
        HDRiToCubemapPass(Device& device, const VkExtent2D& extent, const Ref<DescriptorSetLayout>& descriptorSetLayout)
            : Renderpass(device, extent, descriptorSetLayout){}

        ~HDRiToCubemapPass() override {
            for (const auto& framebuffer : m_Framebuffers) {
                framebuffer->Destroy();
            }

            for (const auto& imageView : m_CubemapFaceViews) {
                if (imageView != VK_NULL_HANDLE) {
                    vkDestroyImageView(m_Device.GetDevice(), imageView, nullptr);
                }
            }

            if (colorAttachment) colorAttachment->Destroy();
            if (depthAttachment) depthAttachment->Destroy();

            if (m_RenderPass != VK_NULL_HANDLE) vkDestroyRenderPass(m_Device.GetDevice(), m_RenderPass, nullptr);
        }

        static Ref<HDRiToCubemapPass> Initialize(Device& device, VkExtent2D extent,  Ref<DescriptorSetLayout>& descriptorSetLayout) {
            auto pass = CreateRef<HDRiToCubemapPass>(device, extent, descriptorSetLayout);
            pass->CreateClearValues();
            if (!pass->CreateRenderTarget()) return nullptr;
            if (!pass->CreateRenderPass()) return nullptr;
            if (!pass->CreateFramebuffer()) return nullptr;
            return pass;
        }

        [[nodiscard]] Ref<Framebuffer> GetFramebuffer(const int face) const { return m_Framebuffers[face]; }

    private:
        bool CreateRenderTarget() override;
        bool CreateRenderPass() override;
        bool CreateFramebuffer() override;
        void CreateClearValues() override;

        std::vector<VkImageView> m_CubemapFaceViews{6};
    };
}



#endif //HDRITOCUBEMAPPASS_H
