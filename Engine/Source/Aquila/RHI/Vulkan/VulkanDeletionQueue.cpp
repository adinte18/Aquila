#include "Aquila/RHI/Vulkan/VulkanDeletionQueue.h"
#include "Aquila/RHI/Vulkan/VulkanDevice.h"

namespace Aquila::RHI {

DeletionQueue::DeletionQueue(VulkanDevice &device) : m_Device(device) {}

DeletionQueue::~DeletionQueue() {
	Flush();
}

void DeletionQueue::QueueDeletion(const Deletion::ResourceVariant &resource) {
	m_PendingDeletions.push({ resource });
}

void DeletionQueue::DestroyNow(const Deletion::ResourceVariant &resource) {
	Dispatch(resource);
}

void DeletionQueue::ProcessDeletions() {
	while (!m_PendingDeletions.empty()) {
		Dispatch(m_PendingDeletions.front().resource);
		m_PendingDeletions.pop();
	}
}

void DeletionQueue::Flush() {
	AQUILA_ASSERT(m_Device.GetDevice() != VK_NULL_HANDLE, "DeletionQueue::Flush — device is null");

	m_Device.Wait();

	while (!m_PendingDeletions.empty()) {
		Dispatch(m_PendingDeletions.front().resource);
		m_PendingDeletions.pop();
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
