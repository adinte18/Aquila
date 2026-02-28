#include "Aquila/Utilities/DeletionManager.h"
#include "Aquila/Core/Defines.h"
#include "Aquila/Graphics/Core/Device.h"
#include "Aquila/Graphics/Pipeline/DescriptorAllocator.h"

namespace Aquila::Utils {

DeletionManager::DeletionManager(Device &device) : m_Device(device) {}

DeletionManager::~DeletionManager() {
	Flush();
}

void DeletionManager::QueueDeletion(const DeferredDeletion::ResourceVariant &resource) {
	m_DeletionQueue.push({ resource });
}

void DeletionManager::DestroyNow(const DeferredDeletion::ResourceVariant &resource) {
	Dispatch(resource);
}

void DeletionManager::ProcessDeletions() {
	while (!m_DeletionQueue.empty()) {
		Dispatch(m_DeletionQueue.front().resource);
		m_DeletionQueue.pop();
	}
}

void DeletionManager::Flush() {
	AQUILA_ASSERT(m_Device.GetDevice() != VK_NULL_HANDLE, "DeletionManager::Flush — device is null");

	m_Device.Wait();

	while (!m_DeletionQueue.empty()) {
		Dispatch(m_DeletionQueue.front().resource);
		m_DeletionQueue.pop();
	}
}

void DeletionManager::Dispatch(const DeferredDeletion::ResourceVariant &resource) {
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
			} else if constexpr (std::is_same_v<T, VkImage>) {
				vkDestroyImage(device, res, nullptr);
			} else if constexpr (std::is_same_v<T, VkImageView>) {
				vkDestroyImageView(device, res, nullptr);
			} else if constexpr (std::is_same_v<T, VkBuffer>) {
				vkDestroyBuffer(device, res, nullptr);
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
			} else {
				AQUILA_ASSERT(false, "DeletionManager::Dispatch — unhandled resource type");
			}
		},
		resource);
}

} // namespace Aquila::Utils
