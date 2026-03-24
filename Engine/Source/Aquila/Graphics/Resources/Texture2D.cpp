#include "Aquila/Graphics/Resources/Texture2D.h"

#include "Aquila/Graphics/Pipeline/DescriptorAllocator.h"
#include "Aquila/Graphics/Resources/Buffer.h"
#include "Aquila/Graphics/Texture/ImageOperations.h"
#include "Aquila/Graphics/Texture/TextureLoader.h"
#include "Aquila/Graphics/Core/DeletionManager.h"

namespace Aquila::Graphics::Resources {

Texture2D::Texture2D(Device &device, std::string debugName) : m_Device(device), m_DebugName(std::move(debugName)) {
	InitializeHandles();
	CreateDescriptorSetLayout();
	AllocateDescriptorSet();
}

Texture2D::~Texture2D() {
	Destroy();
}

void Texture2D::CreateRenderTarget(uint32 width, uint32 height, VkFormat format, VkImageUsageFlags usage,
								   VkSampleCountFlagBits samples) {
	m_IsRenderTarget = true;
	Texture::TextureLoader loader(m_Device);

	auto imageData = loader.CreateImage(width, height, format, usage, samples);
	m_Image = imageData.image;
	m_Memory = imageData.memory;
	m_Format = imageData.format;
	m_MipLevels = 1;

	AQUILA_LOG_DEBUG("Creating RenderTarget: {} ({}x{}, format={})", m_DebugName, width, height, (int)format);
	Texture::ImageOperations::TransitionLayout(m_Device, m_Image, m_Format, VK_IMAGE_LAYOUT_UNDEFINED,
											   VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1);
	Texture::ImageOperations::TransitionLayout(m_Device, m_Image, m_Format, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
											   VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, 1);
	m_ImageView = Texture::ImageOperations::CreateImageView(m_Device, m_Image, m_Format, VK_IMAGE_VIEW_TYPE_2D, 1, 1);
	m_Sampler = Texture::ImageOperations::CreateSampler(m_Device, 1, false);
	UpdateDescriptorSet();
	m_Device.SetObjectDebugName(VK_OBJECT_TYPE_IMAGE, reinterpret_cast<uint64>(m_Image),
								(m_DebugName + "_RT_Image").c_str());
	m_Device.SetObjectDebugName(VK_OBJECT_TYPE_IMAGE_VIEW, reinterpret_cast<uint64>(m_ImageView),
								(m_DebugName + "_RT_ImageView").c_str());
	m_Device.SetObjectDebugName(VK_OBJECT_TYPE_SAMPLER, reinterpret_cast<uint64>(m_Sampler),
								(m_DebugName + "_RT_Sampler").c_str());
}

void Texture2D::CreateCubemap(uint32 width, uint32 height, VkFormat format, VkImageUsageFlags usage) {
	Texture::TextureLoader loader(m_Device);

	auto imageData = loader.CreateCubemap(width, height, format, usage, 1);
	m_Image = imageData.image;
	m_Memory = imageData.memory;
	m_Format = imageData.format;
	m_MipLevels = 1;

	Texture::ImageOperations::TransitionLayout(m_Device, m_Image, m_Format, VK_IMAGE_LAYOUT_UNDEFINED,
											   VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, 1, 6);

	m_ImageView = Texture::ImageOperations::CreateImageView(m_Device, m_Image, m_Format, VK_IMAGE_VIEW_TYPE_CUBE, 6, 1);
	m_Sampler = Texture::ImageOperations::CreateSampler(m_Device, 1, false);

	UpdateDescriptorSet();

	m_Device.SetObjectDebugName(VK_OBJECT_TYPE_IMAGE, reinterpret_cast<uint64>(m_Image),
								(m_DebugName + "_Cubemap_Image").c_str());
	m_Device.SetObjectDebugName(VK_OBJECT_TYPE_IMAGE_VIEW, reinterpret_cast<uint64>(m_ImageView),
								(m_DebugName + "_Cubemap_ImageView").c_str());
	m_Device.SetObjectDebugName(VK_OBJECT_TYPE_SAMPLER, reinterpret_cast<uint64>(m_Sampler),
								(m_DebugName + "_Cubemap_Sampler").c_str());
}

void Texture2D::CreateMipmappedCubemap(uint32 width, uint32 height, VkFormat format, VkImageUsageFlags usage) {
	Texture::TextureLoader loader(m_Device);

	m_MipLevels = Texture::ImageOperations::CalculateMipLevels(width, height);

	auto imageData = loader.CreateCubemap(width, height, format, usage, m_MipLevels);
	m_Image = imageData.image;
	m_Memory = imageData.memory;
	m_Format = imageData.format;

	m_ImageView =
		Texture::ImageOperations::CreateImageView(m_Device, m_Image, m_Format, VK_IMAGE_VIEW_TYPE_CUBE, 6, m_MipLevels);
	m_Sampler = Texture::ImageOperations::CreateMipmappedCubemapSampler(m_Device, m_MipLevels);

	UpdateDescriptorSet();

	m_Device.SetObjectDebugName(VK_OBJECT_TYPE_IMAGE, reinterpret_cast<uint64>(m_Image),
								(m_DebugName + "_MipCubemap_Image").c_str());
	m_Device.SetObjectDebugName(VK_OBJECT_TYPE_IMAGE_VIEW, reinterpret_cast<uint64>(m_ImageView),
								(m_DebugName + "_MipCubemap_ImageView").c_str());
	m_Device.SetObjectDebugName(VK_OBJECT_TYPE_SAMPLER, reinterpret_cast<uint64>(m_Sampler),
								(m_DebugName + "_MipCubemap_Sampler").c_str());

	AQUILA_LOG_DEBUG("Created mip-mapped cubemap with {} levels", m_MipLevels);
}

void Texture2D::CleanupStagingResources() {
	if (m_StagingBuffer != nullptr) {
		m_StagingBuffer->DestroyImmediate();
		m_StagingBuffer = nullptr;
	}
}

void Texture2D::CreateFallback(TextureType type) {
	Texture::TextureLoader loader(m_Device);

	vec4 color;
	switch (type) {
	case TextureType::Albedo:
		color = { 1.0F, 1.0F, 1.0F, 1.0F };
		break;
	case TextureType::Normal:
		color = { 0.5F, 0.5F, 1.0F, 1.0F };
		break;
	case TextureType::Emissive:
		color = { 0.0F, 0.0F, 0.0F, 1.0F };
		break;
	case TextureType::MetallicRoughness:
		color = { 0.0F, 0.5F, 0.0F, 1.0F };
		break;
	case TextureType::AO:
		color = { 1.0F, 1.0F, 1.0F, 1.0F };
		break;
	case TextureType::Cubemap:

		auto imageData = loader.CreateCubemap(1, 1, VK_FORMAT_R8G8B8A8_UNORM,
											  VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, 1);
		m_Image = imageData.image;
		m_Memory = imageData.memory;
		m_Format = imageData.format;
		m_MipLevels = 1;

		Texture::ImageOperations::TransitionLayout(m_Device, m_Image, m_Format, VK_IMAGE_LAYOUT_UNDEFINED,
												   VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, 1, 6);

		m_ImageView =
			Texture::ImageOperations::CreateImageView(m_Device, m_Image, m_Format, VK_IMAGE_VIEW_TYPE_CUBE, 6, 1);
		m_Sampler = Texture::ImageOperations::CreateSampler(m_Device, 1, false);
		UpdateDescriptorSet();

		return;
	}

	auto imageData = loader.CreateSolidColor(color, false);
	m_Image = imageData.image;
	m_Memory = imageData.memory;
	m_Format = imageData.format;
	m_MipLevels = 1;

	Texture::ImageOperations::TransitionLayout(m_Device, m_Image, m_Format, VK_IMAGE_LAYOUT_UNDEFINED,
											   VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1);
	Texture::ImageOperations::TransitionLayout(m_Device, m_Image, m_Format, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
											   VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, 1);

	m_ImageView = Texture::ImageOperations::CreateImageView(m_Device, m_Image, m_Format, VK_IMAGE_VIEW_TYPE_2D, 1, 1);
	m_Sampler = Texture::ImageOperations::CreateSampler(m_Device, 1, false);

	UpdateDescriptorSet();

	m_Device.SetObjectDebugName(VK_OBJECT_TYPE_IMAGE, reinterpret_cast<uint64>(m_Image),
								(m_DebugName + "_Fallback_Image").c_str());
	m_Device.SetObjectDebugName(VK_OBJECT_TYPE_IMAGE_VIEW, reinterpret_cast<uint64>(m_ImageView),
								(m_DebugName + "_Fallback_ImageView").c_str());
	m_Device.SetObjectDebugName(VK_OBJECT_TYPE_SAMPLER, reinterpret_cast<uint64>(m_Sampler),
								(m_DebugName + "_Fallback_Sampler").c_str());
}

VkDescriptorImageInfo Texture2D::GetDescriptorImageInfo() const {
	VkDescriptorImageInfo imageInfo{};
	imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	imageInfo.imageView = m_ImageView;
	imageInfo.sampler = m_Sampler;
	return imageInfo;
}

void Texture2D::Destroy() {
	DestroyVulkanResources();
	ReleaseDescriptorResources();
}

void Texture2D::InitializeHandles() {
	m_Image = VK_NULL_HANDLE;
	m_ImageView = VK_NULL_HANDLE;
	m_Sampler = VK_NULL_HANDLE;
	m_Memory = VK_NULL_HANDLE;
	m_DescriptorSet = VK_NULL_HANDLE;
}

void Texture2D::DestroyVulkanResources() {
	auto &queue = m_Device.GetDeletionManager();

	if (m_ImageView != VK_NULL_HANDLE) {
		queue.QueueDeletion(m_ImageView);
		m_ImageView = VK_NULL_HANDLE;
	}
	if (m_Image != VK_NULL_HANDLE) {
		queue.QueueDeletion(m_Image);
		m_Image = VK_NULL_HANDLE;
	}
	if (m_Sampler != VK_NULL_HANDLE) {
		queue.QueueDeletion(m_Sampler);
		m_Sampler = VK_NULL_HANDLE;
	}
	if (m_Memory != VK_NULL_HANDLE) {
		queue.QueueDeletion(m_Memory);
		m_Memory = VK_NULL_HANDLE;
	}
}

void Texture2D::CreateDescriptorSetLayout() {
	m_DescriptorSetLayout = RenderingPipeline::DescriptorSetLayout::Builder(m_Device)
								.AddBinding(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT)
								.Build();
}

void Texture2D::AllocateDescriptorSet() {
	bool success = RenderingPipeline::DescriptorAllocator::Allocate(m_DescriptorSetLayout->GetDescriptorSetLayout(),
																	m_DescriptorSet);

	if (!success) {
		AQUILA_LOG_ERROR("Failed to allocate descriptor set for texture: {}", m_DebugName);
		throw std::runtime_error("Descriptor allocation failed");
	}
}

void Texture2D::UpdateDescriptorSet() {
	VkDescriptorImageInfo imageInfo{};
	imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	imageInfo.imageView = m_ImageView;
	imageInfo.sampler = m_Sampler;

	auto pool = RenderingPipeline::DescriptorAllocator::GetSharedPool();
	if (!pool) {
		AQUILA_LOG_ERROR("Descriptor pool not available");
		return;
	}

	RenderingPipeline::DescriptorWriter writer(*m_DescriptorSetLayout, *pool);
	writer.WriteImage(0, &imageInfo);
	writer.Overwrite(m_DescriptorSet);
}

void Texture2D::ReleaseDescriptorResources() {
	if (m_DescriptorSet != VK_NULL_HANDLE) {
		RenderingPipeline::DescriptorAllocator::Release(m_DescriptorSet);
		m_DescriptorSet = VK_NULL_HANDLE;
	}

	if (m_DescriptorSetLayout) {
		m_DescriptorSetLayout.reset();
	}
}

void Texture2D::LoadPixelDataFromFile(const std::string &filepath, VkFormat format) {
	Texture::TextureLoader loader(m_Device);

	m_Filepath = filepath;
	m_Format = format;

	auto rawData =
		(filepath.find("://") != std::string::npos) ? loader.LoadFromVFS(filepath) : loader.LoadFromFile(filepath);

	if (!rawData.pixels || rawData.width <= 0 || rawData.height <= 0) {
		throw std::runtime_error("Failed to load texture or invalid dimensions: " + filepath);
	}

	m_Width = rawData.width;
	m_Height = rawData.height;
	m_ImageSize = static_cast<VkDeviceSize>(rawData.width) * static_cast<VkDeviceSize>(rawData.height) * 4;
	m_MipLevels = static_cast<uint32>(std::floor(std::log2(std::max(rawData.width, rawData.height)))) + 1;
	m_IsHDR = false;

	m_Pixels.emplace<Unique<uint8[]>>(std::move(rawData.pixels));
	AQUILA_LOG_DEBUG("LoadPixelDataFromFile END: variant index={}", m_Pixels.index());

	AQUILA_LOG_INFO("Loaded texture pixel data: {} ({}x{})", filepath, m_Width, m_Height);
}

void Texture2D::LoadPixelDataFromHDR(const std::string &filepath) {
	Texture::TextureLoader loader(m_Device);

	m_Filepath = filepath;
	m_IsHDR = true;

	// Load HDR data
	auto rawHDR = loader.LoadHDRFromFile(filepath);

	if (!rawHDR.pixels || rawHDR.width <= 0 || rawHDR.height <= 0) {
		throw std::runtime_error("Failed to load HDR texture or invalid dimensions: " + filepath);
	}

	constexpr uint32 MAX_TEXTURE_SIZE = 8192;
	if (rawHDR.width > MAX_TEXTURE_SIZE || rawHDR.height > MAX_TEXTURE_SIZE) {
		throw std::runtime_error("HDR texture exceeds maximum size: " + filepath);
	}

	m_Width = rawHDR.width;
	m_Height = rawHDR.height;
	m_ImageSize =
		static_cast<VkDeviceSize>(static_cast<unsigned long long>(rawHDR.width * rawHDR.height * 4) * sizeof(float));
	m_Format = VK_FORMAT_R32G32B32A32_SFLOAT;
	m_MipLevels = static_cast<uint32>(std::floor(std::log2(std::max(m_Width, m_Height)))) + 1;
	m_Pixels.emplace<Unique<f32[]>>(std::move(rawHDR.pixels));
	AQUILA_LOG_DEBUG("LoadPixelDataFromHDR END: variant index={}", m_Pixels.index());

	AQUILA_LOG_INFO("Loaded HDR texture from VFS: {} ({}x{})", filepath, m_Width, m_Height);
}

void Texture2D::UploadToGPU(VkCommandBuffer cmd) {
	if (std::holds_alternative<std::monostate>(m_Pixels)) {
		AQUILA_LOG_WARNING("No pixel data to upload for texture: {}", m_DebugName);
	}

	Texture::TextureLoader loader(m_Device);

	m_MipLevels = static_cast<uint32>(std::floor(std::log2(std::max(m_Width, m_Height)))) + 1;

	auto imageData = loader.CreateImage(m_Width, m_Height, m_Format,
										VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT |
											(m_MipLevels > 1 ? VK_IMAGE_USAGE_TRANSFER_SRC_BIT : 0),
										VK_SAMPLE_COUNT_1_BIT, m_MipLevels);

	m_Image = imageData.image;
	m_Memory = imageData.memory;

	m_StagingBuffer = CreateUnique<Resources::Buffer>(
		m_Device, "Texture_StagingBuffer", m_ImageSize, 1, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

	m_StagingBuffer->Map();
	std::visit(
		[&](auto &&pixels) {
			using T = std::decay_t<decltype(pixels)>;
			if constexpr (!std::is_same_v<T, std::monostate>) {
				m_StagingBuffer->Write(pixels.get(), m_ImageSize);
			}
		},
		m_Pixels);
	m_StagingBuffer->UnMap();

	VkImageMemoryBarrier barrier{};
	barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.image = m_Image;
	barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	barrier.subresourceRange.baseMipLevel = 0;
	barrier.subresourceRange.levelCount = m_MipLevels;
	barrier.subresourceRange.baseArrayLayer = 0;
	barrier.subresourceRange.layerCount = 1;
	barrier.srcAccessMask = 0;
	barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

	vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0,
						 nullptr, 1, &barrier);

	VkBufferImageCopy region{};
	region.bufferOffset = 0;
	region.bufferRowLength = 0;
	region.bufferImageHeight = 0;
	region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	region.imageSubresource.mipLevel = 0;
	region.imageSubresource.baseArrayLayer = 0;
	region.imageSubresource.layerCount = 1;
	region.imageOffset = { .x = 0, .y = 0, .z = 0 };
	region.imageExtent = { .width = m_Width, .height = m_Height, .depth = 1 };

	vkCmdCopyBufferToImage(cmd, m_StagingBuffer->GetBuffer(), m_Image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1,
						   &region);

	m_ImageView =
		Texture::ImageOperations::CreateImageView(m_Device, m_Image, m_Format, VK_IMAGE_VIEW_TYPE_2D, 1, m_MipLevels);
	m_Sampler = Texture::ImageOperations::CreateSampler(m_Device, m_MipLevels, m_MipLevels > 1);

	m_DescriptorSetUpdateNeeded = true;

	m_Device.SetObjectDebugName(VK_OBJECT_TYPE_IMAGE, reinterpret_cast<uint64>(m_Image),
								(m_DebugName + "_Image").c_str());
	m_Device.SetObjectDebugName(VK_OBJECT_TYPE_IMAGE_VIEW, reinterpret_cast<uint64>(m_ImageView),
								(m_DebugName + "_ImageView").c_str());
	m_Device.SetObjectDebugName(VK_OBJECT_TYPE_SAMPLER, reinterpret_cast<uint64>(m_Sampler),
								(m_DebugName + "_Sampler").c_str());

	AQUILA_LOG_DEBUG("Uploaded texture to GPU with {} mip levels: {}", m_MipLevels, m_DebugName);
}

void Texture2D::FinalizeDescriptors() {
	if (!m_DescriptorSetUpdateNeeded) {
		return;
	}

	if (m_DescriptorSet == VK_NULL_HANDLE) {
		AllocateDescriptorSet();
	}

	if (m_DescriptorSetUpdateNeeded) {
		UpdateDescriptorSet();
		m_DescriptorSetUpdateNeeded = false;
	}
}

Texture2D::Builder::Builder(Device &device, const std::string &debugName) : m_Device(device), m_DebugName(debugName) {}

Texture2D::Builder &Texture2D::Builder::FromFile(const std::string &filepath, VkFormat format) {
	m_SourceType = SourceType::File;
	m_Filepath = filepath;
	m_Format = format;
	return *this;
}

Texture2D::Builder &Texture2D::Builder::FromHDRFile(const std::string &filepath) {
	m_SourceType = SourceType::HDRFile;
	m_Filepath = filepath;
	return *this;
}

Texture2D::Builder &Texture2D::Builder::AsRenderTarget(uint32 width, uint32 height, VkFormat format,
													   VkImageUsageFlags usage) {
	m_SourceType = SourceType::RenderTarget;
	m_Width = width;
	m_Height = height;
	m_Format = format;
	m_Usage = usage;
	return *this;
}

Texture2D::Builder &Texture2D::Builder::AsCubemap(uint32 width, uint32 height, VkFormat format,
												  VkImageUsageFlags usage) {
	m_SourceType = SourceType::Cubemap;
	m_Width = width;
	m_Height = height;
	m_Format = format;
	m_Usage = usage;
	return *this;
}

Texture2D::Builder &Texture2D::Builder::AsMipmappedCubemap(uint32 width, uint32 height, VkFormat format,
														   VkImageUsageFlags usage) {
	m_SourceType = SourceType::MipmappedCubemap;
	m_Width = width;
	m_Height = height;
	m_Format = format;
	m_Usage = usage;
	return *this;
}

Texture2D::Builder &Texture2D::Builder::DeferGPUUpload() {
	m_DeferUpload = true;
	return *this;
}

Texture2D::Builder &Texture2D::Builder::AsEmpty(uint32 width, uint32 height, VkFormat format, VkImageUsageFlags usage) {
	m_SourceType = SourceType::Fallback;
	m_Width = width;
	m_Height = height;
	m_Format = format;
	m_Usage = usage;
	return *this;
}

Texture2D::Builder &Texture2D::Builder::AsFallback(TextureType type) {
	m_SourceType = SourceType::Fallback;
	m_FallbackType = type;
	return *this;
}

Texture2D::Builder &Texture2D::Builder::WithSamples(VkSampleCountFlagBits samples) {
	m_Samples = samples;
	return *this;
}

Ref<Texture2D> Texture2D::Builder::Build() {
	if (m_SourceType == SourceType::None) {
		throw std::runtime_error("Texture2D::Builder: No source type specified!");
	}

	auto texture = CreateUnique<Texture2D>(m_Device, m_DebugName);

	switch (m_SourceType) {
	case SourceType::File:
		if (m_DeferUpload) {
			texture->LoadPixelDataFromFile(m_Filepath, m_Format);
		}
		break;

	case SourceType::HDRFile:
		if (m_DeferUpload) {
			texture->LoadPixelDataFromHDR(m_Filepath);
		}
		break;

	case SourceType::RenderTarget:
		texture->CreateRenderTarget(m_Width, m_Height, m_Format, m_Usage, m_Samples);
		break;

	case SourceType::Cubemap:
		texture->CreateCubemap(m_Width, m_Height, m_Format, m_Usage);
		break;

	case SourceType::MipmappedCubemap:
		texture->CreateMipmappedCubemap(m_Width, m_Height, m_Format, m_Usage);
		break;

	case SourceType::Fallback:
		texture->CreateFallback(m_FallbackType);
		break;

	default:
		throw std::runtime_error("Texture2D::Builder: Unknown source type!");
	}

	return texture;
}

} // namespace Aquila::Graphics::Resources
