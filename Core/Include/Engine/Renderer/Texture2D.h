#ifndef TEXTURE2D_H
#define TEXTURE2D_H

#include "Engine/Renderer/Buffer.h"
#include "Engine/Renderer/Descriptor.h"
#include "Engine/Renderer/DescriptorAllocator.h"
#include "Engine/Renderer/Device.h"

namespace Engine {

class Texture2D {
public:
  class Builder {
  public:
    explicit Builder(Device &device) : device(device) {}

    Builder &setSize(uint32_t width, uint32_t height) {
      this->width = width;
      this->height = height;
      return *this;
    }

    Builder &setFormat(VkFormat format) {
      this->format = format;
      return *this;
    }

    Builder &setUsage(VkImageUsageFlags usage) {
      this->usage = usage;
      return *this;
    }

    Builder &setSamples(VkSampleCountFlagBits samples) {
      this->samples = samples;
      return *this;
    }

    Builder &setFilepath(const std::string &filepath) {
      this->filepath = filepath;
      return *this;
    }

    Builder &setDebugName(const std::string &debugName) {
      this->debugName = debugName;
      return *this;
    }

    Builder &asCubeMap() {
      useCubemap = true;
      return *this;
    }

    Builder &asHDR() {
      useHDR = true;
      return *this;
    }

    [[nodiscard]] Ref<Texture2D> build() const {
      if (useCubemap && useHDR) {
        throw std::runtime_error(
            "Cannot build a texture as both HDR and Cubemap. Choose only one.");
      }

      auto texture = CreateRef<Texture2D>(device, debugName);

      if (!filepath.empty()) {
        if (useHDR) {
          texture->CreateHDRTexture(filepath, format);
        } else {
          texture->CreateTexture(filepath, format);
        }
      } else {
        if (useCubemap) {
          texture->CreateCubeMap(width, height, format, usage);
        } else {
          texture->CreateTexture(width, height, format, usage, samples);
        }
      }

      return texture;
    }

  private:
    bool useCubemap = false;
    bool useHDR = false;
    Device &device;
    uint32_t width = 512;
    uint32_t height = 512;
    VkFormat format = VK_FORMAT_R8G8B8A8_UNORM;
    VkImageUsageFlags usage = VK_IMAGE_USAGE_TRANSFER_SRC_BIT |
                              VK_IMAGE_USAGE_TRANSFER_DST_BIT |
                              VK_IMAGE_USAGE_SAMPLED_BIT;
    VkSampleCountFlagBits samples = VK_SAMPLE_COUNT_1_BIT;
    std::string filepath;
    std::string debugName;
  };

  enum class TextureType {
    Albedo,
    Normal,
    Emissive,
    AO,
    MetallicRoughness,
    Cubemap
  };

public:
  // Factory method
  [[nodiscard]] static Ref<Texture2D>
  create(Device &device, const std::string &debugName = "") {
    return CreateRef<Texture2D>(device, debugName);
  }

  // Constructor/Destructor
  Texture2D(Device &device, const std::string &debugName);
  ~Texture2D();

  AQUILA_NONCOPYABLE(Texture2D);
  AQUILA_NONMOVEABLE(Texture2D);

  // Public API methods
  void CreateHDRTexture(const std::string &filepath,
                        VkFormat format = VK_FORMAT_R32G32B32A32_SFLOAT);
  void CreateCubeMap(uint32_t width, uint32_t height, VkFormat format,
                     VkImageUsageFlags usage);
  void CreateMipMappedCubemap(uint32_t width, uint32_t height, VkFormat format,
                              VkImageUsageFlags usage);
  void CreateTexture(const std::string &filepath,
                     VkFormat format = VK_FORMAT_R8G8B8A8_UNORM);
  void CreateTexture(uint32_t width, uint32_t height,
                     VkFormat format = VK_FORMAT_R8G8B8A8_UNORM,
                     VkImageUsageFlags usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT |
                                               VK_IMAGE_USAGE_SAMPLED_BIT,
                     VkSampleCountFlagBits samples = VK_SAMPLE_COUNT_1_BIT);
  void UseFallbackTextures(TextureType type);

  // Utility methods
  void GenerateMipmap(VkImage image, VkFormat format, int32_t width,
                      int32_t height, uint32_t mipLevels);
  void CreateImage(uint32_t width, uint32_t height, VkFormat format,
                   VkImageTiling tiling, VkImageUsageFlags usage,
                   VkMemoryPropertyFlags properties, VkImage &image,
                   VkDeviceMemory &imageMemory);

  // Getters
  [[nodiscard]] VkImage GetTextureImage() const { return m_Image; }
  [[nodiscard]] VkImageView GetTextureImageView() const { return m_ImageView; }
  [[nodiscard]] VkSampler GetTextureSampler() const { return m_Sampler; }
  [[nodiscard]] VkDeviceMemory GetTextureImageMemory() const {
    return m_Memory;
  }
  [[nodiscard]] VkDescriptorSet GetDescriptorSet() const {
    return descriptorSet;
  }
  [[nodiscard]] uint32_t GetMipLevels() const { return m_MipLevels; }
  [[nodiscard]] VkFormat GetFormat() const { return m_Format; }
  [[nodiscard]] VkDescriptorImageInfo GetDescriptorSetInfo() const;

  // State queries
  [[nodiscard]] bool HasImage() const { return m_Image != VK_NULL_HANDLE; }
  [[nodiscard]] bool HasImageView() const {
    return m_ImageView != VK_NULL_HANDLE;
  }
  [[nodiscard]] bool HasSampler() const { return m_Sampler != VK_NULL_HANDLE; }
  [[nodiscard]] bool HasImageMemory() const {
    return m_Memory != VK_NULL_HANDLE;
  }

  // Resource management
  void MarkForDestruction() { isMarkedForDestruction = true; }
  bool isMarkedForDestruction = false;
  void DestroyAll();
  void Destroy();

private:
  // Member variables
  std::string m_DebugName;
  uint32_t m_MipLevels = 1;
  Device &m_Device;
  VkFormat m_Format = VK_FORMAT_R8G8B8A8_UNORM;

  VkImage m_Image = VK_NULL_HANDLE;
  VkSampler m_Sampler = VK_NULL_HANDLE;
  VkImageView m_ImageView = VK_NULL_HANDLE;
  VkDeviceMemory m_Memory = VK_NULL_HANDLE;

  Unique<DescriptorPool> descriptorPool;
  Unique<DescriptorSetLayout> descriptorSetLayout;
  VkDescriptorSet descriptorSet = VK_NULL_HANDLE;

  // Initialization and cleanup helpers
  void InitializeHandles();
  void CreateDescriptorSetLayout();
  void AllocateDescriptorSet();
  void DestroyVulkanResources();
  void ReleaseDescriptorResources();

  // Common creation helpers
  VkImageCreateInfo CreateImageCreateInfo(uint32_t width, uint32_t height,
                                          VkFormat format, VkImageTiling tiling,
                                          VkImageUsageFlags usage,
                                          uint32_t arrayLayers,
                                          uint32_t mipLevels) const;
  void AllocateAndBindImageMemory(VkImage image,
                                  VkMemoryPropertyFlags properties,
                                  VkDeviceMemory &imageMemory);
  VkImageView CreateImageView(VkImage image, VkFormat format,
                              VkImageViewType viewType, uint32_t layerCount,
                              uint32_t mipLevels) const;
  VkSamplerCreateInfo CreateSamplerCreateInfo() const;
  Buffer CreateStagingBuffer(VkDeviceSize size,
                             const std::string &debugName) const;
  void SetDebugName(VkObjectType objectType, void *handle,
                    const std::string &name) const;

  // Mipmap generation helpers
  void ValidateLinearFilteringSupport(VkFormat imageFormat) const;
  VkImageMemoryBarrier CreateMipmapBarrier(VkImage image) const;
  void TransitionMipLevel(VkCommandBuffer commandBuffer,
                          VkImageMemoryBarrier &barrier, uint32_t mipLevel,
                          VkImageLayout oldLayout, VkImageLayout newLayout,
                          VkAccessFlags srcAccess,
                          VkAccessFlags dstAccess) const;
  void BlitMipLevel(VkCommandBuffer commandBuffer, VkImage image,
                    uint32_t srcMip, uint32_t dstMip, int32_t srcWidth,
                    int32_t srcHeight) const;
  void CalculateNextMipDimensions(int32_t &width, int32_t &height) const;
  void CalculateMipLevels(uint32_t width, uint32_t height);

  // Layout transition helpers
  void TransitionImageLayout(VkImage image, VkFormat format,
                             VkImageLayout oldLayout, VkImageLayout newLayout,
                             uint32_t layers = 1);
  VkImageMemoryBarrier CreateLayoutTransitionBarrier(VkImage image,
                                                     VkFormat format,
                                                     VkImageLayout oldLayout,
                                                     VkImageLayout newLayout,
                                                     uint32_t layers) const;
  std::pair<VkAccessFlags, VkAccessFlags>
  GetAccessMasks(VkImageLayout oldLayout, VkImageLayout newLayout) const;
  std::pair<VkPipelineStageFlags, VkPipelineStageFlags>
  GetPipelineStages(VkImageLayout oldLayout, VkImageLayout newLayout) const;

  // Buffer to image operations
  void CopyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width,
                         uint32_t height) const;
  void WriteToDescriptorSet();

  // Specific texture creation methods
  void CreateHDRTextureImage(const std::string &filepath);
  void CreateTextureImageFromFile(const std::string &filepath);
  void CreateTextureImage(uint32_t width, uint32_t height, VkFormat format,
                          VkImageUsageFlags usage,
                          VkSampleCountFlagBits samples);
  void CreateTextureImageView(VkFormat format);
  void CreateTextureSampler();

  // Cubemap specific methods
  void CreateCubemapImage(uint32_t width, uint32_t height, VkFormat format,
                          VkImageUsageFlags usage);
  void CreateCubemapImageView(VkFormat format);
  void CreateMipMappedCubemapImage(uint32_t width, uint32_t height,
                                   VkFormat format);
  void CreateMipMappedCubemapImageView(VkFormat format);
  void CreateMipMappedCubemapSampler();

  // Solid color texture helpers
  void CreateSolidColorTexture(glm::vec4 color);
  void CreateSolidColorCubemap(glm::vec4 color);
  std::array<uint8_t, 4> CreatePixelData(glm::vec4 color,
                                         bool isHDR = false) const;

  // Image manipulation helpers
  std::unique_ptr<float[]> FlipImageVertically(float *pixels, int width,
                                               int height, int channels) const;
};

} // namespace Engine

#endif // TEXTURE2D_H