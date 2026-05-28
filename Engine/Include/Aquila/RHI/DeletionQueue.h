#pragma once
#include "GraphicsPCH.h"
#include "Aquila/Foundation/Defines.h"
#include "Aquila/Foundation/PrimitiveTypes.h"
#include "Aquila/Foundation/SharedConstants.h"

namespace Aquila::RHI {

class VulkanDevice;

namespace Deletion {
struct VmaImageDeletion {
	VkImage image;
	VmaAllocation allocation;
};
struct VmaBufferDeletion {
	VkBuffer buffer;
	VmaAllocation allocation;
};

using ResourceVariant =
	std::variant<VkPipeline, VkShaderModule, VkPipelineLayout, VkPipelineCache, VkImageView, VkDeviceMemory, VkSampler,
				 VkDescriptorSetLayout, VkRenderPass, VkFramebuffer, VkSemaphore, VmaImageDeletion, VmaBufferDeletion>;
} // namespace Deletion

class DeletionQueue {
  public:
	explicit DeletionQueue(VulkanDevice &device);
	~DeletionQueue();

	AQUILA_NONCOPYABLE(DeletionQueue);

	void QueueDeletion(const Deletion::ResourceVariant &resource);

	void DestroyNow(const Deletion::ResourceVariant &resource);

	void SetCurrentSlot(uint32 slot);

	void Flush(uint32 slot);

	void FlushAll();

  private:
	void Dispatch(const Deletion::ResourceVariant &resource);

	VulkanDevice &m_Device;
	uint32 m_CurrentSlot = 0;
	std::array<std::vector<Deletion::ResourceVariant>, SharedConstants::MAX_FRAMES_IN_FLIGHT> m_Buckets;
};

} // namespace Aquila::RHI
