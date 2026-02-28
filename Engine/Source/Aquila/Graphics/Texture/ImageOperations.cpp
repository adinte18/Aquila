#include "Aquila/Graphics/Texture/ImageOperations.h"
#include "Aquila/Graphics/Core/Device.h"

namespace Aquila::Graphics::Texture {

void ImageOperations::CopyImage(Device &device, VkImage srcImage, VkImage dstImage, uint32 width, uint32 height,
								VkImageAspectFlags aspectMask) {
	device.ExecuteGraphicsCommands([&](VkCommandBuffer cmd) {
		VkImageCopy copyRegion{};
		copyRegion.srcSubresource = { .aspectMask = aspectMask, .mipLevel = 0, .baseArrayLayer = 0, .layerCount = 1 };
		copyRegion.dstSubresource = { .aspectMask = aspectMask, .mipLevel = 0, .baseArrayLayer = 0, .layerCount = 1 };
		copyRegion.srcOffset = { .x = 0, .y = 0, .z = 0 };
		copyRegion.dstOffset = { .x = 0, .y = 0, .z = 0 };
		copyRegion.extent = { .width = width, .height = height, .depth = 1 };
		vkCmdCopyImage(cmd, srcImage, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, dstImage,
					   VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copyRegion);
	});
}

void ImageOperations::CopyImageWithTransitions(Device &device, VkImage srcImage, VkImage dstImage, uint32 width,
											   uint32 height, VkFormat format, VkImageLayout srcInitialLayout,
											   VkImageLayout dstInitialLayout, VkImageLayout srcFinalLayout,
											   VkImageLayout dstFinalLayout) {
	device.ExecuteGraphicsCommands([&](VkCommandBuffer cmd) {
		VkImageAspectFlags aspectMask =
			(format == VK_FORMAT_D32_SFLOAT || format == VK_FORMAT_D24_UNORM_S8_UINT || format == VK_FORMAT_D16_UNORM)
				? VK_IMAGE_ASPECT_DEPTH_BIT
				: VK_IMAGE_ASPECT_COLOR_BIT;

		VkImageMemoryBarrier srcBarrier1{};
		srcBarrier1.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		srcBarrier1.oldLayout = srcInitialLayout;
		srcBarrier1.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
		srcBarrier1.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		srcBarrier1.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		srcBarrier1.image = srcImage;
		srcBarrier1.subresourceRange = { .aspectMask = aspectMask,
										 .baseMipLevel = 0,
										 .levelCount = 1,
										 .baseArrayLayer = 0,
										 .layerCount = 1 };
		auto srcInfo1 = GetBarrierInfo(srcInitialLayout, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
		srcBarrier1.srcAccessMask = srcInfo1.srcAccess;
		srcBarrier1.dstAccessMask = srcInfo1.dstAccess;

		VkImageMemoryBarrier dstBarrier1{};
		dstBarrier1.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		dstBarrier1.oldLayout = dstInitialLayout;
		dstBarrier1.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		dstBarrier1.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		dstBarrier1.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		dstBarrier1.image = dstImage;
		dstBarrier1.subresourceRange = { .aspectMask = aspectMask,
										 .baseMipLevel = 0,
										 .levelCount = 1,
										 .baseArrayLayer = 0,
										 .layerCount = 1 };
		auto dstInfo1 = GetBarrierInfo(dstInitialLayout, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
		dstBarrier1.srcAccessMask = dstInfo1.srcAccess;
		dstBarrier1.dstAccessMask = dstInfo1.dstAccess;

		VkImageMemoryBarrier preBarriers[] = { srcBarrier1, dstBarrier1 };
		vkCmdPipelineBarrier(cmd, srcInfo1.srcStage | dstInfo1.srcStage, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr,
							 0, nullptr, 2, preBarriers);

		VkImageCopy copyRegion{};
		copyRegion.srcSubresource = { .aspectMask = aspectMask, .mipLevel = 0, .baseArrayLayer = 0, .layerCount = 1 };
		copyRegion.dstSubresource = { .aspectMask = aspectMask, .mipLevel = 0, .baseArrayLayer = 0, .layerCount = 1 };
		copyRegion.srcOffset = { .x = 0, .y = 0, .z = 0 };
		copyRegion.dstOffset = { .x = 0, .y = 0, .z = 0 };
		copyRegion.extent = { .width = width, .height = height, .depth = 1 };
		vkCmdCopyImage(cmd, srcImage, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, dstImage,
					   VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copyRegion);

		VkImageMemoryBarrier srcBarrier2{};
		srcBarrier2.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		srcBarrier2.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
		srcBarrier2.newLayout = srcFinalLayout;
		srcBarrier2.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		srcBarrier2.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		srcBarrier2.image = srcImage;
		srcBarrier2.subresourceRange = { .aspectMask = aspectMask,
										 .baseMipLevel = 0,
										 .levelCount = 1,
										 .baseArrayLayer = 0,
										 .layerCount = 1 };
		auto srcInfo2 = GetBarrierInfo(VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, srcFinalLayout);
		srcBarrier2.srcAccessMask = srcInfo2.srcAccess;
		srcBarrier2.dstAccessMask = srcInfo2.dstAccess;

		VkImageMemoryBarrier dstBarrier2{};
		dstBarrier2.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		dstBarrier2.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		dstBarrier2.newLayout = dstFinalLayout;
		dstBarrier2.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		dstBarrier2.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		dstBarrier2.image = dstImage;
		dstBarrier2.subresourceRange = { .aspectMask = aspectMask,
										 .baseMipLevel = 0,
										 .levelCount = 1,
										 .baseArrayLayer = 0,
										 .layerCount = 1 };
		auto dstInfo2 = GetBarrierInfo(VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, dstFinalLayout);
		dstBarrier2.srcAccessMask = dstInfo2.srcAccess;
		dstBarrier2.dstAccessMask = dstInfo2.dstAccess;

		VkImageMemoryBarrier postBarriers[] = { srcBarrier2, dstBarrier2 };
		vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TRANSFER_BIT, srcInfo2.dstStage | dstInfo2.dstStage, 0, 0, nullptr,
							 0, nullptr, 2, postBarriers);
	});
}

void ImageOperations::TransitionLayout(Device &device, VkImage image, VkFormat format, VkImageLayout oldLayout,
									   VkImageLayout newLayout, uint32 mipLevels, uint32 layers) {
	device.ExecuteGraphicsCommands([&](VkCommandBuffer cmd) {
		TransitionLayout(device, cmd, image, format, oldLayout, newLayout, mipLevels, layers);
	});
}

void ImageOperations::TransitionLayout(Device &device, VkCommandBuffer commandBuffer, VkImage image, VkFormat format,
									   VkImageLayout oldLayout, VkImageLayout newLayout, uint32 mipLevels,
									   uint32 layers) {
	BarrierInfo barrier = GetBarrierInfo(oldLayout, newLayout);

	VkImageMemoryBarrier imageBarrier{};
	imageBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	imageBarrier.oldLayout = oldLayout;
	imageBarrier.newLayout = newLayout;
	imageBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	imageBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	imageBarrier.image = image;
	imageBarrier.subresourceRange.aspectMask =
		(format == VK_FORMAT_D32_SFLOAT) ? VK_IMAGE_ASPECT_DEPTH_BIT : VK_IMAGE_ASPECT_COLOR_BIT;
	imageBarrier.subresourceRange.baseMipLevel = 0;
	imageBarrier.subresourceRange.levelCount = mipLevels;
	imageBarrier.subresourceRange.baseArrayLayer = 0;
	imageBarrier.subresourceRange.layerCount = layers;
	imageBarrier.srcAccessMask = barrier.srcAccess;
	imageBarrier.dstAccessMask = barrier.dstAccess;

	vkCmdPipelineBarrier(commandBuffer, barrier.srcStage, barrier.dstStage, 0, 0, nullptr, 0, nullptr, 1,
						 &imageBarrier);
}

void ImageOperations::GenerateMipmaps(Device &device, VkImage image, VkFormat format, int32 width, int32 height,
									  uint32 mipLevels) {
	ValidateFormatSupport(device, format);

	device.ExecuteGraphicsCommands([&](VkCommandBuffer cmd) {
		GenerateMipmapsWithCmd(device, cmd, image, format, static_cast<uint32>(width), static_cast<uint32>(height),
							   mipLevels);
	});
}
void ImageOperations::GenerateMipmapsWithCmd(Device &device, VkCommandBuffer cmd, VkImage image, VkFormat format,
											 uint32 width, uint32 height, uint32 mipLevels) {
	VkFormatProperties formatProperties;
	vkGetPhysicalDeviceFormatProperties(device.GetPhysicalDevice(), format, &formatProperties);

	if ((formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT) == 0U) {
		throw std::runtime_error("Texture image format does not support linear blitting!");
	}

	VkImageMemoryBarrier barrier{};
	barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	barrier.image = image;
	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	barrier.subresourceRange.baseArrayLayer = 0;
	barrier.subresourceRange.layerCount = 1;
	barrier.subresourceRange.levelCount = 1;

	int mipWidth = static_cast<int>(width);
	int mipHeight = static_cast<int>(height);

	for (uint32 i = 1; i < mipLevels; i++) {
		// Transition previous mip level to TRANSFER_SRC_OPTIMAL
		barrier.subresourceRange.baseMipLevel = i - 1;
		barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
		barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

		vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0,
							 nullptr, 1, &barrier);

		// Blit from previous mip level to current
		VkImageBlit blit{};
		blit.srcOffsets[0] = { .x = 0, .y = 0, .z = 0 };
		blit.srcOffsets[1] = { .x = mipWidth, .y = mipHeight, .z = 1 };
		blit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		blit.srcSubresource.mipLevel = i - 1;
		blit.srcSubresource.baseArrayLayer = 0;
		blit.srcSubresource.layerCount = 1;
		blit.dstOffsets[0] = { .x = 0, .y = 0, .z = 0 };
		blit.dstOffsets[1] = { .x = mipWidth > 1 ? mipWidth / 2 : 1, .y = mipHeight > 1 ? mipHeight / 2 : 1, .z = 1 };
		blit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		blit.dstSubresource.mipLevel = i;
		blit.dstSubresource.baseArrayLayer = 0;
		blit.dstSubresource.layerCount = 1;

		vkCmdBlitImage(cmd, image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1,
					   &blit, VK_FILTER_LINEAR);

		// Transition previous mip level to SHADER_READ_ONLY_OPTIMAL
		barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
		barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
		barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

		vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr,
							 0, nullptr, 1, &barrier);

		if (mipWidth > 1) {
			mipWidth /= 2;
		}
		if (mipHeight > 1) {
			mipHeight /= 2;
		}
	}

	// Transition the last mip level to SHADER_READ_ONLY_OPTIMAL
	barrier.subresourceRange.baseMipLevel = mipLevels - 1;
	barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
	barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
	barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

	vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr, 0,
						 nullptr, 1, &barrier);
}

uint32 ImageOperations::CalculateMipLevels(uint32 width, uint32 height) {
	return static_cast<uint32>(std::floor(std::log2(std::max(width, height)))) + 1;
}

VkImageView ImageOperations::CreateImageView(Device &device, VkImage image, VkFormat format, VkImageViewType viewType,
											 uint32 layerCount, uint32 mipLevels) {
	VkImageViewCreateInfo viewInfo{};
	viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	viewInfo.image = image;
	viewInfo.viewType = viewType;
	viewInfo.format = format;
	viewInfo.subresourceRange.aspectMask =
		(format == VK_FORMAT_D32_SFLOAT) ? VK_IMAGE_ASPECT_DEPTH_BIT : VK_IMAGE_ASPECT_COLOR_BIT;
	viewInfo.subresourceRange.baseMipLevel = 0;
	viewInfo.subresourceRange.levelCount = mipLevels;
	viewInfo.subresourceRange.baseArrayLayer = 0;
	viewInfo.subresourceRange.layerCount = layerCount;

	VkImageView imageView;
	AQUILA_VULKAN_CHECK(vkCreateImageView(device.GetDevice(), &viewInfo, nullptr, &imageView));
	return imageView;
}

VkSampler ImageOperations::CreateSampler(Device &device, uint32 mipLevels, bool mipmapped) {
	VkPhysicalDeviceProperties properties{};
	vkGetPhysicalDeviceProperties(device.GetPhysicalDevice(), &properties);

	VkSamplerCreateInfo samplerInfo{};
	samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	samplerInfo.magFilter = VK_FILTER_LINEAR;
	samplerInfo.minFilter = VK_FILTER_LINEAR;
	samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	samplerInfo.anisotropyEnable = VK_TRUE;
	samplerInfo.maxAnisotropy = properties.limits.maxSamplerAnisotropy;
	samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
	samplerInfo.unnormalizedCoordinates = VK_FALSE;
	samplerInfo.compareEnable = VK_FALSE;
	samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
	samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
	samplerInfo.minLod = 0.0f;
	samplerInfo.maxLod = mipmapped ? static_cast<f32>(mipLevels) : 0.0f;
	samplerInfo.mipLodBias = 0.0f;

	VkSampler sampler;
	AQUILA_VULKAN_CHECK(vkCreateSampler(device.GetDevice(), &samplerInfo, nullptr, &sampler));
	return sampler;
}
VkSampler ImageOperations::CreateMipmappedCubemapSampler(Device &device, uint32 mipLevels) {
	VkPhysicalDeviceProperties properties{};
	vkGetPhysicalDeviceProperties(device.GetPhysicalDevice(), &properties);

	VkSamplerCreateInfo samplerInfo{};
	samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	samplerInfo.magFilter = VK_FILTER_LINEAR;
	samplerInfo.minFilter = VK_FILTER_LINEAR;
	samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	samplerInfo.anisotropyEnable = VK_FALSE;
	samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
	samplerInfo.unnormalizedCoordinates = VK_FALSE;
	samplerInfo.compareEnable = VK_FALSE;
	samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
	samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
	samplerInfo.minLod = 0.0f;
	samplerInfo.maxLod = static_cast<f32>(mipLevels);
	samplerInfo.mipLodBias = 0.0f;

	VkSampler sampler = nullptr;
	AQUILA_VULKAN_CHECK(vkCreateSampler(device.GetDevice(), &samplerInfo, nullptr, &sampler));
	return sampler;
}

// Private methods

ImageOperations::BarrierInfo ImageOperations::GetBarrierInfo(VkImageLayout oldLayout, VkImageLayout newLayout) {
	BarrierInfo info{};

	// Determine source stage and access
	switch (oldLayout) {
	case VK_IMAGE_LAYOUT_UNDEFINED:
		info.srcAccess = 0;
		info.srcStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		break;
	case VK_IMAGE_LAYOUT_PREINITIALIZED:
		info.srcAccess = VK_ACCESS_HOST_WRITE_BIT;
		info.srcStage = VK_PIPELINE_STAGE_HOST_BIT;
		break;
	case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
		info.srcAccess = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		info.srcStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		break;
	case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
		info.srcAccess = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
		info.srcStage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
		break;
	case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
		info.srcAccess = VK_ACCESS_TRANSFER_READ_BIT;
		info.srcStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
		break;
	case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
		info.srcAccess = VK_ACCESS_TRANSFER_WRITE_BIT;
		info.srcStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
		break;
	case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
		info.srcAccess = VK_ACCESS_SHADER_READ_BIT;
		info.srcStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
		break;
	case VK_IMAGE_LAYOUT_PRESENT_SRC_KHR:
		info.srcAccess = 0;
		info.srcStage = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
		break;

	default:
		throw std::invalid_argument("Unsupported old layout transition!");
	}

	// Determine destination stage and access
	switch (newLayout) {
	case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
		info.dstAccess = VK_ACCESS_TRANSFER_WRITE_BIT;
		info.dstStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
		break;
	case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
		info.dstAccess = VK_ACCESS_TRANSFER_READ_BIT;
		info.dstStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
		break;
	case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
		info.dstAccess = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		info.dstStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		break;
	case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
		info.dstAccess = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
		info.dstStage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
		break;
	case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
		info.dstAccess = VK_ACCESS_SHADER_READ_BIT;
		info.dstStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
		break;
	case VK_IMAGE_LAYOUT_PRESENT_SRC_KHR:
		info.dstAccess = 0;
		info.dstStage = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
		break;
	default:
		throw std::invalid_argument("Unsupported new layout transition!");
	}

	return info;
}

void ImageOperations::ValidateFormatSupport(Device &device, VkFormat format) {
	VkFormatProperties formatProperties;
	vkGetPhysicalDeviceFormatProperties(device.GetPhysicalDevice(), format, &formatProperties);

	if ((formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT) == 0U) {
		throw std::runtime_error("Texture image format does not support linear blitting!");
	}
}

} // namespace Aquila::Graphics::Texture
