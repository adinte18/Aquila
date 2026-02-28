#ifndef AQUILA_TEXTURELOADER_H
#define AQUILA_TEXTURELOADER_H

#include "Aquila/Graphics/Core/Device.h"

namespace Aquila::Graphics::Resources {
class Buffer;
}

namespace Aquila::Graphics::Texture {

class TextureLoader {
  public:
	struct ImageData {
		Unique<f32[]> pixels;
		uint32 width;
		uint32 height;
		VkFormat format;
		VkImage image;
		VkDeviceMemory memory;
		uint32 mipLevels;
	};

	struct RawImageData {
		Unique<uint8[]> pixels;
		uint32 width;
		uint32 height;
		uint32 channels;
	};

	struct RawHDRData {
		Unique<f32[]> pixels;
		uint32 width;
		uint32 height;
		uint32 channels;
	};

	explicit TextureLoader(Device &device);

	// Create solid color (for fallbacks)
	ImageData CreateSolidColor(vec4 color, bool isHDR = false);

	// Create empty image
	ImageData CreateImage(uint32 width, uint32 height, VkFormat format, VkImageUsageFlags usage,
						  VkSampleCountFlagBits samples = VK_SAMPLE_COUNT_1_BIT, uint32 mipLevels = 1);

	// Create cubemap image
	ImageData CreateCubemap(uint32 width, uint32 height, VkFormat format, VkImageUsageFlags usage,
							uint32 mipLevels = 1);

	RawImageData LoadFromFile(const std::string &filepath);
	RawImageData LoadFromVFS(const std::string &filepath);
	RawHDRData LoadHDRFromFile(const std::string &filepath);

  private:
	Unique<f32[]> FlipHDRVertically(f32 *pixels, int width, int height, int channels);

	void UploadToGPU(const void *data, VkDeviceSize size, VkImage image, uint32 width, uint32 height);
	void CopyBufferToImage(VkBuffer buffer, VkImage image, uint32 width, uint32 height);

	std::array<uint8, 4> ColorToPixel(vec4 color);

	Device &m_Device;
};

} // namespace Aquila::Graphics::Texture

#endif
