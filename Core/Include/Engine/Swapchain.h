//
// Created by adinte on 7/5/24.
//

#ifndef VK_APP_VK_SWAP_CHAIN_H
#define VK_APP_VK_SWAP_CHAIN_H

#include "Engine/Device.h"

#include <vulkan/vulkan.h>
#include <memory>

namespace Engine{
    class Swapchain {

    public :
        static constexpr int MAX_FRAMES_IN_FLIGHT = 2;

        Swapchain(Device &device, VkExtent2D extent);
        Swapchain(Device &device, VkExtent2D extent, Ref<Swapchain> oldSC);

        ~Swapchain();

        Swapchain(const Swapchain&) = delete;
        Swapchain& operator=(const Swapchain&) = delete;

        // Helper functions
        static VkSurfaceFormatKHR vk_ChooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR> &availableFormats);
        static VkPresentModeKHR vk_ChooseSwapPresentMode(const std::vector<VkPresentModeKHR> &availablePresentModes);
        VkExtent2D vk_ChooseSwapExtent(const VkSurfaceCapabilitiesKHR &capabilities) const;
        [[nodiscard]] uint32_t width() const { return windowExtent.width; }
        [[nodiscard]] uint32_t height() const { return windowExtent.height; }
        [[nodiscard]] VkRenderPass getRenderPass() const { return renderPass; }
        [[nodiscard]] size_t imageCount() const { return swapChainImages.size(); }
        [[nodiscard]] VkFormat getSwapChainImageFormat() const { return swapChainImageFormat; }
        [[nodiscard]] VkExtent2D getSwapChainExtent() const { return swapChainExtent; }
        VkFramebuffer getFrameBuffer(int index) { return swapChainFramebuffers[index]; }
        VkImageView getImageView(int index) { return swapChainImageViews[index]; }
        VkResult vk_GetNextImage(uint32_t *imageIndex) const;
        VkResult vk_SubmitCommandBuffers(const VkCommandBuffer *buffers, const uint32_t *imageIndex);
        float vk_GetExtentAspectRatio() {
            return static_cast<float>(swapChainExtent.width) / static_cast<float>(swapChainExtent.height);
        }

        [[nodiscard]]
        bool vk_CompareSCFormats(const Swapchain& swapChain) const
        {
            return swapChain.swapChainDepthFormat == swapChainDepthFormat &&
                swapChain.swapChainImageFormat == swapChainImageFormat;
        }

        VkFormat swapChainImageFormat;
        VkFormat swapChainDepthFormat;
        VkExtent2D swapChainExtent;

        std::vector<VkFramebuffer> swapChainFramebuffers;
        VkRenderPass renderPass;

        std::vector<VkImage> depthImages;
        std::vector<VkDeviceMemory> depthImageMemorys;
        std::vector<VkImageView> depthImageViews;
        std::vector<VkImage> swapChainImages;
        std::vector<VkImageView> swapChainImageViews;

        Device &device;
        VkExtent2D windowExtent;

        VkSwapchainKHR swapChain;
        Ref<Swapchain> oldSC;

        std::vector<VkSemaphore> imageAvailableSemaphores;
        std::vector<VkSemaphore> renderFinishedSemaphores;
        std::vector<VkFence> inFlightFences;
        std::vector<VkFence> imagesInFlight;
        size_t currentFrame = 0;

        void SwapchainInit();

        void vk_CreateSwapChain();

        void vk_CreateImageViews();

        void vk_CreateRenderPass();

        void vk_CreateFramebuffers();

        void vk_CreateSyncObjects();

        void vk_CreateDepthRessources();

        VkFormat vk_FindDepthFormat();
    };
}

#endif //VK_APP_VK_SWAP_CHAIN_H
