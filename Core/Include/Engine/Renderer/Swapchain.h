//
// Created by adinte on 7/5/24.
//

#ifndef VK_APP_VK_SWAP_CHAIN_H
#define VK_APP_VK_SWAP_CHAIN_H

#include "Engine/Renderer/Device.h"
#include "Engine/Renderer/Framebuffer.h"

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
        static VkSurfaceFormatKHR ChooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR> &availableFormats);
        static VkPresentModeKHR ChooseSwapPresentMode(const std::vector<VkPresentModeKHR> &availablePresentModes);
        VkExtent2D ChooseSwapExtent(const VkSurfaceCapabilitiesKHR &capabilities) const;
        [[nodiscard]] uint32_t Width() const { return windowExtent.width; }
        [[nodiscard]] uint32_t Height() const { return windowExtent.height; }
        [[nodiscard]] VkRenderPass GetRenderpass() const { return renderPass; }
        [[nodiscard]] size_t GetImageCount() const { return swapChainImages.size(); }
        [[nodiscard]] VkFormat GetImageFormat() const { return swapChainImageFormat; }
        [[nodiscard]] VkExtent2D GetExtent() const { return swapChainExtent; }
        VkFramebuffer GetFramebuffer(int index) { return swapChainFramebuffers[index]->GetHandle(); }
        VkImageView GetImageView(int index) { return swapChainImageViews[index]; }
        VkResult GetNextImage(uint32_t *imageIndex) const;
        VkResult SubmitCommandBuffers(const VkCommandBuffer *buffers, const uint32_t *imageIndex);
        float GetExtentAspectRatio() {
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

        std::vector<Ref<Framebuffer>> swapChainFramebuffers;
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

        void Initialize();

        void CreateSwapchain();

        void CreateImageViews();

        void CreateRenderpass();

        void CreateFramebuffers();

        void CreateSyncObjects();

        void CreateDepthRessources();

        VkFormat FindDepthFormat();
    };
}

#endif //VK_APP_VK_SWAP_CHAIN_H
