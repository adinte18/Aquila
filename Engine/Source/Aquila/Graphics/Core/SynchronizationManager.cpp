#include "Aquila/Graphics/Core/SynchronizationManager.h"

namespace Aquila::Graphics {
SynchronizationManager::SynchronizationManager(Device &device, uint32 framesInFlight)
	: m_Device(device), m_FramesInFlight(framesInFlight) {}

SynchronizationManager::~SynchronizationManager() {
	for (auto &[semaphores] : m_Semaphores | std::views::values) {
		for (const auto semaphore : semaphores) {
			if (semaphore != VK_NULL_HANDLE) {
				vkDestroySemaphore(m_Device.GetDevice(), semaphore, nullptr);
			}
		}
	}

	for (auto &[fences] : m_Fences | std::views::values) {
		for (const auto fence : fences) {
			if (fence != VK_NULL_HANDLE) {
				vkDestroyFence(m_Device.GetDevice(), fence, nullptr);
			}
		}
	}
}

void SynchronizationManager::CreateSemaphore(const std::string &name) {
	if (m_Semaphores.contains(name)) {
		return; // already exists
	}

	SemaphoreSet semaphoreSet;
	semaphoreSet.semaphores.resize(m_FramesInFlight);

	constexpr VkSemaphoreCreateInfo semInfo{ VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO };
	for (uint32 i = 0; i < m_FramesInFlight; ++i) {
		AQUILA_VULKAN_CHECK(vkCreateSemaphore(m_Device.GetDevice(), &semInfo, nullptr, &semaphoreSet.semaphores[i]));
	}

	m_Semaphores[name] = std::move(semaphoreSet);
}

VkSemaphore SynchronizationManager::GetSemaphore(const std::string &name, const uint32 frameIndex) {
	auto it = m_Semaphores.find(name);
	if (it == m_Semaphores.end()) {
		CreateSemaphore(name);
		it = m_Semaphores.find(name);
	}

	if (frameIndex >= m_FramesInFlight) {
		throw std::runtime_error("Invalid frame index for semaphore: " + name);
	}

	return it->second.semaphores[frameIndex];
}

void SynchronizationManager::CreateFence(const std::string &name, const bool startSignaled) {
	if (m_Fences.contains(name)) {
		return; // already exists
	}

	FenceSet fenceSet;
	fenceSet.fences.resize(m_FramesInFlight);

	VkFenceCreateInfo fenceInfo{};
	fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	if (startSignaled) {
		fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
	}

	for (uint32 i = 0; i < m_FramesInFlight; ++i) {
		AQUILA_VULKAN_CHECK(vkCreateFence(m_Device.GetDevice(), &fenceInfo, nullptr, &fenceSet.fences[i]));
	}

	m_Fences[name] = std::move(fenceSet);
}

VkFence SynchronizationManager::GetFence(const std::string &name, const uint32 frameIndex) {
	auto it = m_Fences.find(name);
	if (it == m_Fences.end()) {
		CreateFence(name);
		it = m_Fences.find(name);
	}

	if (frameIndex >= m_FramesInFlight) {
		throw std::runtime_error("Invalid frame index for fence: " + name);
	}

	return it->second.fences[frameIndex];
}

bool SynchronizationManager::IsFenceSignaled(const std::string &name, const uint32 frameIndex) {
	const VkFence fence = GetFence(name, frameIndex);
	return vkGetFenceStatus(m_Device.GetDevice(), fence) == VK_SUCCESS;
}

void SynchronizationManager::WaitForFence(const std::string &name, const uint32 frameIndex) {
	const VkFence fence = GetFence(name, frameIndex);
	vkWaitForFences(m_Device.GetDevice(), 1, &fence, VK_TRUE, UINT64_MAX);
}

void SynchronizationManager::ResetFence(const std::string &name, const uint32 frameIndex) {
	const VkFence fence = GetFence(name, frameIndex);
	vkResetFences(m_Device.GetDevice(), 1, &fence);
}

} // namespace Aquila::Graphics