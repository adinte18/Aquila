#include "Aquila/RHI/Vulkan/VulkanTexture.h"
#include "Aquila/RHI/Vulkan/VulkanDeletionQueue.h"

#include "Aquila/RHI/Vulkan/VulkanDevice.h"
#include "Aquila/RHI/Vulkan/VulkanFormatUtils.h"

namespace Aquila::RHI {

VulkanTexture::VulkanTexture(VulkanDevice &device, const TextureDesc &desc) : m_Device(device), m_Desc(desc) {
	m_ImageAllocation = m_Device.CreateImage<MemoryDomain::GPU_ONLY>(
		desc.width, desc.height, ToVkFormat(desc.format), ToVkImageUsage(desc.usage), desc.mipLevels, desc.arrayLayers,
		ToVkSampleCount(desc.samples), desc.debugName.c_str());

	CreateImageView();
	CreateSampler();

	m_Device.SetObjectDebugName(VK_OBJECT_TYPE_IMAGE, reinterpret_cast<uint64>(m_ImageAllocation.image),
								(desc.debugName + "_Image").c_str());
	m_Device.SetObjectDebugName(VK_OBJECT_TYPE_IMAGE_VIEW, reinterpret_cast<uint64>(m_ImageView),
								(desc.debugName + "_ImageView").c_str());
	m_Device.SetObjectDebugName(VK_OBJECT_TYPE_SAMPLER, reinterpret_cast<uint64>(m_Sampler),
								(desc.debugName + "_Sampler").c_str());
}

VulkanTexture::~VulkanTexture() {
	auto &queue = m_Device.GetDeletionQueue();

	if (m_ImageView != VK_NULL_HANDLE) {
		queue.QueueDeletion(m_ImageView);
	}

	if (m_ImageAllocation.image != VK_NULL_HANDLE) {
		queue.QueueDeletion(
			Deletion::VmaImageDeletion{ .image = m_ImageAllocation.image, .allocation = m_ImageAllocation.allocation });
	}
}

void VulkanTexture::DestroyImmediate() {
	if (m_ImageView != VK_NULL_HANDLE) {
		vkDestroyImageView(m_Device.GetDevice(), m_ImageView, nullptr);
		m_ImageView = VK_NULL_HANDLE;
	}
	vmaDestroyImage(m_Device.GetAllocator(), m_ImageAllocation.image, m_ImageAllocation.allocation);
	m_ImageAllocation.image = VK_NULL_HANDLE;
	m_ImageAllocation.allocation = VK_NULL_HANDLE;
}

VkDescriptorImageInfo VulkanTexture::GetDescriptorImageInfo() const {
	VkDescriptorImageInfo info{};
	info.imageLayout = IsDepthFormat(m_Desc.format) ? VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL
													: VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	info.imageView = m_ImageView;
	info.sampler = m_Sampler;
	return info;
}

void VulkanTexture::CreateImageView() {
	bool isDepth = IsDepthFormat(m_Desc.format);

	VkImageViewCreateInfo viewInfo{};
	viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	viewInfo.image = m_ImageAllocation.image;
	viewInfo.viewType = ToVkImageViewType(m_Desc.viewType);
	viewInfo.format = ToVkFormat(m_Desc.format);
	viewInfo.components = ToVkComponentMapping(m_Desc.swizzle);
	viewInfo.subresourceRange.aspectMask = isDepth ? VK_IMAGE_ASPECT_DEPTH_BIT : VK_IMAGE_ASPECT_COLOR_BIT;
	viewInfo.subresourceRange.baseMipLevel = 0;
	viewInfo.subresourceRange.levelCount = m_Desc.mipLevels;
	viewInfo.subresourceRange.baseArrayLayer = 0;
	viewInfo.subresourceRange.layerCount = m_Desc.arrayLayers;

	AQUILA_VULKAN_CHECK(vkCreateImageView(m_Device.GetDevice(), &viewInfo, nullptr, &m_ImageView));
}

void VulkanTexture::CreateSampler() {
	m_Sampler = m_Device.GetOrCreateSampler(m_Desc.sampler);
}

} // namespace Aquila::RHI
