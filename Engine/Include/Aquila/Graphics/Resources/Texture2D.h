#ifndef AQUILA_TEXTURE_H
#define AQUILA_TEXTURE_H

#include "Aquila/Graphics/Core/Device.h"
#include "Aquila/Graphics/Pipeline/Descriptor.h"
#include "Aquila/Foundation/PrimitiveTypes.h"
#include "Aquila/Graphics/Core/DeletionManager.h"
#include "Aquila/Graphics/Resources/Buffer.h"

namespace Aquila::Graphics::Texture {
class TextureLoader;
class ImageOperations;
} // namespace Aquila::Graphics::Texture

namespace Aquila::Graphics::Resources {

using namespace Aquila::Foundation;

enum class TextureType : uint8 { Albedo, Normal, Emissive, MetallicRoughness, AO, Cubemap };
enum class SourceType : uint8 { None, File, HDRFile, RenderTarget, Cubemap, MipmappedCubemap, Fallback };

class Texture2D {
  public:
	Texture2D(Device &device, std::string debugName = "Texture2D");
	~Texture2D();

	AQUILA_NONCOPYABLE(Texture2D);
	void UploadToGPU(VkCommandBuffer cmd);
	void FinalizeDescriptors();

	class Builder {
	  public:
		explicit Builder(Device &device, const std::string &debugName = "Texture2D");

		Builder &FromFile(const std::string &filepath, VkFormat format = VK_FORMAT_R8G8B8A8_UNORM);
		Builder &FromHDRFile(const std::string &filepath);
		Builder &AsRenderTarget(uint32 width, uint32 height, VkFormat format, VkImageUsageFlags usage);
		Builder &AsCubemap(uint32 width, uint32 height, VkFormat format, VkImageUsageFlags usage);
		Builder &AsMipmappedCubemap(uint32 width, uint32 height, VkFormat format, VkImageUsageFlags usage);
		Builder &AsEmpty(uint32 width, uint32 height, VkFormat format, VkImageUsageFlags usage);
		Builder &AsFallback(TextureType type);
		Builder &DeferGPUUpload();

		Builder &WithSamples(VkSampleCountFlagBits samples);
		Builder &WithDeletionQueue(DeletionManager *queue);

		Ref<Texture2D> Build();

	  private:
		Device &m_Device;
		std::string m_DebugName;

		SourceType m_SourceType = SourceType::None;
		std::string m_Filepath;
		VkFormat m_Format = VK_FORMAT_R8G8B8A8_UNORM;
		uint32 m_Width = 0;
		uint32 m_Height = 0;
		VkImageUsageFlags m_Usage = 0;
		VkSampleCountFlagBits m_Samples = VK_SAMPLE_COUNT_1_BIT;
		TextureType m_FallbackType = TextureType::Albedo;
		bool m_DeferUpload = false;
	};

	void CreateRenderTarget(uint32 width, uint32 height, VkFormat format, VkImageUsageFlags usage,
							VkSampleCountFlagBits samples = VK_SAMPLE_COUNT_1_BIT);
	void CreateCubemap(uint32 width, uint32 height, VkFormat format, VkImageUsageFlags usage);
	void CreateMipmappedCubemap(uint32 width, uint32 height, VkFormat format, VkImageUsageFlags usage);
	void CreateFallback(TextureType type);
	void CleanupStagingResources();
	[[nodiscard]] VkImageView GetImageView() const { return m_ImageView; }
	[[nodiscard]] VkSampler GetSampler() const { return m_Sampler; }
	[[nodiscard]] VkImage GetImage() const { return m_Image; }
	VkDescriptorSet GetDescriptorSet() {
		if ((m_DescriptorSetUpdateNeeded || m_DescriptorSet == VK_NULL_HANDLE) && m_ImageView != VK_NULL_HANDLE) {
			FinalizeDescriptors();
		}
		return m_DescriptorSet;
	}
	[[nodiscard]] uint32 GetMipLevels() const { return m_MipLevels; }
	[[nodiscard]] VkFormat GetFormat() const { return m_Format; }
	[[nodiscard]] VkDeviceMemory GetMemory() const { return m_Memory; }
	const std::string &GetPath() { return m_Filepath; }
	[[nodiscard]] uint32 GetWidth() const { return m_Width; }
	[[nodiscard]] uint32 GetHeight() const { return m_Height; }
	[[nodiscard]] const void *GetData() const {
		return std::visit(
			[](auto &&pixels) -> const void * {
				using T = std::decay_t<decltype(pixels)>;
				if constexpr (std::is_same_v<T, std::monostate>) {
					return nullptr;
				} else {
					return pixels.get();
				}
			},
			m_Pixels);
	}
	[[nodiscard]] VkDescriptorImageInfo GetDescriptorImageInfo() const;

	void Destroy();

  private:
	void InitializeHandles();
	void DestroyVulkanResources();

	void CreateDescriptorSetLayout();
	void AllocateDescriptorSet();
	void UpdateDescriptorSet();
	void ReleaseDescriptorResources();

	void LoadPixelDataFromFile(const std::string &filepath, VkFormat format);
	void LoadPixelDataFromHDR(const std::string &filepath);

	Device &m_Device;
	std::string m_DebugName;
	std::string m_Filepath;

	VkImage m_Image = VK_NULL_HANDLE;
	VkImageView m_ImageView = VK_NULL_HANDLE;
	VkSampler m_Sampler = VK_NULL_HANDLE;
	VkDeviceMemory m_Memory = VK_NULL_HANDLE;
	VkFormat m_Format = VK_FORMAT_UNDEFINED;
	uint32 m_MipLevels = 1;
	uint32 m_Width = 0;
	uint32 m_Height = 0;
	std::variant<std::monostate, std::unique_ptr<uint8[]>, std::unique_ptr<f32[]>> m_Pixels;

	VkDeviceSize m_ImageSize = 0;
	bool m_IsHDR = false;

	Unique<RenderingPipeline::DescriptorSetLayout> m_DescriptorSetLayout;
	VkDescriptorSet m_DescriptorSet = VK_NULL_HANDLE;
	Unique<Resources::Buffer> m_StagingBuffer = nullptr;
	bool m_IsRenderTarget = false;
	bool m_DescriptorSetUpdateNeeded = false;
};

} // namespace Aquila::Graphics::Resources
#endif // AQUILA_TEXTURE_H
