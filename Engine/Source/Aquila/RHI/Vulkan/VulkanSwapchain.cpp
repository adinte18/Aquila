#include "Aquila/RHI/Vulkan/VulkanSwapchain.h"
#include "Aquila/RHI/DeletionQueue.h"
#include "Aquila/Foundation/Log.h"
#include "Aquila/RHI/Vulkan/VulkanDevice.h"
#include "Aquila/RHI/Vulkan/VulkanDeletionQueue.h"
#include "Aquila/RHI/Vulkan/VulkanFormatUtils.h"

namespace Aquila::RHI {

VulkanSwapchain::VulkanSwapchain(VulkanDevice &device, VkExtent2D extent, bool vsync)
	: m_Device(device), m_WindowExtent(extent), m_VSyncEnabled(vsync) {
	Initialize();
}

VulkanSwapchain::VulkanSwapchain(VulkanDevice &device, VkExtent2D extent, bool vsync, Ref<VulkanSwapchain> previous)
	: m_Device(device), m_WindowExtent(extent), m_VSyncEnabled(vsync), m_OldSwapchain(std::move(previous)) {
	Initialize();
	m_OldSwapchain = nullptr;
}

VulkanSwapchain::~VulkanSwapchain() {
	VkDevice dev = m_Device.GetDevice();

	// Application::~Application calls WaitIdle() (vkDeviceWaitIdle) before destroying the
	// swapchain, so all queues are already drained and all fences are in the signaled state.
	for (auto &pending : m_PendingCmdBufs) {
		for (auto &p : pending) {
			vkFreeCommandBuffers(dev, p.pool, 1, &p.cmd);
		}
		pending.clear();
	}

	for (auto *view : m_ImageViews) {
		if (view != VK_NULL_HANDLE) {
			vkDestroyImageView(dev, view, nullptr);
		}
	}
	m_ImageViews.clear();

	for (auto *view : m_DepthImageViews) {
		if (view != VK_NULL_HANDLE) {
			vkDestroyImageView(dev, view, nullptr);
		}
	}
	m_DepthImageViews.clear();

	for (auto &alloc : m_DepthAllocations) {
		vmaDestroyImage(m_Device.GetAllocator(), alloc.image, alloc.allocation);
	}
	m_DepthAllocations.clear();

	if (m_Swapchain != VK_NULL_HANDLE) {
		vkDestroySwapchainKHR(dev, m_Swapchain, nullptr);
		m_Swapchain = VK_NULL_HANDLE;
	}

	auto &deletionQueue = m_Device.GetDeletionQueue();

	for (auto *sem : m_ImageAvailableSemaphores) {
		if (sem != VK_NULL_HANDLE) {
			deletionQueue.QueueDeletion(sem);
		}
	}
	for (auto *sem : m_RenderFinishedSemaphores) {
		if (sem != VK_NULL_HANDLE) {
			deletionQueue.QueueDeletion(sem);
		}
	}
	for (auto *fence : m_InFlightFences) {
		if (fence != VK_NULL_HANDLE) {
			vkDestroyFence(dev, fence, nullptr);
		}
	}
}

void VulkanSwapchain::Initialize() {
	CreateSwapchain();
	CreateImageViews();
	CreateDepthResources();
	CreateSyncObjects();
}

void VulkanSwapchain::CreateSwapchain(VkSwapchainKHR oldHandle) {
	VkSwapChainSupportDetails support = m_Device.GetSwapChainSupport();

	VkSurfaceFormatKHR surfaceFormat = ChooseSwapSurfaceFormat(support.m_Formats);
	VkPresentModeKHR presentMode = ChooseSwapPresentMode(support.m_PresentModes);
	VkExtent2D extent = ChooseSwapExtent(support.m_SurfaceCapabilities);

	uint32 imageCount = support.m_SurfaceCapabilities.minImageCount + 1;
	if (support.m_SurfaceCapabilities.maxImageCount > 0 && imageCount > support.m_SurfaceCapabilities.maxImageCount) {
		imageCount = support.m_SurfaceCapabilities.maxImageCount;
	}

	VkSwapchainCreateInfoKHR createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	createInfo.surface = m_Device.GetSurface();
	createInfo.minImageCount = imageCount;
	createInfo.imageFormat = surfaceFormat.format;
	createInfo.imageColorSpace = surfaceFormat.colorSpace;
	createInfo.imageExtent = extent;
	createInfo.imageArrayLayers = 1;
	createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;

	VkQueueFamilyIndices indices = m_Device.FindPhysicalQF();
	uint32 queueFamilyIndices[] = { indices.m_GraphicsFamily.value(), indices.m_PresentFamily.value() };

	if (indices.m_GraphicsFamily != indices.m_PresentFamily) {
		createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
		createInfo.queueFamilyIndexCount = 2;
		createInfo.pQueueFamilyIndices = queueFamilyIndices;
	} else {
		createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
	}

	createInfo.preTransform = support.m_SurfaceCapabilities.currentTransform;
	createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	createInfo.presentMode = presentMode;
	createInfo.clipped = VK_TRUE;
	createInfo.oldSwapchain =
		oldHandle != VK_NULL_HANDLE ? oldHandle : (m_OldSwapchain ? m_OldSwapchain->m_Swapchain : VK_NULL_HANDLE);

	AQUILA_VULKAN_CHECK(vkCreateSwapchainKHR(m_Device.GetDevice(), &createInfo, nullptr, &m_Swapchain));

	vkGetSwapchainImagesKHR(m_Device.GetDevice(), m_Swapchain, &imageCount, nullptr);
	m_Images.resize(imageCount);
	vkGetSwapchainImagesKHR(m_Device.GetDevice(), m_Swapchain, &imageCount, m_Images.data());

	m_ImageFormat = surfaceFormat.format;
	m_Extent = extent;
}

void VulkanSwapchain::CreateImageViews() {
	m_ImageInitialized.resize(m_Images.size(), false);
	m_ImageViews.resize(m_Images.size());

	for (size_t i = 0; i < m_Images.size(); i++) {
		VkImageViewCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		createInfo.image = m_Images[i];
		createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
		createInfo.format = m_ImageFormat;
		createInfo.components = { .r = VK_COMPONENT_SWIZZLE_IDENTITY,
								  .g = VK_COMPONENT_SWIZZLE_IDENTITY,
								  .b = VK_COMPONENT_SWIZZLE_IDENTITY,
								  .a = VK_COMPONENT_SWIZZLE_IDENTITY };
		createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		createInfo.subresourceRange.baseMipLevel = 0;
		createInfo.subresourceRange.levelCount = 1;
		createInfo.subresourceRange.baseArrayLayer = 0;
		createInfo.subresourceRange.layerCount = 1;

		AQUILA_VULKAN_CHECK(vkCreateImageView(m_Device.GetDevice(), &createInfo, nullptr, &m_ImageViews[i]));
	}
}

void VulkanSwapchain::CreateDepthResources() {
	m_DepthFormat = FindDepthFormat();

	m_DepthAllocations.resize(m_Images.size());
	m_DepthImageViews.resize(m_Images.size());

	for (size_t i = 0; i < m_Images.size(); i++) {
		m_DepthAllocations[i] = m_Device.CreateImage<MemoryDomain::GPU_ONLY>(
			m_Extent.width, m_Extent.height, m_DepthFormat, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, 1, 1,
			VK_SAMPLE_COUNT_1_BIT, ("SwapchainDepth_" + std::to_string(i)).c_str());

		VkImageViewCreateInfo viewInfo{};
		viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		viewInfo.image = m_DepthAllocations[i].image;
		viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
		viewInfo.format = m_DepthFormat;
		viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
		viewInfo.subresourceRange.baseMipLevel = 0;
		viewInfo.subresourceRange.levelCount = 1;
		viewInfo.subresourceRange.baseArrayLayer = 0;
		viewInfo.subresourceRange.layerCount = 1;

		AQUILA_VULKAN_CHECK(vkCreateImageView(m_Device.GetDevice(), &viewInfo, nullptr, &m_DepthImageViews[i]));
	}
}

void VulkanSwapchain::CreateSyncObjects() {
	m_ImageAvailableSemaphores.resize(SharedConstants::MAX_FRAMES_IN_FLIGHT);
	m_RenderFinishedSemaphores.resize(SharedConstants::MAX_FRAMES_IN_FLIGHT);
	m_InFlightFences.resize(SharedConstants::MAX_FRAMES_IN_FLIGHT);

	VkSemaphoreCreateInfo semInfo{};
	semInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

	// Create fences pre-signaled so AcquireNextImage's first wait returns immediately.
	VkFenceCreateInfo fenceInfo{};
	fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

	for (uint32 i = 0; i < SharedConstants::MAX_FRAMES_IN_FLIGHT; ++i) {
		AQUILA_VULKAN_CHECK(vkCreateSemaphore(m_Device.GetDevice(), &semInfo, nullptr, &m_ImageAvailableSemaphores[i]));
		AQUILA_VULKAN_CHECK(vkCreateSemaphore(m_Device.GetDevice(), &semInfo, nullptr, &m_RenderFinishedSemaphores[i]));
		AQUILA_VULKAN_CHECK(vkCreateFence(m_Device.GetDevice(), &fenceInfo, nullptr, &m_InFlightFences[i]));
	}
}

bool VulkanSwapchain::AcquireNextImage(uint32 &outImageIndex) {
	VkDevice dev = m_Device.GetDevice();

	if (vkGetFenceStatus(dev, m_InFlightFences[m_NextFrameSlot]) == VK_NOT_READY) {
		Aquila::Foundation::LogWarning("AcquireNextImage: fence NOT_READY for slot {}", m_NextFrameSlot);
		return false;
	}

	m_Device.GetDeletionQueue().Flush(m_NextFrameSlot);

	for (auto &p : m_PendingCmdBufs[m_NextFrameSlot]) {
		vkFreeCommandBuffers(dev, p.pool, 1, &p.cmd);
	}
	m_PendingCmdBufs[m_NextFrameSlot].clear();

	m_Device.ResetFrameCommandPool(m_NextFrameSlot);

	m_Device.GetDeletionQueue().SetCurrentSlot(m_NextFrameSlot);

	VkSemaphore sem = m_ImageAvailableSemaphores[m_NextFrameSlot];
	VkResult result = vkAcquireNextImageKHR(dev, m_Swapchain, 0, sem, VK_NULL_HANDLE, &outImageIndex);

	if (result == VK_NOT_READY) {
		Aquila::Foundation::LogWarning("AcquireNextImage: image acquisition NOT_READY for slot {}", m_NextFrameSlot);
		return false;
	}
	if (result == VK_ERROR_OUT_OF_DATE_KHR) {
		m_NeedsResize = true;
		return false;
	}
	if (result == VK_SUBOPTIMAL_KHR) {
		m_NeedsResize = true;
	}
	if (result == VK_SUCCESS || result == VK_SUBOPTIMAL_KHR) {
		vkResetFences(dev, 1, &m_InFlightFences[m_NextFrameSlot]);
		m_NextFrameSlot = (m_NextFrameSlot + 1) % SharedConstants::MAX_FRAMES_IN_FLIGHT;
		return true;
	}
	return false;
}

TextureFormat VulkanSwapchain::GetFormat() const {
	return FromVkFormat(m_ImageFormat);
}

VkResult VulkanSwapchain::PresentImageRaw(const uint32 *imageIndex, VkSemaphore renderFinishedSemaphore) {
	VkPresentInfoKHR presentInfo{};
	presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	presentInfo.waitSemaphoreCount = 1;
	presentInfo.pWaitSemaphores = &renderFinishedSemaphore;
	presentInfo.swapchainCount = 1;
	presentInfo.pSwapchains = &m_Swapchain;
	presentInfo.pImageIndices = imageIndex;

	VkResult result = vkQueuePresentKHR(m_Device.GetPresentQueue(), &presentInfo);
	if (result == VK_SUBOPTIMAL_KHR || result == VK_ERROR_OUT_OF_DATE_KHR) {
		m_NeedsResize = true;
	}
	return result;
}

void VulkanSwapchain::DestroyImageResources() {
	for (uint32 i = 0; i < SharedConstants::MAX_FRAMES_IN_FLIGHT; ++i) {
		m_Device.GetDeletionQueue().Flush(i);
	}

	VkDevice dev = m_Device.GetDevice();

	for (auto &pending : m_PendingCmdBufs) {
		for (auto &p : pending) {
			vkFreeCommandBuffers(dev, p.pool, 1, &p.cmd);
		}
		pending.clear();
	}

	for (auto *view : m_ImageViews) {
		if (view != VK_NULL_HANDLE) {
			vkDestroyImageView(dev, view, nullptr);
		}
	}
	m_ImageViews.clear();

	for (auto *view : m_DepthImageViews) {
		if (view != VK_NULL_HANDLE) {
			vkDestroyImageView(dev, view, nullptr);
		}
	}
	m_DepthImageViews.clear();

	for (auto &alloc : m_DepthAllocations) {
		vmaDestroyImage(m_Device.GetAllocator(), alloc.image, alloc.allocation);
	}
	m_DepthAllocations.clear();

	m_ImageInitialized.clear();
}

void VulkanSwapchain::DeferCmdBufFree(uint32 frameIndex, VkCommandBuffer cmd, VkCommandPool pool) {
	m_PendingCmdBufs[frameIndex].push_back({ cmd, pool });
}

void VulkanSwapchain::Resize(uint32 width, uint32 height) {
	m_WindowExtent = { .width = width, .height = height };

	DestroyImageResources();

	VkSwapchainKHR prevHandle = m_Swapchain;
	m_Swapchain = VK_NULL_HANDLE;
	CreateSwapchain(prevHandle);
	vkDestroySwapchainKHR(m_Device.GetDevice(), prevHandle, nullptr);

	CreateImageViews();
	CreateDepthResources();

	m_NeedsResize = false;
	m_NextFrameSlot = 0;
}

VkFormat VulkanSwapchain::FindDepthFormat() {
	return m_Device.FindSupportedFormat(
		{ VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT }, VK_IMAGE_TILING_OPTIMAL,
		VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);
}

VkSurfaceFormatKHR VulkanSwapchain::ChooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR> &availableFormats) {
	for (const auto &format : availableFormats) {
		if (format.format == VK_FORMAT_R8G8B8A8_UNORM && format.colorSpace == VK_COLOR_SPACE_EXTENDED_SRGB_LINEAR_EXT) {
			return format;
		}
	}
	return availableFormats[0];
}

VkPresentModeKHR VulkanSwapchain::ChooseSwapPresentMode(const std::vector<VkPresentModeKHR> &availablePresentModes) {
	if (m_VSyncEnabled) {
		Aquila::Foundation::LogInfo("VulkanSwapchain: present_mode=FIFO (vsync=true)");
		return VK_PRESENT_MODE_FIFO_KHR;
	}
	for (const auto &mode : availablePresentModes) {
		if (mode == VK_PRESENT_MODE_MAILBOX_KHR) {
			Aquila::Foundation::LogInfo("VulkanSwapchain: present_mode=MAILBOX (vsync=false)");
			return mode;
		}
	}
	for (const auto &mode : availablePresentModes) {
		if (mode == VK_PRESENT_MODE_IMMEDIATE_KHR) {
			Aquila::Foundation::LogInfo("VulkanSwapchain: present_mode=IMMEDIATE (vsync=false, MAILBOX unavailable)");
			return mode;
		}
	}
	Aquila::Foundation::LogWarning(
		"VulkanSwapchain: present_mode=FIFO (vsync=false requested but MAILBOX/IMMEDIATE unavailable — vsync active)");
	return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D VulkanSwapchain::ChooseSwapExtent(const VkSurfaceCapabilitiesKHR &capabilities) const {
	if (capabilities.currentExtent.width != std::numeric_limits<uint32>::max()) {
		return capabilities.currentExtent;
	}
	VkExtent2D actual = m_WindowExtent;
	actual.width = std::clamp(actual.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
	actual.height = std::clamp(actual.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);
	return actual;
}

} // namespace Aquila::RHI
