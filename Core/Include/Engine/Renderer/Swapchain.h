#pragma once

#include "Engine/Renderer/Device.h"
#include "Engine/Renderer/Framebuffer.h"

namespace Engine {

class Swapchain {
public:
  static constexpr int MAX_FRAMES_IN_FLIGHT = 2;

  Swapchain(Device &device, VkExtent2D extent);
  Swapchain(Device &device, VkExtent2D extent, Ref<Swapchain> previous);
  ~Swapchain();

  Swapchain(const Swapchain &) = delete;
  Swapchain &operator=(const Swapchain &) = delete;

  VkFramebuffer GetFramebuffer(int index) {
    return swapChainFramebuffers[index]->GetHandle();
  }
  VkRenderPass GetRenderpass() { return renderPass; }
  VkImageView GetImageView(int index) { return swapChainImageViews[index]; }
  size_t GetImageCount() { return swapChainImages.size(); }
  VkFormat GetImageFormat() { return swapChainImageFormat; }
  VkExtent2D GetExtent() { return swapChainExtent; }
  VkImage GetImage(int index) { return swapChainImages[index]; }

  uint32_t Width() { return swapChainExtent.width; }
  uint32_t Height() { return swapChainExtent.height; }

  float ExtentAspectRatio() {
    return static_cast<float>(swapChainExtent.width) /
           static_cast<float>(swapChainExtent.height);
  }

  VkFormat FindDepthFormat();

  VkResult AcquireNextImage(uint32_t *imageIndex,
                            VkSemaphore imageAvailableSemaphore) const;
  VkResult PresentImage(const uint32_t *imageIndex,
                        VkSemaphore renderFinishedSemaphore) const;

  bool vk_CompareSCFormats(const Swapchain &swapChain) const {
    return swapChain.swapChainDepthFormat == swapChainDepthFormat &&
           swapChain.swapChainImageFormat == swapChainImageFormat;
  }

private:
  void Initialize();
  void CreateSwapchain();
  void CreateImageViews();
  void CreateRenderpass();
  void CreateDepthRessources();
  void CreateFramebuffers();

  VkSurfaceFormatKHR ChooseSwapSurfaceFormat(
      const std::vector<VkSurfaceFormatKHR> &availableFormats);
  VkPresentModeKHR ChooseSwapPresentMode(
      const std::vector<VkPresentModeKHR> &availablePresentModes);
  VkExtent2D
  ChooseSwapExtent(const VkSurfaceCapabilitiesKHR &capabilities) const;

  VkSwapchainKHR swapChain = VK_NULL_HANDLE;
  Ref<Swapchain> oldSC;
  Device &device;
  VkExtent2D windowExtent;

  std::vector<VkImage> swapChainImages;
  std::vector<VkImageView> swapChainImageViews;
  VkFormat swapChainImageFormat;
  VkExtent2D swapChainExtent;

  std::vector<VkImage> depthImages;
  std::vector<VkDeviceMemory> depthImageMemorys;
  std::vector<VkImageView> depthImageViews;
  VkFormat swapChainDepthFormat;

  VkRenderPass renderPass = VK_NULL_HANDLE;
  std::vector<Ref<Framebuffer>> swapChainFramebuffers;
};

} // namespace Engine