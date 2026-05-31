#ifndef AQUILA_VULKAN_TEXTURE_H
#define AQUILA_VULKAN_TEXTURE_H

#include "GraphicsPCH.h"
#include "Aquila/Foundation/PrimitiveTypes.h"
#include "Aquila/RHI/Backend/IRHITexture.h"
#include "Aquila/RHI/Backend/RHITypes.h"
#include "Aquila/RHI/Vulkan/VulkanTypes.h"

namespace Aquila::RHI {

class VulkanDevice;

class VulkanTexture final : public IRHITexture {
  public:
	VulkanTexture(VulkanDevice &device, const TextureDesc &desc);
	~VulkanTexture() override;

	AQUILA_NONCOPYABLE(VulkanTexture);

	void DestroyImmediate() override;
	[[nodiscard]] bool IsReady() const override { return m_ImageView != VK_NULL_HANDLE; }

	// IRHITexture
	[[nodiscard]] uint32 GetWidth() const override { return m_Desc.width; }
	[[nodiscard]] uint32 GetHeight() const override { return m_Desc.height; }
	[[nodiscard]] uint32 GetMipLevels() const override { return m_Desc.mipLevels; }
	[[nodiscard]] uint32 GetArrayLayers() const override { return m_Desc.arrayLayers; }
	[[nodiscard]] TextureFormat GetFormat() const override { return m_Desc.format; }
	[[nodiscard]] SampleCount GetSampleCount() const override { return m_Desc.samples; }
	[[nodiscard]] const TextureDesc &GetDesc() const override { return m_Desc; }

	// Vulkan-specific accessors (used by rendering systems and descriptor writers)
	[[nodiscard]] VkImage GetImage() const { return m_ImageAllocation.image; }
	[[nodiscard]] VkImageView GetImageView() const { return m_ImageView; }
	[[nodiscard]] VkSampler GetSampler() const { return m_Sampler; }
	[[nodiscard]] const ImageAllocation &GetAllocation() const { return m_ImageAllocation; }
	[[nodiscard]] VkDescriptorImageInfo GetDescriptorImageInfo() const;

  private:
	void CreateImageView();
	void CreateSampler();

	VulkanDevice &m_Device;
	TextureDesc m_Desc;
	ImageAllocation m_ImageAllocation{};
	VkImageView m_ImageView = VK_NULL_HANDLE;
	VkSampler m_Sampler = VK_NULL_HANDLE;
};

} // namespace Aquila::RHI
#endif
