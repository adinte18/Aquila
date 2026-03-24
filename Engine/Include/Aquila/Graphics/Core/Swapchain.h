#ifndef AQUILA_SWAPCHAIN_H
#define AQUILA_SWAPCHAIN_H

#include "Aquila/Graphics/Core/Device.h"
#include "Aquila/Graphics/Pipeline/Rendertarget.h"
#include "Aquila/Graphics/Core/DeletionManager.h"

namespace Aquila::Graphics {

using namespace Aquila::Foundation;

class Swapchain {
  public:
	Swapchain(Device &device, VkExtent2D extent);
	Swapchain(Device &device, VkExtent2D extent, Ref<Swapchain> previous);
	~Swapchain();

	Swapchain(const Swapchain &) = delete;
	Swapchain &operator=(const Swapchain &) = delete;

	[[nodiscard]] VkFormat GetImageFormat() const { return swapChainImageFormat; }
	[[nodiscard]] VkExtent2D GetExtent() const { return swapChainExtent; }
	[[nodiscard]] size_t GetImageCount() const { return swapChainImages.size(); }

	[[nodiscard]] const std::array<VkClearValue, 2> &GetClearValues() const;

	VkResult AcquireNextImage(uint32_t *imageIndex, VkSemaphore imageAvailableSemaphore) const;
	VkResult PresentImage(const uint32_t *imageIndex, VkSemaphore renderFinishedSemaphore) const;

	[[nodiscard]] f32 ExtentAspectRatio() const {
		return static_cast<f32>(swapChainExtent.width) / static_cast<f32>(swapChainExtent.height);
	}

	[[nodiscard]] bool IsImageInitialized(uint32_t index) const {
		return index < m_ImageInitialized.size() && m_ImageInitialized[index];
	}
	void MarkImageInitialized(uint32_t index) {
		if (index < m_ImageInitialized.size()) {
			m_ImageInitialized[index] = true;
		}
	}

	[[nodiscard]] VkImageView GetImageView(uint32_t index) const {
		if (index >= swapChainImageViews.size()) {
			throw std::out_of_range("Swapchain image index out of range");
		}
		return swapChainImageViews[index];
	}

	[[nodiscard]] VkImageView GetCurrentImageView(uint32_t currentImageIndex) const {
		return GetImageView(currentImageIndex);
	}
	[[nodiscard]] VkImage GetCurrentImage(uint32 currentImageIndex) const { return swapChainImages[currentImageIndex]; }

	VkFormat FindDepthFormat();

  private:
	void Initialize();
	void CreateSwapchain();
	void CreateImageViews();
	void CreateDepthResources();

	VkSurfaceFormatKHR ChooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR> &availableFormats);
	VkPresentModeKHR ChooseSwapPresentMode(const std::vector<VkPresentModeKHR> &availablePresentModes);
	[[nodiscard]] VkExtent2D ChooseSwapExtent(const VkSurfaceCapabilitiesKHR &capabilities) const;

	VkFormat swapChainImageFormat;
	VkFormat swapChainDepthFormat;
	VkExtent2D swapChainExtent;

	std::vector<VkImage> swapChainImages;
	std::vector<VkImageView> swapChainImageViews;

	std::vector<Ref<RenderingPipeline::RenderTarget>> m_DepthTargets;

	Device &device;
	VkExtent2D windowExtent;

	VkSwapchainKHR swapChain;
	Ref<Swapchain> oldSC;
	std::vector<bool> m_ImageInitialized;
};

} // namespace Aquila::Graphics

#endif
