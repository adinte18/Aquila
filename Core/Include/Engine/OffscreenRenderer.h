//
// Created by alexa on 19/10/2024.
//

#ifndef OFFSCREENRENDERER_H
#define OFFSCREENRENDERER_H

#include "Common.h"
#include <Engine/Device.h>
#include <Engine/RenderPass.h>
#include <Engine/Framebuffer.h>

namespace Engine {
    class OffscreenRenderer {

    public:
        OffscreenRenderer(Device& device, VkExtent2D extent = {800, 600});
        ~OffscreenRenderer();

        OffscreenRenderer(const OffscreenRenderer&) = delete;
        OffscreenRenderer& operator=(const OffscreenRenderer&) = delete;


        void Recreate(VkExtent2D newExtent);
        void BeginRenderPass(VkCommandBuffer commandBuffer);
        void EndRenderPass(VkCommandBuffer commandBuffer);

        Framebuffer& GetFramebuffer() { return framebuffer; }

    private:
        Device& device;
        VkExtent2D extent;
        VkRenderPass renderPass;
        Framebuffer framebuffer;
    };;
}

#endif //OFFSCREENRENDERER_H
