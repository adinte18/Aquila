#ifndef SEMAPHORE_MANAGER_H
#define SEMAPHORE_MANAGER_H

#include "Engine/Renderer/Device.h"


namespace Engine {


    class SemaphoreManager {
    public:
        SemaphoreManager(Device& device, uint32_t framesInFlight);
        ~SemaphoreManager();

        VkSemaphore GetSemaphore(const std::string& name, uint32_t frameIndex);
        void CreateSemaphore(const std::string& name);

    private:
        Device& m_Device;
        uint32_t m_FramesInFlight;
        
        struct SemaphoreSet {
            std::vector<VkSemaphore> semaphores;
        };
        
        std::unordered_map<std::string, SemaphoreSet> m_Semaphores;
    };
}

#endif // SEMAPHORE_MANAGER_H