#ifndef LUTPASS_H
#define LUTPASS_H

#include "Engine/Renderpass.h"

namespace Engine {

    class LUTPass : public Renderpass {
    public:
        LUTPass(Device& device, const VkExtent2D& extent)
            : Renderpass(device, extent){}

        ~LUTPass() override {
            if (m_Framebuffer != VK_NULL_HANDLE) vkDestroyFramebuffer(m_Device.vk_GetDevice(), m_Framebuffer, nullptr);

            if (colorAttachment) colorAttachment->Destroy();
            if (depthAttachment) depthAttachment->Destroy();

            if (m_RenderPass != VK_NULL_HANDLE) vkDestroyRenderPass(m_Device.vk_GetDevice(), m_RenderPass, nullptr);
        }

        static std::shared_ptr<LUTPass> Initialize(Device& device, VkExtent2D extent) {
            auto pass = std::make_shared<LUTPass>(device, extent);
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



#endif //LUTPASS_H
