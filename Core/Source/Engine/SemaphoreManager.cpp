#include "Engine/SemaphoreManager.h"

namespace Engine {
    SemaphoreManager::SemaphoreManager(Device& device, uint32_t framesInFlight)
        : m_Device(device), m_FramesInFlight(framesInFlight) {
    }

    SemaphoreManager::~SemaphoreManager() {
        for (auto& [name, semaphoreSet] : m_Semaphores) {
            for (auto semaphore : semaphoreSet.semaphores) {
                if (semaphore != VK_NULL_HANDLE) {
                    vkDestroySemaphore(m_Device.GetDevice(), semaphore, nullptr);
                }
            }
        }
    }

    void SemaphoreManager::CreateSemaphore(const std::string& name) {
        if (m_Semaphores.find(name) != m_Semaphores.end()) {
            return; // already exists
        }
        
        SemaphoreSet semaphoreSet;
        semaphoreSet.semaphores.resize(m_FramesInFlight);
        
        VkSemaphoreCreateInfo semInfo{VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO};
        for (uint32_t i = 0; i < m_FramesInFlight; ++i) {
            if (vkCreateSemaphore(m_Device.GetDevice(), &semInfo, nullptr, &semaphoreSet.semaphores[i]) != VK_SUCCESS) {
                throw std::runtime_error("Failed to create semaphore: " + name + "_" + std::to_string(i));
            }
        }
        
        m_Semaphores[name] = std::move(semaphoreSet);
    }

    VkSemaphore SemaphoreManager::GetSemaphore(const std::string& name, uint32_t frameIndex) {
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
}