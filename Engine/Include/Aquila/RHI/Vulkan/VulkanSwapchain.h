#ifndef AQUILA_VULKAN_SWAPCHAIN_H
#define AQUILA_VULKAN_SWAPCHAIN_H

#include "GraphicsPCH.h"
#include "Aquila/Foundation/PrimitiveTypes.h"
#include "Aquila/RHI/Backend/IRHISwapchain.h"
#include "Aquila/RHI/Vulkan/VulkanTypes.h"

namespace Aquila::RHI {

class VulkanDevice;

class VulkanSwapchain final : public IRHISwapchain {
  public:
	VulkanSwapchain(VulkanDevice &device, VkExtent2D extent);
	VulkanSwapchain(VulkanDevice &device, VkExtent2D extent, Ref<VulkanSwapchain> previous);
	~VulkanSwapchain() override;

	AQUILA_NONCOPYABLE(VulkanSwapchain);

	// IRHISwapchain semaphores managed internally per frame
	bool AcquireNextImage(uint32 &outImageIndex) override;
	bool Present(uint32 imageIndex) override;
	[[nodiscard]] bool NeedsResize() const override { return m_NeedsResize; }
	void Resize(uint32 width, uint32 height) override;

	[[nodiscard]] uint32 GetWidth() const override { return m_Extent.width; }
	[[nodiscard]] uint32 GetHeight() const override { return m_Extent.height; }
	[[nodiscard]] TextureFormat GetFormat() const override;
	[[nodiscard]] uint32 GetImageCount() const override { return static_cast<uint32>(m_Images.size()); }

	// Vulkan-specific accessors (render loops, ImGui, etc.)
	[[nodiscard]] VkFormat GetImageFormat() const { return m_ImageFormat; }
	[[nodiscard]] VkFormat GetDepthFormat() const { return m_DepthFormat; }
	[[nodiscard]] VkExtent2D GetExtent() const { return m_Extent; }
	[[nodiscard]] VkImage GetImage(uint32 index) const { return m_Images[index]; }
	[[nodiscard]] VkImageView GetImageView(uint32 index) const { return m_ImageViews[index]; }
	[[nodiscard]] VkImageView GetDepthImageView(uint32 index) const { return m_DepthImageViews[index]; }
	[[nodiscard]] VkImage GetDepthImage(uint32 index) const { return m_DepthAllocations[index].image; }

	// Semaphore accessors for external sync (render loop)
	[[nodiscard]] VkSemaphore GetImageAvailableSemaphore(uint32 frameIndex) const {
		return m_ImageAvailableSemaphores[frameIndex];
	}
	[[nodiscard]] VkSemaphore GetRenderFinishedSemaphore(uint32 frameIndex) const {
		return m_RenderFinishedSemaphores[frameIndex];
	}

	// Raw acquire/present (render loop with explicit semaphore control)
	VkResult AcquireNextImageRaw(uint32 *imageIndex, VkSemaphore imageAvailableSemaphore) const;
	VkResult PresentImageRaw(const uint32 *imageIndex, VkSemaphore renderFinishedSemaphore);

	[[nodiscard]] uint32 GetLastAcquiredFrameIndex() const {
		return (m_CurrentFrame + k_MaxFramesInFlight - 1) % k_MaxFramesInFlight;
	}

	[[nodiscard]] bool IsImageInitialized(uint32 index) const {
		return index < m_ImageInitialized.size() && m_ImageInitialized[index];
	}
	void MarkImageInitialized(uint32 index) {
		if (index < m_ImageInitialized.size()) {
			m_ImageInitialized[index] = true;
		}
	}

	[[nodiscard]] f32 AspectRatio() const {
		return static_cast<f32>(m_Extent.width) / static_cast<f32>(m_Extent.height);
	}
	[[nodiscard]] VkFormat FindDepthFormat();

  private:
	void Initialize();
	void CreateSwapchain(VkSwapchainKHR oldHandle = VK_NULL_HANDLE);
	void CreateImageViews();
	void CreateDepthResources();
	void CreateSyncObjects();
	void DestroyImageResources();

	VkSurfaceFormatKHR ChooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR> &availableFormats);
	VkPresentModeKHR ChooseSwapPresentMode(const std::vector<VkPresentModeKHR> &availablePresentModes);
	[[nodiscard]] VkExtent2D ChooseSwapExtent(const VkSurfaceCapabilitiesKHR &capabilities) const;

	VulkanDevice &m_Device;
	VkExtent2D m_WindowExtent;
	VkSwapchainKHR m_Swapchain = VK_NULL_HANDLE;
	Ref<VulkanSwapchain> m_OldSwapchain;

	VkFormat m_ImageFormat{};
	VkFormat m_DepthFormat{};
	VkExtent2D m_Extent{};

	std::vector<VkImage> m_Images;
	std::vector<VkImageView> m_ImageViews;

	std::vector<ImageAllocation> m_DepthAllocations;
	std::vector<VkImageView> m_DepthImageViews;
	std::vector<bool> m_ImageInitialized;

	// Frame-indexed sync objects owned by the swapchain
	static constexpr uint32 k_MaxFramesInFlight = 3;
	std::vector<VkSemaphore> m_ImageAvailableSemaphores;
	std::vector<VkSemaphore> m_RenderFinishedSemaphores;
	uint32 m_CurrentFrame = 0;
	bool m_NeedsResize = false;
};

} // namespace Aquila::RHI
#endif
