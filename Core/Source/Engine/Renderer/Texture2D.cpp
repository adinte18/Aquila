#include "Engine/Renderer/Texture2D.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
namespace Engine {

Texture2D::Texture2D(Device &device, const std::string &debugName)
    : m_Device(device), m_DebugName(debugName) {
  InitializeHandles();
  CreateDescriptorSetLayout();
  AllocateDescriptorSet();
}

Texture2D::~Texture2D() { DestroyAll(); }

void Texture2D::InitializeHandles() {
  m_Image = VK_NULL_HANDLE;
  m_Sampler = VK_NULL_HANDLE;
  m_ImageView = VK_NULL_HANDLE;
  m_Memory = VK_NULL_HANDLE;
  descriptorSet = VK_NULL_HANDLE;
}

void Texture2D::CreateDescriptorSetLayout() {
  descriptorSetLayout =
      DescriptorSetLayout::Builder(m_Device)
          .addBinding(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                      VK_SHADER_STAGE_FRAGMENT_BIT)
          .build();
}

void Texture2D::AllocateDescriptorSet() {
  DescriptorAllocator::Allocate(descriptorSetLayout->GetDescriptorSetLayout(),
                                descriptorSet);
}

void Texture2D::DestroyAll() {
  DestroyVulkanResources();
  ReleaseDescriptorResources();
}

void Texture2D::DestroyVulkanResources() {
  VkDevice device = m_Device.GetDevice();

  if (m_ImageView != VK_NULL_HANDLE) {
    vkDestroyImageView(device, m_ImageView, nullptr);
    m_ImageView = VK_NULL_HANDLE;
  }

  if (m_Image != VK_NULL_HANDLE) {
    vkDestroyImage(device, m_Image, nullptr);
    m_Image = VK_NULL_HANDLE;
  }

  if (m_Sampler != VK_NULL_HANDLE) {
    vkDestroySampler(device, m_Sampler, nullptr);
    m_Sampler = VK_NULL_HANDLE;
  }

  if (m_Memory != VK_NULL_HANDLE) {
    vkFreeMemory(device, m_Memory, nullptr);
    m_Memory = VK_NULL_HANDLE;
  }
}

void Texture2D::ReleaseDescriptorResources() {
  if (descriptorSet != VK_NULL_HANDLE) {
    std::vector<VkDescriptorSet> sets{descriptorSet};
    DescriptorAllocator::Release(sets);
    descriptorSet = VK_NULL_HANDLE;
  }

  if (descriptorSetLayout) {
    descriptorSetLayout.reset();
  }
}

void Texture2D::Destroy() { DestroyAll(); }

// Public API methods
void Texture2D::CreateHDRTexture(const std::string &filepath, VkFormat format) {
  CreateHDRTextureImage(filepath);
  CreateTextureImageView(format);
  CreateTextureSampler();
  WriteToDescriptorSet();
}

void Texture2D::CreateCubeMap(uint32_t width, uint32_t height, VkFormat format,
                              VkImageUsageFlags usage) {
  CreateCubemapImage(width, height, format, usage);
  CreateCubemapImageView(format);
  CreateTextureSampler();
  WriteToDescriptorSet();
}

void Texture2D::CreateMipMappedCubemap(uint32_t width, uint32_t height,
                                       VkFormat format,
                                       VkImageUsageFlags usage) {
  CalculateMipLevels(width, height);
  m_Format = format;

  CreateMipMappedCubemapImage(width, height, format);
  CreateMipMappedCubemapImageView(format);
  CreateMipMappedCubemapSampler();
  WriteToDescriptorSet();

  std::cout << "Created mip-mapped cubemap" << std::endl;
}

void Texture2D::CreateTexture(const std::string &filepath, VkFormat format) {
  CreateTextureImageFromFile(filepath);
  CreateTextureImageView(format);
  CreateTextureSampler();
  WriteToDescriptorSet();
}

void Texture2D::CreateTexture(const uint32_t width, const uint32_t height,
                              const VkFormat format,
                              const VkImageUsageFlags usage,
                              const VkSampleCountFlagBits samples) {
  CreateTextureImage(width, height, format, usage, samples);
  CreateTextureImageView(format);
  CreateTextureSampler();
  WriteToDescriptorSet();
}

void Texture2D::UseFallbackTextures(TextureType type) {
  switch (type) {
  case TextureType::Albedo:
    CreateSolidColorTexture({1.0f, 1.0f, 1.0f, 1.0f});
    break;
  case TextureType::Normal:
    CreateSolidColorTexture({0.5f, 0.5f, 1.0f, 1.0f});
    break;
  case TextureType::Emissive:
    CreateSolidColorTexture({1.0f, 1.0f, 1.0f, 1.0f});
    break;
  case TextureType::MetallicRoughness:
    CreateSolidColorTexture({1.0f, 0.5f, 0.0f, 1.0f});
    break;
  case TextureType::AO:
    CreateSolidColorTexture({1.0f, 1.0f, 1.0f, 1.0f});
    break;
  case TextureType::Cubemap:
    CreateSolidColorCubemap({1.0f, 1.0f, 1.0f, 1.0f});
    break;
  default:
    throw std::invalid_argument("Unknown texture type");
  }
  WriteToDescriptorSet();
}

// Helper methods
void Texture2D::CalculateMipLevels(uint32_t width, uint32_t height) {
  m_MipLevels =
      static_cast<uint32_t>(std::floor(std::log2(std::max(width, height)))) + 1;
}

void Texture2D::WriteToDescriptorSet() {
  VkDescriptorImageInfo imageInfo{};
  imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
  imageInfo.imageView = m_ImageView;
  imageInfo.sampler = m_Sampler;

  DescriptorWriter writer(*descriptorSetLayout,
                          *DescriptorAllocator::GetSharedPool());
  writer.writeImage(0, &imageInfo);
  writer.overwrite(descriptorSet);
}

VkDescriptorImageInfo Texture2D::GetDescriptorSetInfo() const {
  VkDescriptorImageInfo imageInfo{};
  imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
  imageInfo.imageView = GetTextureImageView();
  imageInfo.sampler = GetTextureSampler();
  return imageInfo;
}

// Image creation methods
void Texture2D::CreateImage(uint32_t width, uint32_t height, VkFormat format,
                            VkImageTiling tiling, VkImageUsageFlags usage,
                            VkMemoryPropertyFlags properties, VkImage &image,
                            VkDeviceMemory &imageMemory) {
  CalculateMipLevels(width, height);

  VkImageCreateInfo imageInfo =
      CreateImageCreateInfo(width, height, format, tiling, usage, 1, 1);
  imageInfo.mipLevels = m_MipLevels;

  AQUILA_VULKAN_CHECK(
      vkCreateImage(m_Device.GetDevice(), &imageInfo, nullptr, &image));

  AllocateAndBindImageMemory(image, properties, imageMemory);
}

VkImageCreateInfo Texture2D::CreateImageCreateInfo(
    uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling,
    VkImageUsageFlags usage, uint32_t arrayLayers, uint32_t mipLevels) const {
  VkImageCreateInfo imageInfo{};
  imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
  imageInfo.imageType = VK_IMAGE_TYPE_2D;
  imageInfo.extent.width = width;
  imageInfo.extent.height = height;
  imageInfo.extent.depth = 1;
  imageInfo.mipLevels = mipLevels;
  imageInfo.arrayLayers = arrayLayers;
  imageInfo.format = format;
  imageInfo.tiling = tiling;
  imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  imageInfo.usage = usage;
  imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
  imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
  return imageInfo;
}

void Texture2D::AllocateAndBindImageMemory(VkImage image,
                                           VkMemoryPropertyFlags properties,
                                           VkDeviceMemory &imageMemory) {
  VkMemoryRequirements memRequirements;
  vkGetImageMemoryRequirements(m_Device.GetDevice(), image, &memRequirements);

  VkMemoryAllocateInfo allocInfo{};
  allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
  allocInfo.allocationSize = memRequirements.size;
  allocInfo.memoryTypeIndex =
      m_Device.FindMemoryType(memRequirements.memoryTypeBits, properties);

  AQUILA_VULKAN_CHECK(vkAllocateMemory(m_Device.GetDevice(), &allocInfo,
                                       nullptr, &imageMemory));
  vkBindImageMemory(m_Device.GetDevice(), image, imageMemory, 0);
}

// Mipmap generation
void Texture2D::GenerateMipmap(VkImage image, VkFormat imageFormat,
                               int32_t texWidth, int32_t texHeight,
                               uint32_t mipLevels) {
  ValidateLinearFilteringSupport(imageFormat);

  VkCommandBuffer commandBuffer = m_Device.BeginSingleTimeCommands();
  VkImageMemoryBarrier barrier = CreateMipmapBarrier(image);

  int32_t mipWidth = texWidth;
  int32_t mipHeight = texHeight;

  for (uint32_t i = 1; i < mipLevels; i++) {
    TransitionMipLevel(
        commandBuffer, barrier, i - 1, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_ACCESS_TRANSFER_WRITE_BIT,
        VK_ACCESS_TRANSFER_READ_BIT);

    BlitMipLevel(commandBuffer, image, i - 1, i, mipWidth, mipHeight);

    TransitionMipLevel(commandBuffer, barrier, i - 1,
                       VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                       VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                       VK_ACCESS_TRANSFER_READ_BIT, VK_ACCESS_SHADER_READ_BIT);

    CalculateNextMipDimensions(mipWidth, mipHeight);
  }

  // Transition final mip level
  TransitionMipLevel(commandBuffer, barrier, mipLevels - 1,
                     VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                     VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                     VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT);

  m_Device.EndSingleTimeCommands(commandBuffer);
}

void Texture2D::ValidateLinearFilteringSupport(VkFormat imageFormat) const {
  VkFormatProperties formatProperties;
  vkGetPhysicalDeviceFormatProperties(m_Device.GetPhysicalDevice(), imageFormat,
                                      &formatProperties);

  if (!(formatProperties.optimalTilingFeatures &
        VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT)) {
    throw std::runtime_error(
        "Texture image format does not support linear blitting!");
  }
}

VkImageMemoryBarrier Texture2D::CreateMipmapBarrier(VkImage image) const {
  VkImageMemoryBarrier barrier{};
  barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
  barrier.image = image;
  barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  barrier.subresourceRange.baseArrayLayer = 0;
  barrier.subresourceRange.layerCount = 1;
  barrier.subresourceRange.levelCount = 1;
  return barrier;
}

void Texture2D::TransitionMipLevel(VkCommandBuffer commandBuffer,
                                   VkImageMemoryBarrier &barrier,
                                   uint32_t mipLevel, VkImageLayout oldLayout,
                                   VkImageLayout newLayout,
                                   VkAccessFlags srcAccess,
                                   VkAccessFlags dstAccess) const {
  barrier.subresourceRange.baseMipLevel = mipLevel;
  barrier.oldLayout = oldLayout;
  barrier.newLayout = newLayout;
  barrier.srcAccessMask = srcAccess;
  barrier.dstAccessMask = dstAccess;

  VkPipelineStageFlags srcStage =
      (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
          ? VK_PIPELINE_STAGE_TRANSFER_BIT
          : VK_PIPELINE_STAGE_TRANSFER_BIT;
  VkPipelineStageFlags dstStage =
      (newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
          ? VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT
          : VK_PIPELINE_STAGE_TRANSFER_BIT;

  vkCmdPipelineBarrier(commandBuffer, srcStage, dstStage, 0, 0, nullptr, 0,
                       nullptr, 1, &barrier);
}

void Texture2D::BlitMipLevel(VkCommandBuffer commandBuffer, VkImage image,
                             uint32_t srcMip, uint32_t dstMip, int32_t srcWidth,
                             int32_t srcHeight) const {
  VkImageBlit blit{};
  blit.srcOffsets[0] = {0, 0, 0};
  blit.srcOffsets[1] = {srcWidth, srcHeight, 1};
  blit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  blit.srcSubresource.mipLevel = srcMip;
  blit.srcSubresource.baseArrayLayer = 0;
  blit.srcSubresource.layerCount = 1;

  blit.dstOffsets[0] = {0, 0, 0};
  blit.dstOffsets[1] = {srcWidth > 1 ? srcWidth / 2 : 1,
                        srcHeight > 1 ? srcHeight / 2 : 1, 1};
  blit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  blit.dstSubresource.mipLevel = dstMip;
  blit.dstSubresource.baseArrayLayer = 0;
  blit.dstSubresource.layerCount = 1;

  vkCmdBlitImage(commandBuffer, image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                 image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &blit,
                 VK_FILTER_LINEAR);
}

void Texture2D::CalculateNextMipDimensions(int32_t &width,
                                           int32_t &height) const {
  if (width > 1)
    width /= 2;
  if (height > 1)
    height /= 2;
}

// Layout transition
void Texture2D::TransitionImageLayout(VkImage image, VkFormat format,
                                      VkImageLayout oldLayout,
                                      VkImageLayout newLayout,
                                      uint32_t layers) {
  VkCommandBuffer commandBuffer = m_Device.BeginSingleTimeCommands();

  VkImageMemoryBarrier barrier = CreateLayoutTransitionBarrier(
      image, format, oldLayout, newLayout, layers);

  auto [sourceStage, destinationStage] =
      GetPipelineStages(oldLayout, newLayout);

  vkCmdPipelineBarrier(commandBuffer, sourceStage, destinationStage, 0, 0,
                       nullptr, 0, nullptr, 1, &barrier);

  m_Device.EndSingleTimeCommands(commandBuffer);
}

VkImageMemoryBarrier Texture2D::CreateLayoutTransitionBarrier(
    VkImage image, VkFormat format, VkImageLayout oldLayout,
    VkImageLayout newLayout, uint32_t layers) const {

  VkImageMemoryBarrier barrier{};
  barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
  barrier.oldLayout = oldLayout;
  barrier.newLayout = newLayout;
  barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  barrier.image = image;
  barrier.subresourceRange.aspectMask = (format == VK_FORMAT_D32_SFLOAT)
                                            ? VK_IMAGE_ASPECT_DEPTH_BIT
                                            : VK_IMAGE_ASPECT_COLOR_BIT;
  barrier.subresourceRange.baseMipLevel = 0;
  barrier.subresourceRange.levelCount = m_MipLevels;
  barrier.subresourceRange.baseArrayLayer = 0;
  barrier.subresourceRange.layerCount = layers;

  auto [srcAccess, dstAccess] = GetAccessMasks(oldLayout, newLayout);
  barrier.srcAccessMask = srcAccess;
  barrier.dstAccessMask = dstAccess;

  return barrier;
}

std::pair<VkAccessFlags, VkAccessFlags>
Texture2D::GetAccessMasks(VkImageLayout oldLayout,
                          VkImageLayout newLayout) const {

  VkAccessFlags srcAccess = 0;
  VkAccessFlags dstAccess = 0;

  // Source access masks
  switch (oldLayout) {
  case VK_IMAGE_LAYOUT_UNDEFINED:
  case VK_IMAGE_LAYOUT_PREINITIALIZED:
    srcAccess = (oldLayout == VK_IMAGE_LAYOUT_PREINITIALIZED)
                    ? VK_ACCESS_HOST_WRITE_BIT
                    : 0;
    break;
  case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
    srcAccess = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    break;
  case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
    srcAccess = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
    break;
  case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
    srcAccess = VK_ACCESS_TRANSFER_READ_BIT;
    break;
  case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
    srcAccess = VK_ACCESS_TRANSFER_WRITE_BIT;
    break;
  case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
    srcAccess = VK_ACCESS_SHADER_READ_BIT;
    break;
  default:
    throw std::invalid_argument("Unsupported old layout transition!");
  }

  // Destination access masks
  switch (newLayout) {
  case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
    dstAccess = VK_ACCESS_TRANSFER_WRITE_BIT;
    break;
  case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
    dstAccess = VK_ACCESS_TRANSFER_READ_BIT;
    break;
  case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
    dstAccess = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    break;
  case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
    dstAccess = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
    break;
  case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
    dstAccess = VK_ACCESS_SHADER_READ_BIT;
    break;
  default:
    throw std::invalid_argument("Unsupported new layout transition!");
  }

  return {srcAccess, dstAccess};
}

std::pair<VkPipelineStageFlags, VkPipelineStageFlags>
Texture2D::GetPipelineStages(VkImageLayout oldLayout,
                             VkImageLayout newLayout) const {

  VkPipelineStageFlags sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
  VkPipelineStageFlags destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;

  switch (oldLayout) {
  case VK_IMAGE_LAYOUT_UNDEFINED:
    sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
    break;
  case VK_IMAGE_LAYOUT_PREINITIALIZED:
    sourceStage = VK_PIPELINE_STAGE_HOST_BIT;
    break;
  case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
    sourceStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    break;
  case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
    sourceStage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    break;
  case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
  case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
    sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
    break;
  case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
    sourceStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    break;
  }

  switch (newLayout) {
  case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
  case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
    destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
    break;
  case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
    destinationStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    break;
  case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
    destinationStage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    break;
  case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
    destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    break;
  }

  return {sourceStage, destinationStage};
}

// Buffer to image copy
void Texture2D::CopyBufferToImage(VkBuffer buffer, VkImage image,
                                  uint32_t width, uint32_t height) const {
  VkCommandBuffer commandBuffer = m_Device.BeginSingleTimeCommands();

  VkBufferImageCopy region{};
  region.bufferOffset = 0;
  region.bufferRowLength = 0;
  region.bufferImageHeight = 0;
  region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  region.imageSubresource.mipLevel = 0;
  region.imageSubresource.baseArrayLayer = 0;
  region.imageSubresource.layerCount = 1;
  region.imageOffset = {0, 0, 0};
  region.imageExtent = {width, height, 1};

  vkCmdCopyBufferToImage(commandBuffer, buffer, image,
                         VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

  m_Device.EndSingleTimeCommands(commandBuffer);
}

// Specific texture creation implementations
void Texture2D::CreateHDRTextureImage(const std::string &filepath) {
  stbi_set_flip_vertically_on_load(false);

  int texWidth, texHeight, texChannels;
  float *pixels =
      stbi_loadf(filepath.c_str(), &texWidth, &texHeight, &texChannels, 4);
  if (!pixels) {
    throw std::runtime_error("Failed to load HDR texture image: " + filepath);
  }

  auto flippedPixels = FlipImageVertically(pixels, texWidth, texHeight, 4);
  VkDeviceSize imageSize = texWidth * texHeight * 4 * sizeof(float);

  Buffer stagingBuffer =
      CreateStagingBuffer(imageSize, "HDR_Texture_StagingBuffer");
  stagingBuffer.Map();
  stagingBuffer.Write(flippedPixels.get());
  stagingBuffer.UnMap();

  stbi_image_free(pixels);

  CreateImage(texWidth, texHeight, VK_FORMAT_R32G32B32A32_SFLOAT,
              VK_IMAGE_TILING_OPTIMAL,
              VK_IMAGE_USAGE_TRANSFER_SRC_BIT |
                  VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
              VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, m_Image, m_Memory);

  TransitionImageLayout(m_Image, VK_FORMAT_R32G32B32A32_SFLOAT,
                        VK_IMAGE_LAYOUT_UNDEFINED,
                        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

  CopyBufferToImage(stagingBuffer.GetBuffer(), m_Image,
                    static_cast<uint32_t>(texWidth),
                    static_cast<uint32_t>(texHeight));

  GenerateMipmap(m_Image, VK_FORMAT_R32G32B32A32_SFLOAT, texWidth, texHeight,
                 m_MipLevels);

  SetDebugName(VK_OBJECT_TYPE_IMAGE, m_Image, m_DebugName + "_HDR_Image");
}

std::unique_ptr<float[]> Texture2D::FlipImageVertically(float *pixels,
                                                        int width, int height,
                                                        int channels) const {
  auto flippedPixels = std::make_unique<float[]>(width * height * channels);

  for (int y = 0; y < height; ++y) {
    for (int x = 0; x < width; ++x) {
      int originalIdx = (y * width + x) * channels;
      int flippedIdx = ((height - 1 - y) * width + x) * channels;

      for (int c = 0; c < channels; ++c) {
        flippedPixels[flippedIdx + c] = pixels[originalIdx + c];
      }
    }
  }

  return flippedPixels;
}

Buffer Texture2D::CreateStagingBuffer(VkDeviceSize size,
                                      const std::string &debugName) const {
  return Buffer{m_Device,
                debugName,
                size,
                1,
                VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT};
}

void Texture2D::SetDebugName(VkObjectType objectType, void *handle,
                             const std::string &name) const {
  m_Device.SetObjectDebugName(objectType, reinterpret_cast<uint64>(handle),
                              name.c_str());
}

void Texture2D::CreateTextureImageFromFile(const std::string &filepath) {
  int texWidth, texHeight, texChannels;
  stbi_uc *pixels = stbi_load(filepath.c_str(), &texWidth, &texHeight,
                              &texChannels, STBI_rgb_alpha);

  if (!pixels) {
    throw std::runtime_error("Failed to load texture image: " + filepath);
  }

  VkDeviceSize imageSize = texWidth * texHeight * 4;
  Buffer stagingBuffer =
      CreateStagingBuffer(imageSize, "Texture_StagingBuffer");

  stagingBuffer.Map();
  stagingBuffer.Write(pixels);
  stagingBuffer.UnMap();

  stbi_image_free(pixels);

  CreateImage(texWidth, texHeight, VK_FORMAT_R8G8B8A8_UNORM,
              VK_IMAGE_TILING_OPTIMAL,
              VK_IMAGE_USAGE_TRANSFER_SRC_BIT |
                  VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
              VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, m_Image, m_Memory);

  TransitionImageLayout(m_Image, VK_FORMAT_R8G8B8A8_UNORM,
                        VK_IMAGE_LAYOUT_UNDEFINED,
                        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

  CopyBufferToImage(stagingBuffer.GetBuffer(), m_Image,
                    static_cast<uint32_t>(texWidth),
                    static_cast<uint32_t>(texHeight));

  GenerateMipmap(m_Image, VK_FORMAT_R8G8B8A8_UNORM, texWidth, texHeight,
                 m_MipLevels);

  SetDebugName(VK_OBJECT_TYPE_IMAGE, m_Image, m_DebugName + "_LDR_Image");
}

void Texture2D::CreateTextureImage(uint32_t width, uint32_t height,
                                   VkFormat format, VkImageUsageFlags usage,
                                   VkSampleCountFlagBits samples) {
  m_Format = format;

  VkImageCreateInfo imageInfo = CreateImageCreateInfo(
      width, height, format, VK_IMAGE_TILING_OPTIMAL,
      usage | VK_IMAGE_USAGE_TRANSFER_SRC_BIT |
          VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
      1, 1);
  imageInfo.samples = samples;

  AQUILA_VULKAN_CHECK(
      vkCreateImage(m_Device.GetDevice(), &imageInfo, nullptr, &m_Image));

  AllocateAndBindImageMemory(m_Image, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                             m_Memory);
  SetDebugName(VK_OBJECT_TYPE_IMAGE, m_Image, m_DebugName + "_LDR_Image");
}

void Texture2D::CreateTextureImageView(VkFormat format) {
  m_Format = format;
  m_ImageView = CreateImageView(m_Image, format, VK_IMAGE_VIEW_TYPE_2D, 1, 1);
  SetDebugName(VK_OBJECT_TYPE_IMAGE_VIEW, m_ImageView,
               m_DebugName + "_LDR_ImageView");
}

void Texture2D::CreateTextureSampler() {
  VkSamplerCreateInfo samplerInfo = CreateSamplerCreateInfo();
  AQUILA_VULKAN_CHECK(
      vkCreateSampler(m_Device.GetDevice(), &samplerInfo, nullptr, &m_Sampler));
  SetDebugName(VK_OBJECT_TYPE_SAMPLER, m_Sampler, m_DebugName + "_LDR_Sampler");
}

VkImageView Texture2D::CreateImageView(VkImage image, VkFormat format,
                                       VkImageViewType viewType,
                                       uint32_t layerCount,
                                       uint32_t mipLevels) const {
  VkImageViewCreateInfo viewInfo{};
  viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
  viewInfo.image = image;
  viewInfo.viewType = viewType;
  viewInfo.format = format;
  viewInfo.subresourceRange.aspectMask = (format == VK_FORMAT_D32_SFLOAT)
                                             ? VK_IMAGE_ASPECT_DEPTH_BIT
                                             : VK_IMAGE_ASPECT_COLOR_BIT;
  viewInfo.subresourceRange.baseMipLevel = 0;
  viewInfo.subresourceRange.levelCount = mipLevels;
  viewInfo.subresourceRange.baseArrayLayer = 0;
  viewInfo.subresourceRange.layerCount = layerCount;

  VkImageView imageView;
  AQUILA_VULKAN_CHECK(
      vkCreateImageView(m_Device.GetDevice(), &viewInfo, nullptr, &imageView));
  return imageView;
}

VkSamplerCreateInfo Texture2D::CreateSamplerCreateInfo() const {
  VkPhysicalDeviceProperties properties{};
  vkGetPhysicalDeviceProperties(m_Device.GetPhysicalDevice(), &properties);

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
  return samplerInfo;
}

// Cubemap specific methods
void Texture2D::CreateCubemapImage(uint32_t width, uint32_t height,
                                   VkFormat format, VkImageUsageFlags usage) {
  m_Format = format;

  VkImageCreateInfo imageInfo = CreateImageCreateInfo(
      width, height, format, VK_IMAGE_TILING_OPTIMAL,
      usage | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
      6, 1);
  imageInfo.flags = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;

  AQUILA_VULKAN_CHECK(
      vkCreateImage(m_Device.GetDevice(), &imageInfo, nullptr, &m_Image));

  AllocateAndBindImageMemory(m_Image, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                             m_Memory);

  TransitionImageLayout(m_Image, VK_FORMAT_R32G32B32A32_SFLOAT,
                        VK_IMAGE_LAYOUT_UNDEFINED,
                        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, 6);

  SetDebugName(VK_OBJECT_TYPE_IMAGE, m_Image, m_DebugName + "_Cubemap_Image");
}

void Texture2D::CreateCubemapImageView(VkFormat format) {
  m_ImageView = CreateImageView(m_Image, format, VK_IMAGE_VIEW_TYPE_CUBE, 6, 1);
  SetDebugName(VK_OBJECT_TYPE_IMAGE_VIEW, m_ImageView,
               m_DebugName + "_Cubemap_ImageView");
}

void Texture2D::CreateMipMappedCubemapImage(uint32_t width, uint32_t height,
                                            VkFormat format) {
  VkImageCreateInfo imageInfo = CreateImageCreateInfo(
      width, height, VK_FORMAT_R32G32B32A32_SFLOAT, VK_IMAGE_TILING_OPTIMAL,
      VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, 6,
      m_MipLevels);
  imageInfo.flags = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;

  AQUILA_VULKAN_CHECK(
      vkCreateImage(m_Device.GetDevice(), &imageInfo, nullptr, &m_Image));

  AllocateAndBindImageMemory(m_Image, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                             m_Memory);
}

void Texture2D::CreateMipMappedCubemapImageView(VkFormat format) {
  m_ImageView = CreateImageView(m_Image, VK_FORMAT_R32G32B32A32_SFLOAT,
                                VK_IMAGE_VIEW_TYPE_CUBE, 6, m_MipLevels);
}

void Texture2D::CreateMipMappedCubemapSampler() {
  VkSamplerCreateInfo samplerInfo = CreateSamplerCreateInfo();
  samplerInfo.minLod = 0.0f;
  samplerInfo.maxLod = static_cast<float>(m_MipLevels);
  samplerInfo.mipLodBias = 0.0f;
  samplerInfo.anisotropyEnable = VK_FALSE;

  AQUILA_VULKAN_CHECK(
      vkCreateSampler(m_Device.GetDevice(), &samplerInfo, nullptr, &m_Sampler));
}

// Solid color texture creation
void Texture2D::CreateSolidColorTexture(glm::vec4 color) {
  constexpr uint32_t SOLID_COLOR_SIZE = 1;
  auto pixelData = CreatePixelData(color, false);

  Buffer stagingBuffer =
      CreateStagingBuffer(sizeof(uint8_t) * 4, "SolidColor_StagingBuffer");
  stagingBuffer.Map();
  stagingBuffer.Write(pixelData.data());
  stagingBuffer.UnMap();

  CreateImage(SOLID_COLOR_SIZE, SOLID_COLOR_SIZE, VK_FORMAT_R8G8B8A8_UNORM,
              VK_IMAGE_TILING_OPTIMAL,
              VK_IMAGE_USAGE_TRANSFER_SRC_BIT |
                  VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
              VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, m_Image, m_Memory);

  TransitionImageLayout(m_Image, VK_FORMAT_R8G8B8A8_UNORM,
                        VK_IMAGE_LAYOUT_UNDEFINED,
                        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

  CopyBufferToImage(stagingBuffer.GetBuffer(), m_Image, SOLID_COLOR_SIZE,
                    SOLID_COLOR_SIZE);

  TransitionImageLayout(m_Image, VK_FORMAT_R8G8B8A8_UNORM,
                        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

  CreateTextureImageView(VK_FORMAT_R8G8B8A8_UNORM);
  CreateTextureSampler();
}

void Texture2D::CreateSolidColorCubemap(glm::vec4 color) {
  constexpr uint32_t CUBEMAP_SIZE = 1;
  constexpr uint32_t FACES = 6;
  auto pixelData = CreatePixelData(color, false);

  VkDeviceSize imageSize = sizeof(uint8_t) * 4 * FACES;
  Buffer stagingBuffer =
      CreateStagingBuffer(imageSize, "SolidCubemap_StagingBuffer");

  stagingBuffer.Map();
  for (int i = 0; i < FACES; ++i) {
    stagingBuffer.Write(pixelData.data());
  }
  stagingBuffer.UnMap();

  CreateCubemapImage(CUBEMAP_SIZE, CUBEMAP_SIZE, VK_FORMAT_R32G32B32A32_SFLOAT,
                     VK_IMAGE_USAGE_TRANSFER_DST_BIT |
                         VK_IMAGE_USAGE_SAMPLED_BIT);

  TransitionImageLayout(m_Image, VK_FORMAT_R32G32B32A32_SFLOAT,
                        VK_IMAGE_LAYOUT_UNDEFINED,
                        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 6);

  CopyBufferToImage(stagingBuffer.GetBuffer(), m_Image, CUBEMAP_SIZE,
                    CUBEMAP_SIZE);

  TransitionImageLayout(m_Image, VK_FORMAT_R32G32B32A32_SFLOAT,
                        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, 6);

  CreateCubemapImageView(VK_FORMAT_R32G32B32A32_SFLOAT);
  CreateTextureSampler();
}

std::array<uint8_t, 4> Texture2D::CreatePixelData(glm::vec4 color,
                                                  bool isHDR) const {
  if (isHDR) {
    // For HDR, we'd need a different approach with float data
    throw std::runtime_error(
        "HDR pixel data creation not implemented in this helper");
  }

  return {static_cast<uint8_t>(color.r * 255.0f),
          static_cast<uint8_t>(color.g * 255.0f),
          static_cast<uint8_t>(color.b * 255.0f),
          static_cast<uint8_t>(color.a * 255.0f)};
}

} // namespace Engine