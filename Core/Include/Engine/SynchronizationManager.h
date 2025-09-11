#ifndef SEMAPHORE_MANAGER_H
#define SEMAPHORE_MANAGER_H

#include "Engine/Renderer/Device.h"


namespace Engine {


    class SynchronizationManager {
    public:
        SynchronizationManager(Device& device, uint32_t framesInFlight);
        ~SynchronizationManager();

        VkSemaphore GetSemaphore(const std::string& name, uint32_t frameIndex);
        void CreateSemaphore(const std::string& name);

        void CreateFence(const std::string& name, bool startSignaled = true);
        VkFence GetFence(const std::string& name, uint32_t frameIndex);
        bool IsFenceSignaled(const std::string& name, uint32_t frameIndex);
        void WaitForFence(const std::string& name, uint32_t frameIndex);
        void ResetFence(const std::string& name, uint32_t frameIndex);


    private:
        struct SemaphoreSet {
            std::vector<VkSemaphore> semaphores;
        };
        
        struct FenceSet {
            std::vector<VkFence> fences;
        };

        Device& m_Device;
        uint32_t m_FramesInFlight;
        
        std::unordered_map<std::string, SemaphoreSet> m_Semaphores;
        std::unordered_map<std::string, FenceSet> m_Fences;
    };
}

#endif // SEMAPHORE_MANAGER_H