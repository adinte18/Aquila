#pragma once
#include "GraphicsPCH.h"
#include "Aquila/Foundation/Defines.h"
#include "Aquila/Foundation/PrimitiveTypes.h"

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
	void ProcessDeletions();
	void Flush();

  private:
	void Dispatch(const Deletion::ResourceVariant &resource);

	VulkanDevice &m_Device;

	struct PendingDeletion {
		Deletion::ResourceVariant resource;
	};
	std::queue<PendingDeletion> m_PendingDeletions;
};

} // namespace Aquila::RHI
