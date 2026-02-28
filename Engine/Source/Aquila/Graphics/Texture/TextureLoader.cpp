#include "Aquila/Graphics/Texture/TextureLoader.h"
#include "Aquila/Graphics/Resources/Buffer.h"
#include "Aquila/Graphics/Texture/ImageOperations.h"
#include "Aquila/Platform/Filesystem/VirtualFileSystem.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

namespace Aquila::Graphics::Texture {

TextureLoader::TextureLoader(Device &device) : m_Device(device) {}

TextureLoader::ImageData TextureLoader::CreateSolidColor(vec4 color, bool isHDR) {
	ImageData imageResult;
	imageResult.width = 1;
	imageResult.height = 1;
	imageResult.format = isHDR ? VK_FORMAT_R32G32B32A32_SFLOAT : VK_FORMAT_R8G8B8A8_UNORM;
	imageResult.mipLevels = 1;

	VkImageCreateInfo imageInfo{};
	imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	imageInfo.imageType = VK_IMAGE_TYPE_2D;
	imageInfo.extent.width = 1;
	imageInfo.extent.height = 1;
	imageInfo.extent.depth = 1;
	imageInfo.mipLevels = 1;
	imageInfo.arrayLayers = 1;
	imageInfo.format = imageResult.format;
	imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
	imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	imageInfo.usage = VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
	imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
	imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	AQUILA_VULKAN_CHECK(vkCreateImage(m_Device.GetDevice(), &imageInfo, nullptr, &imageResult.image));

	VkMemoryRequirements memRequirements;
	vkGetImageMemoryRequirements(m_Device.GetDevice(), imageResult.image, &memRequirements);

	VkMemoryAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocInfo.allocationSize = memRequirements.size;
	allocInfo.memoryTypeIndex =
		m_Device.FindMemoryType(memRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

	AQUILA_VULKAN_CHECK(vkAllocateMemory(m_Device.GetDevice(), &allocInfo, nullptr, &imageResult.memory));
	AQUILA_VULKAN_CHECK(vkBindImageMemory(m_Device.GetDevice(), imageResult.image, imageResult.memory, 0));

	Texture::ImageOperations::TransitionLayout(m_Device, imageResult.image, imageResult.format,
											   VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1);

	auto pixel = ColorToPixel(color);
	UploadToGPU(pixel.data(), sizeof(pixel), imageResult.image, 1, 1);

	return imageResult;
}

TextureLoader::ImageData TextureLoader::CreateImage(uint32 width, uint32 height, VkFormat format,
													VkImageUsageFlags usage, VkSampleCountFlagBits samples,
													uint32 mipLevels) {
	ImageData imageResult;
	imageResult.width = width;
	imageResult.height = height;
	imageResult.format = format;
	imageResult.mipLevels = mipLevels;

	VkImageCreateInfo imageInfo{};
	imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	imageInfo.imageType = VK_IMAGE_TYPE_2D;
	imageInfo.extent.width = width;
	imageInfo.extent.height = height;
	imageInfo.extent.depth = 1;
	imageInfo.mipLevels = mipLevels;
	imageInfo.arrayLayers = 1;
	imageInfo.format = format;
	imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
	imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	imageInfo.usage =
		usage | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
	imageInfo.samples = samples;
	imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	AQUILA_VULKAN_CHECK(vkCreateImage(m_Device.GetDevice(), &imageInfo, nullptr, &imageResult.image));

	VkMemoryRequirements memRequirements;
	vkGetImageMemoryRequirements(m_Device.GetDevice(), imageResult.image, &memRequirements);

	VkMemoryAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocInfo.allocationSize = memRequirements.size;
	allocInfo.memoryTypeIndex =
		m_Device.FindMemoryType(memRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

	AQUILA_VULKAN_CHECK(vkAllocateMemory(m_Device.GetDevice(), &allocInfo, nullptr, &imageResult.memory));
	AQUILA_VULKAN_CHECK(vkBindImageMemory(m_Device.GetDevice(), imageResult.image, imageResult.memory, 0));

	return imageResult;
}

TextureLoader::ImageData TextureLoader::CreateCubemap(uint32 width, uint32 height, VkFormat format,
													  VkImageUsageFlags usage, uint32 mipLevels) {
	ImageData imageResult;
	imageResult.width = width;
	imageResult.height = height;
	imageResult.format = format;
	imageResult.mipLevels = mipLevels;

	VkImageCreateInfo imageInfo{};
	imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	imageInfo.imageType = VK_IMAGE_TYPE_2D;
	imageInfo.extent.width = width;
	imageInfo.extent.height = height;
	imageInfo.extent.depth = 1;
	imageInfo.mipLevels = mipLevels;
	imageInfo.arrayLayers = 6;
	imageInfo.format = format;
	imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
	imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	imageInfo.usage = usage | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
	imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
	imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	imageInfo.flags = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;

	AQUILA_VULKAN_CHECK(vkCreateImage(m_Device.GetDevice(), &imageInfo, nullptr, &imageResult.image));

	VkMemoryRequirements memRequirements;
	vkGetImageMemoryRequirements(m_Device.GetDevice(), imageResult.image, &memRequirements);

	VkMemoryAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocInfo.allocationSize = memRequirements.size;
	allocInfo.memoryTypeIndex =
		m_Device.FindMemoryType(memRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

	AQUILA_VULKAN_CHECK(vkAllocateMemory(m_Device.GetDevice(), &allocInfo, nullptr, &imageResult.memory));
	vkBindImageMemory(m_Device.GetDevice(), imageResult.image, imageResult.memory, 0);

	return imageResult;
}

TextureLoader::RawImageData TextureLoader::LoadFromFile(const std::string &filepath) {
	RawImageData data;

	int width = 0, height = 0, channels = 0;
	stbi_uc *pixels = stbi_load(filepath.c_str(), &width, &height, &channels, STBI_rgb_alpha);

	data.width = static_cast<uint32>(width);
	data.height = static_cast<uint32>(height);
	data.channels = static_cast<uint32>(channels);

	if (pixels != nullptr) {
		size_t size = static_cast<size_t>(data.width * data.height) * 4;
		data.pixels = CreateUnique<uint8[]>(size);
		std::memcpy(data.pixels.get(), pixels, size);
		stbi_image_free(pixels);
	}

	return data;
}

TextureLoader::RawImageData TextureLoader::LoadFromVFS(const std::string &filepath) {
	auto file = Platform::Filesystem::VirtualFileSystem::Get()->OpenFile(filepath, "rb");
	if (!file || !file->IsValid()) {
		throw std::runtime_error("Failed to open VFS file: " + filepath);
	}

	int64 size = file->Size();
	if (size <= 0) {
		throw std::runtime_error("VFS file has invalid size: " + filepath);
	}

	std::vector<char> buffer(size);
	size_t bytesRead = file->Read(buffer.data(), size);
	if (bytesRead != static_cast<size_t>(size)) {
		throw std::runtime_error("Failed to read complete VFS file: " + filepath);
	}

	RawImageData data;

	int width = 0, height = 0, channels = 0;
	stbi_uc *pixels = stbi_load_from_memory(reinterpret_cast<const stbi_uc *>(buffer.data()),
											static_cast<int>(bytesRead), &width, &height, &channels, STBI_rgb_alpha);

	data.width = static_cast<uint32>(width);
	data.height = static_cast<uint32>(height);
	data.channels = static_cast<uint32>(channels);

	if (pixels != nullptr) {
		size_t pixelSize = static_cast<size_t>(data.width * data.height) * 4;
		data.pixels = CreateUnique<uint8[]>(pixelSize);
		std::memcpy(data.pixels.get(), pixels, pixelSize);
		stbi_image_free(pixels);
	}

	return data;
}

TextureLoader::RawHDRData TextureLoader::LoadHDRFromFile(const std::string &filepath) {
	stbi_set_flip_vertically_on_load(false);

	RawHDRData data{};

	if (filepath.find("://") != std::string::npos) {
		// Load from VFS
		auto file = Platform::Filesystem::VirtualFileSystem::Get()->OpenFile(filepath, "rb");
		if (!file || !file->IsValid()) {
			AQUILA_LOG_ERROR("Failed to open VFS file: {}", filepath);
			return data;
		}

		int64 size = file->Size();
		if (size <= 0) {
			AQUILA_LOG_ERROR("VFS file has invalid size: {}", filepath);
			return data;
		}

		std::vector<char> buffer(size);
		size_t bytesRead = file->Read(buffer.data(), size);
		if (bytesRead != static_cast<size_t>(size)) {
			AQUILA_LOG_ERROR("Failed to read complete VFS file: {}", filepath);
			return data;
		}

		int width = 0, height = 0, channels = 0;
		f32 *pixels = stbi_loadf_from_memory(reinterpret_cast<const stbi_uc *>(buffer.data()),
											 static_cast<int>(bytesRead), &width, &height, &channels, 4);

		data.width = static_cast<uint32>(width);
		data.height = static_cast<uint32>(height);
		data.channels = static_cast<uint32>(channels);

		if (pixels == nullptr) {
			AQUILA_LOG_ERROR("Failed to load HDR from VFS: {} - {}", filepath, stbi_failure_reason());
			return data;
		}

		size_t pixelCount = static_cast<size_t>(data.width) * data.height * 4;
		data.pixels = CreateUnique<f32[]>(pixelCount);
		std::memcpy(data.pixels.get(), pixels, pixelCount * sizeof(f32));
		stbi_image_free(pixels);

		AQUILA_LOG_INFO("Loaded HDR texture from VFS: {} ({}x{})", filepath, data.width, data.height);
	} else {
		int width = 0, height = 0, channels = 0;
		f32 *pixels = stbi_loadf(filepath.c_str(), &width, &height, &channels, 4);

		data.width = static_cast<uint32>(width);
		data.height = static_cast<uint32>(height);
		data.channels = static_cast<uint32>(channels);

		if (pixels == nullptr) {
			AQUILA_LOG_ERROR("Failed to load HDR file: {} - {}", filepath, stbi_failure_reason());
			return data;
		}

		size_t pixelCount = static_cast<size_t>(data.width) * data.height * 4;
		data.pixels = CreateUnique<f32[]>(pixelCount);
		std::memcpy(data.pixels.get(), pixels, pixelCount * sizeof(f32));
		stbi_image_free(pixels);

		AQUILA_LOG_INFO("Loaded HDR texture from file: {} ({}x{})", filepath, data.width, data.height);
	}

	return data;
}

void TextureLoader::UploadToGPU(const void *data, VkDeviceSize size, VkImage image, uint32 width, uint32 height) {
	Resources::Buffer stagingBuffer{ m_Device,
									 "Texture_StagingBuffer",
									 size,
									 1,
									 VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
									 VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT };

	stagingBuffer.Map();
	stagingBuffer.Write(const_cast<void *>(data));
	stagingBuffer.UnMap();

	CopyBufferToImage(stagingBuffer.GetBuffer(), image, width, height);
	stagingBuffer.DestroyImmediate();
}

void TextureLoader::CopyBufferToImage(VkBuffer buffer, VkImage image, uint32 width, uint32 height) {
	m_Device.ExecuteGraphicsCommands([&](VkCommandBuffer cmd) {
		VkBufferImageCopy region{};
		region.bufferOffset = 0;
		region.bufferRowLength = 0;
		region.bufferImageHeight = 0;
		region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		region.imageSubresource.mipLevel = 0;
		region.imageSubresource.baseArrayLayer = 0;
		region.imageSubresource.layerCount = 1;
		region.imageOffset = { .x = 0, .y = 0, .z = 0 };
		region.imageExtent = { .width = width, .height = height, .depth = 1 };

		vkCmdCopyBufferToImage(cmd, buffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);
	});
}

std::array<uint8, 4> TextureLoader::ColorToPixel(vec4 color) {
	return { static_cast<uint8>(color.r * 255.0f), static_cast<uint8>(color.g * 255.0f),
			 static_cast<uint8>(color.b * 255.0f), static_cast<uint8>(color.a * 255.0f) };
}

} // namespace Aquila::Graphics::Texture
