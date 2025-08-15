//
// Created by alexa on 15/04/2025.
//

#ifndef GEOMETRYPASS_H
#define GEOMETRYPASS_H

#include "Engine/Renderer/Framebuffer.h"
#include "Engine/Renderer/Renderpass.h"

namespace Engine {
    class GeometryPass : public Renderpass {
    public:
        GeometryPass(Device& device, const VkExtent2D& extent, const Ref<DescriptorSetLayout>& descriptorSetLayout)
            : Renderpass(device, extent, descriptorSetLayout){}

        ~GeometryPass() override {
            for (auto& framebuffer : m_Framebuffers) {
                framebuffer->Destroy();
            }

            if (colorAttachment) colorAttachment->Destroy();
            if (depthAttachment) depthAttachment->Destroy();

            if (m_RenderPass != VK_NULL_HANDLE) vkDestroyRenderPass(m_Device.GetDevice(), m_RenderPass, nullptr);
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

            for (auto& framebuffer : m_Framebuffers) {
                framebuffer->Destroy();
            }

            m_Framebuffers.clear();

            CreateRenderTarget();
            CreateFramebuffer();
        }

        [[nodiscard]] std::vector<Ref<Framebuffer>> GetFramebuffers() const { return m_Framebuffers; }


    private:
        bool CreateRenderTarget() override;
        bool CreateRenderPass() override;
        bool CreateFramebuffer() override;
        void CreateClearValues() override;
    };
}



#endif //GEOMETRYPASS_H
