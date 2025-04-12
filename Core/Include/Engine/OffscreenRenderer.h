//
// Created by alexa on 19/10/2024.
//

#ifndef OFFSCREENRENDERER_H
#define OFFSCREENRENDERER_H

#include <Engine/Device.h>
#include <Engine/Renderpass.h>

namespace Engine {
    class OffscreenRenderer {

    public:
        explicit OffscreenRenderer(Device& device);
        ~OffscreenRenderer();

        OffscreenRenderer(const OffscreenRenderer&) = delete;
        OffscreenRenderer& operator=(const OffscreenRenderer&) = delete;

        void Initialize(uint32_t width, uint32_t height);
        void Resize(VkExtent2D newExtent);

        void TransitionImages(VkCommandBuffer commandBuffer, RenderPassType src, RenderPassType dst);

        void BeginRenderPass(VkCommandBuffer commandBuffer, RenderPassType type);
        void BeginRenderPass(VkCommandBuffer commandBuffer, RenderPassType type, int cubemapFace);
        void EndRenderPass(VkCommandBuffer commandBuffer);

        VkCommandBuffer BeginFrame();
        void EndFrame();

        [[nodiscard]] float GetAspectRatio() const { return static_cast<float>(m_Extent.width) / static_cast<float>(m_Extent.height); }
        [[nodiscard]] VkDescriptorImageInfo GetImageInfo(RenderPassType type, VkImageLayout layout) const;
        [[nodiscard]] VkRenderPass GetRenderPass(RenderPassType type) const { return m_Renderpass.GetRenderPass(type); }
        [[nodiscard]] VkDescriptorSet& GetRenderPassDescriptorSet(RenderPassType type) {return m_Renderpass.GetDescriptorSet(type); }
        [[nodiscard]] std::unique_ptr<DescriptorPool>& GetRenderPassDescriptorPool(RenderPassType type) {return m_Renderpass.GetDescriptorPool(type); }
        [[nodiscard]] std::unique_ptr<DescriptorSetLayout>& GetRenderPassDescriptorLayout(RenderPassType type) {return m_Renderpass.GetDescriptorLayout(type); }
    private:
        Device& m_Device;
        VkExtent2D m_Extent{};
        Renderpass m_Renderpass;
        std::vector<VkCommandBuffer> m_CommandBuffers;

        uint32_t m_CurrentImageID{0};
        bool m_FrameStarted{false};
        int m_CurrentFrameID{0};

        void CreateCommandBuffers();
        VkCommandBuffer GetCurrentCommandBuffer();
        void FreeCommandBuffers();

    };
}

#endif //OFFSCREENRENDERER_H
