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

        VkDescriptorImageInfo GetDepthInfo(RenderPassType type) const;

        OffscreenRenderer(const OffscreenRenderer&) = delete;
        OffscreenRenderer& operator=(const OffscreenRenderer&) = delete;

        void Initialize(uint32_t width, uint32_t height);
        void Resize(VkExtent2D newExtent);

        void BeginRenderPass(VkCommandBuffer commandBuffer, RenderPassType type);
        void EndRenderPass(VkCommandBuffer commandBuffer);

        [[nodiscard]] VkRenderPass GetRenderPass(RenderPassType type) const { return renderpass.GetRenderPass(type); }
        [[nodiscard]] VkDescriptorSet GetRenderPassImage(RenderPassType type) const {return renderpass.GetImage(type); }

    private:
        Device& device;
        VkExtent2D extent{};
        Renderpass renderpass;
    };
}

#endif //OFFSCREENRENDERER_H
