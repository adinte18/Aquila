#include "Engine/SynchronizationManager.h"

namespace Engine {
SynchronizationManager::SynchronizationManager(Device &device,
                                               uint32_t framesInFlight)
    : m_Device(device), m_FramesInFlight(framesInFlight) {}

SynchronizationManager::~SynchronizationManager() {
  for (auto &[name, semaphoreSet] : m_Semaphores) {
    for (auto semaphore : semaphoreSet.semaphores) {
      if (semaphore != VK_NULL_HANDLE) {
        vkDestroySemaphore(m_Device.GetDevice(), semaphore, nullptr);
      }
    }
  }

  for (auto &[name, fenceSet] : m_Fences) {
    for (auto fence : fenceSet.fences) {
      if (fence != VK_NULL_HANDLE) {
        vkDestroyFence(m_Device.GetDevice(), fence, nullptr);
      }
    }
  }
}

void SynchronizationManager::CreateSemaphore(const std::string &name) {
  if (m_Semaphores.find(name) != m_Semaphores.end()) {
    return; // already exists
  }

  SemaphoreSet semaphoreSet;
  semaphoreSet.semaphores.resize(m_FramesInFlight);

  VkSemaphoreCreateInfo semInfo{VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO};
  for (uint32_t i = 0; i < m_FramesInFlight; ++i) {
    if (vkCreateSemaphore(m_Device.GetDevice(), &semInfo, nullptr,
                          &semaphoreSet.semaphores[i]) != VK_SUCCESS) {
      throw std::runtime_error("Failed to create semaphore: " + name + "_" +
                               std::to_string(i));
    }
  }

  m_Semaphores[name] = std::move(semaphoreSet);
}

VkSemaphore SynchronizationManager::GetSemaphore(const std::string &name,
                                                 uint32_t frameIndex) {
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

void SynchronizationManager::CreateFence(const std::string &name,
                                         bool startSignaled) {
  if (m_Fences.find(name) != m_Fences.end()) {
    return; // already exists
  }

  FenceSet fenceSet;
  fenceSet.fences.resize(m_FramesInFlight);

  VkFenceCreateInfo fenceInfo{};
  fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
  if (startSignaled) {
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
  }

  for (uint32_t i = 0; i < m_FramesInFlight; ++i) {
    if (vkCreateFence(m_Device.GetDevice(), &fenceInfo, nullptr,
                      &fenceSet.fences[i]) != VK_SUCCESS) {
      throw std::runtime_error("Failed to create fence: " + name + "_" +
                               std::to_string(i));
    }
  }

  m_Fences[name] = std::move(fenceSet);
}

VkFence SynchronizationManager::GetFence(const std::string &name,
                                         uint32_t frameIndex) {
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

bool SynchronizationManager::IsFenceSignaled(const std::string &name,
                                             uint32_t frameIndex) {
  VkFence fence = GetFence(name, frameIndex);
  return vkGetFenceStatus(m_Device.GetDevice(), fence) == VK_SUCCESS;
}

void SynchronizationManager::WaitForFence(const std::string &name,
                                          uint32_t frameIndex) {
  VkFence fence = GetFence(name, frameIndex);
  vkWaitForFences(m_Device.GetDevice(), 1, &fence, VK_TRUE, UINT64_MAX);
}

void SynchronizationManager::ResetFence(const std::string &name,
                                        uint32_t frameIndex) {
  VkFence fence = GetFence(name, frameIndex);
  vkResetFences(m_Device.GetDevice(), 1, &fence);
}

} // namespace Engine