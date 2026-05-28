#include "Aquila/RHI/Vulkan/VulkanDeletionQueue.h"
#include "Aquila/RHI/Vulkan/VulkanDevice.h"

namespace Aquila::RHI {

DeletionQueue::DeletionQueue(VulkanDevice &device) : m_Device(device) {}

DeletionQueue::~DeletionQueue() {
	FlushAll();
}

void DeletionQueue::SetCurrentSlot(uint32 slot) {
	AQUILA_ASSERT(slot < SharedConstants::MAX_FRAMES_IN_FLIGHT, "DeletionQueue slot out of range");
	m_CurrentSlot = slot;
}

void DeletionQueue::QueueDeletion(const Deletion::ResourceVariant &resource) {
	m_Buckets[m_CurrentSlot].push_back(resource);
}

void DeletionQueue::DestroyNow(const Deletion::ResourceVariant &resource) {
	Dispatch(resource);
}

void DeletionQueue::Flush(uint32 slot) {
	AQUILA_ASSERT(slot < SharedConstants::MAX_FRAMES_IN_FLIGHT, "DeletionQueue slot out of range");
	for (auto &resource : m_Buckets[slot]) {
		Dispatch(resource);
	}
	m_Buckets[slot].clear();
}

void DeletionQueue::FlushAll() {
	AQUILA_ASSERT(m_Device.GetDevice() != VK_NULL_HANDLE, "DeletionQueue::FlushAll — device is null");
	m_Device.Wait();
	for (uint32 i = 0; i < SharedConstants::MAX_FRAMES_IN_FLIGHT; ++i) {
		Flush(i);
	}
}

void DeletionQueue::Dispatch(const Deletion::ResourceVariant &resource) {
	VkDevice device = m_Device.GetDevice();

	std::visit(
		[&](auto &&res) {
			using T = std::decay_t<decltype(res)>;

			if constexpr (std::is_same_v<T, VkPipeline>) {
				vkDestroyPipeline(device, res, nullptr);
			} else if constexpr (std::is_same_v<T, VkShaderModule>) {
				vkDestroyShaderModule(device, res, nullptr);
			} else if constexpr (std::is_same_v<T, VkPipelineLayout>) {
				vkDestroyPipelineLayout(device, res, nullptr);
			} else if constexpr (std::is_same_v<T, VkPipelineCache>) {
				vkDestroyPipelineCache(device, res, nullptr);
			} else if constexpr (std::is_same_v<T, VkImageView>) {
				vkDestroyImageView(device, res, nullptr);
			} else if constexpr (std::is_same_v<T, VkDeviceMemory>) {
				vkFreeMemory(device, res, nullptr);
			} else if constexpr (std::is_same_v<T, VkSampler>) {
				vkDestroySampler(device, res, nullptr);
			} else if constexpr (std::is_same_v<T, VkDescriptorSetLayout>) {
				vkDestroyDescriptorSetLayout(device, res, nullptr);
			} else if constexpr (std::is_same_v<T, VkRenderPass>) {
				vkDestroyRenderPass(device, res, nullptr);
			} else if constexpr (std::is_same_v<T, VkFramebuffer>) {
				vkDestroyFramebuffer(device, res, nullptr);
			} else if constexpr (std::is_same_v<T, VkSemaphore>) {
				vkDestroySemaphore(device, res, nullptr);
			} else if constexpr (std::is_same_v<T, Deletion::VmaImageDeletion>) {
				vmaDestroyImage(m_Device.GetAllocator(), res.image, res.allocation);
			} else if constexpr (std::is_same_v<T, Deletion::VmaBufferDeletion>) {
				vmaDestroyBuffer(m_Device.GetAllocator(), res.buffer, res.allocation);
			} else {
				AQUILA_ASSERT(false, "DeletionQueue::Dispatch — unhandled resource type");
			}
		},
		resource);
}

} // namespace Aquila::RHI
