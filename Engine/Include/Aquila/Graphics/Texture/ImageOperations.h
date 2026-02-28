#ifndef AQUILA_IMAGEOPERATIONS_H
#define AQUILA_IMAGEOPERATIONS_H

#include "Aquila/Graphics/Core/Device.h"

namespace Aquila::Graphics::Texture {

class ImageOperations {
  public:
	static uint32 CalculateMipLevels(uint32 width, uint32 height);

	static void CopyImage(Device &device, VkImage srcImage, VkImage dstImage, uint32 width, uint32 height,
						  VkImageAspectFlags aspectMask);
	static void CopyImageWithTransitions(Device &device, VkImage srcImage, VkImage dstImage, uint32 width,
										 uint32 height, VkFormat format, VkImageLayout srcInitialLayout,
										 VkImageLayout dstInitialLayout, VkImageLayout srcFinalLayout,
										 VkImageLayout dstFinalLayout);
	static void TransitionLayout(Device &device, VkImage image, VkFormat format, VkImageLayout oldLayout,
								 VkImageLayout newLayout, uint32 mipLevels, uint32 layers = 1);
	static void TransitionLayout(Device &device, VkCommandBuffer commandBuffer, VkImage image, VkFormat format,
								 VkImageLayout oldLayout, VkImageLayout newLayout, uint32 mipLevels, uint32 layers = 1);
	static void GenerateMipmaps(Device &device, VkImage image, VkFormat format, int32 width, int32 height,
								uint32 mipLevels);
	static void GenerateMipmapsWithCmd(Device &device, VkCommandBuffer cmd, VkImage image, VkFormat format,
									   uint32 width, uint32 height, uint32 mipLevels);
	static VkImageView CreateImageView(Device &device, VkImage image, VkFormat format, VkImageViewType viewType,
									   uint32 layerCount, uint32 mipLevels);
	static VkSampler CreateSampler(Device &device, uint32 mipLevels, bool mipmapped);
	static VkSampler CreateMipmappedCubemapSampler(Device &device, uint32 mipLevels);

  private:
	struct BarrierInfo {
		VkAccessFlags srcAccess;
		VkAccessFlags dstAccess;
		VkPipelineStageFlags srcStage;
		VkPipelineStageFlags dstStage;
	};

	static BarrierInfo GetBarrierInfo(VkImageLayout oldLayout, VkImageLayout newLayout);
	static void ValidateFormatSupport(Device &device, VkFormat format);
};

} // namespace Aquila::Graphics::Texture

#endif
