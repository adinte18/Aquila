#include "Aquila/Graphics/Core/Swapchain.h"

namespace Aquila::Graphics {

Swapchain::Swapchain(Device &device, VkExtent2D extent) : device{ device }, windowExtent{ extent } {
	Initialize();
}

Swapchain::Swapchain(Device &device, VkExtent2D extent, Ref<Swapchain> previous)
	: device{ device }, windowExtent{ extent }, oldSC{ std::move(previous) } {
	Initialize();
	oldSC = nullptr;
}

void Swapchain::Initialize() {
	CreateSwapchain();
	CreateImageViews();
	CreateDepthResources();
}

Swapchain::~Swapchain() {
	for (auto *imageView : swapChainImageViews) {
		if (imageView != VK_NULL_HANDLE) {
			device.GetDeletionManager().QueueDeletion(imageView);
		}
	}
	swapChainImageViews.clear();

	if (swapChain != nullptr) {
		vkDestroySwapchainKHR(device.GetDevice(), swapChain, nullptr);
		swapChain = nullptr;
	}

	m_DepthTargets.clear();
}

void Swapchain::CreateSwapchain() {
	VkSwapChainSupportDetails swapChainSupport = device.GetSwapChainSupport();

	VkSurfaceFormatKHR surfaceFormat = ChooseSwapSurfaceFormat(swapChainSupport.m_Formats);
	VkPresentModeKHR presentMode = ChooseSwapPresentMode(swapChainSupport.m_PresentModes);
	VkExtent2D extent = ChooseSwapExtent(swapChainSupport.m_SurfaceCapabilities);

	uint32 imageCount = swapChainSupport.m_SurfaceCapabilities.minImageCount + 1;
	if (swapChainSupport.m_SurfaceCapabilities.maxImageCount > 0 &&
		imageCount > swapChainSupport.m_SurfaceCapabilities.maxImageCount) {
		imageCount = swapChainSupport.m_SurfaceCapabilities.maxImageCount;
	}

	VkSwapchainCreateInfoKHR createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	createInfo.surface = device.GetSurface();
	createInfo.minImageCount = imageCount;
	createInfo.imageFormat = surfaceFormat.format;
	createInfo.imageColorSpace = surfaceFormat.colorSpace;
	createInfo.imageExtent = extent;
	createInfo.imageArrayLayers = 1;
	createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

	VkQueueFamilyIndices indices = device.FindPhysicalQF();
	uint32 queueFamilyIndices[] = { indices.m_GraphicsFamily.value(), indices.m_PresentFamily.value() };

	if (indices.m_GraphicsFamily != indices.m_PresentFamily) {
		createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
		createInfo.queueFamilyIndexCount = 2;
		createInfo.pQueueFamilyIndices = static_cast<const uint32 *>(queueFamilyIndices);
	} else {
		createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
	}

	createInfo.preTransform = swapChainSupport.m_SurfaceCapabilities.currentTransform;
	createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	createInfo.presentMode = presentMode;
	createInfo.clipped = VK_TRUE;
	createInfo.oldSwapchain = oldSC == nullptr ? VK_NULL_HANDLE : oldSC->swapChain;

	AQUILA_VULKAN_CHECK(vkCreateSwapchainKHR(device.GetDevice(), &createInfo, nullptr, &swapChain));

	vkGetSwapchainImagesKHR(device.GetDevice(), swapChain, &imageCount, nullptr);
	swapChainImages.resize(imageCount);
	vkGetSwapchainImagesKHR(device.GetDevice(), swapChain, &imageCount, swapChainImages.data());

	swapChainImageFormat = surfaceFormat.format;
	swapChainExtent = extent;
}

void Swapchain::CreateImageViews() {
	m_ImageInitialized.resize(swapChainImages.size(), false);
	swapChainImageViews.resize(swapChainImages.size());

	for (size_t i = 0; i < swapChainImages.size(); i++) {
		VkImageViewCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		createInfo.image = swapChainImages[i];
		createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
		createInfo.format = swapChainImageFormat;
		createInfo.components = { .r = VK_COMPONENT_SWIZZLE_IDENTITY,
								  .g = VK_COMPONENT_SWIZZLE_IDENTITY,
								  .b = VK_COMPONENT_SWIZZLE_IDENTITY,
								  .a = VK_COMPONENT_SWIZZLE_IDENTITY };
		createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		createInfo.subresourceRange.levelCount = 1;
		createInfo.subresourceRange.layerCount = 1;

		AQUILA_VULKAN_CHECK(vkCreateImageView(device.GetDevice(), &createInfo, nullptr, &swapChainImageViews[i]));
	}
}

void Swapchain::CreateDepthResources() {
	VkFormat depthFormat = FindDepthFormat();
	swapChainDepthFormat = depthFormat;

	m_DepthTargets.resize(GetImageCount());

	for (size_t i = 0; i < m_DepthTargets.size(); i++) {
		RenderingPipeline::RenderTarget::RenderTargetSpecification spec{};
		spec.width = swapChainExtent.width;
		spec.height = swapChainExtent.height;
		spec.depthFormat = depthFormat;
		spec.type = RenderingPipeline::RenderTarget::Type::DEPTH_ONLY;
		spec.usage = RenderingPipeline::RenderTarget::Usage::SCENE_VIEW;
		spec.debugName = "SwapchainDepth_" + std::to_string(i);

		m_DepthTargets[i] = CreateRef<RenderingPipeline::RenderTarget>(device, spec);
	}
}

VkFormat Swapchain::FindDepthFormat() {
	return device.FindSupportedFormat(
		{ VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT }, VK_IMAGE_TILING_OPTIMAL,
		VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);
}

VkSurfaceFormatKHR Swapchain::ChooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR> &availableFormats) {
	for (const auto &availableFormat : availableFormats) {
		if (availableFormat.format == VK_FORMAT_R8G8B8A8_UNORM &&
			availableFormat.colorSpace == VK_COLOR_SPACE_EXTENDED_SRGB_LINEAR_EXT) {
			return availableFormat;
		}
	}
	return availableFormats[0];
}

VkPresentModeKHR Swapchain::ChooseSwapPresentMode(const std::vector<VkPresentModeKHR> &availablePresentModes) {
	for (const auto &availablePresentMode : availablePresentModes) {
		if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
			return availablePresentMode;
		}
	}
	return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D Swapchain::ChooseSwapExtent(const VkSurfaceCapabilitiesKHR &capabilities) const {
	if (capabilities.currentExtent.width != std::numeric_limits<uint32>::max()) {
		return capabilities.currentExtent;
	}
	VkExtent2D actualExtent = windowExtent;
	actualExtent.width =
		std::max(capabilities.minImageExtent.width, std::min(capabilities.maxImageExtent.width, actualExtent.width));
	actualExtent.height =
		std::max(capabilities.minImageExtent.height, std::min(capabilities.maxImageExtent.height, actualExtent.height));
	return actualExtent;
}

VkResult Swapchain::AcquireNextImage(uint32 *imageIndex, VkSemaphore imageAvailableSemaphore) const {
	return vkAcquireNextImageKHR(device.GetDevice(), swapChain, std::numeric_limits<uint64_t>::max(),
								 imageAvailableSemaphore, VK_NULL_HANDLE, imageIndex);
}

VkResult Swapchain::PresentImage(const uint32 *imageIndex, VkSemaphore renderFinishedSemaphore) const {
	VkPresentInfoKHR presentInfo{};
	presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	presentInfo.waitSemaphoreCount = 1;
	presentInfo.pWaitSemaphores = &renderFinishedSemaphore;

	VkSwapchainKHR swapChains[] = { swapChain };
	presentInfo.swapchainCount = 1;
	presentInfo.pSwapchains = static_cast<const VkSwapchainKHR *>(swapChains);
	presentInfo.pImageIndices = imageIndex;

	return vkQueuePresentKHR(device.GetPresentQueue(), &presentInfo);
}

} // namespace Aquila::Graphics
