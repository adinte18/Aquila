#ifndef LUTPASS_H
#define LUTPASS_H

#include "Engine/Renderer/Renderpass.h"

namespace Engine {

    class LUTPass : public Renderpass {
    public:
        LUTPass(Device& device, const VkExtent2D& extent, const Ref<DescriptorSetLayout>& descriptorSetLayout)
            : Renderpass(device, extent, descriptorSetLayout){}

        ~LUTPass() override {
            for (const auto& framebuffer : m_Framebuffers) {
                framebuffer->Destroy();
            }

            if (colorAttachment) colorAttachment->Destroy();
            if (depthAttachment) depthAttachment->Destroy();

            if (m_RenderPass != VK_NULL_HANDLE) vkDestroyRenderPass(m_Device.GetDevice(), m_RenderPass, nullptr);
        }

        static Ref<LUTPass> Initialize(Device& device, VkExtent2D extent,  Ref<DescriptorSetLayout>& descriptorSetLayout) {
            auto pass = std::make_shared<LUTPass>(device, extent, descriptorSetLayout);
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

            for (const auto& framebuffer : m_Framebuffers) {
                framebuffer->Destroy();
            }

            CreateRenderTarget();
            CreateFramebuffer();
        }

        [[nodiscard]] Ref<Framebuffer> GetFramebuffer() const { return m_Framebuffers[0]; }

    private:
        bool CreateRenderTarget() override;
        bool CreateRenderPass() override;
        bool CreateFramebuffer() override;
        void CreateClearValues() override;
    };

}



#endif //LUTPASS_H
