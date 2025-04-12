//
// Created by alexa on 19/10/2024.
//

#ifndef RENDERER_H
#define RENDERER_H

#include <cassert>

#include <Engine/Window.h>
#include <Engine/Device.h>
#include <Engine/Swapchain.h>

namespace Engine {
    class OnscreenRenderer {
    public:
        OnscreenRenderer(Window &window, Device &device);
        ~OnscreenRenderer();

        OnscreenRenderer(const OnscreenRenderer&) = delete;
        OnscreenRenderer& operator=(const OnscreenRenderer&) = delete;

        [[nodiscard]]
        VkRenderPass vk_GetCurrentRenderPass() const {return swapChain->renderPass;}

        float vk_GetAspectRatio() const { return swapChain->vk_GetExtentAspectRatio(); }

        [[nodiscard]]
        bool vk_FrameStarted() const { return frameStarted; }

        [[nodiscard]]
        VkCommandBuffer vk_GetCurrentCommandBuffer() const {
            assert(frameStarted && "Cannot get command buffer when frame not in progress");
            return commandBuffers[currentFrameID];
        }

        VkCommandBuffer vk_BeginFrame();
        void vk_EndFrame();
        VkExtent2D vk_GetSwapChainExtent() const;

        void vk_BeginSwapChainRenderPass(VkCommandBuffer commandBuffer);
        void vk_EndSwapChainRenderPass(VkCommandBuffer commandBuffer);

        [[nodiscard]] VkImageView vk_GetCurrentImageView() const;

        [[nodiscard]] VkFormat vk_GetDepthFormat() {
            return swapChain->vk_FindDepthFormat();
        }

        [[nodiscard]]
        int vk_GetFrameID() const {
            assert(frameStarted && "Cannot get frame index when frame not in progress");
            return currentFrameID;
        }
    private:
        Window& window;
        Device& device;
        std::unique_ptr<Swapchain> swapChain;
        std::vector<VkCommandBuffer> commandBuffers;

        uint32_t currentImageID{0};
        bool frameStarted{false};
        int currentFrameID{0};

        void vk_CreateCommandBuffers();

        void vk_FreeCommandBuffers();

        void vk_RecreateSwapChain();

    };

};



#endif //RENDERER_H
