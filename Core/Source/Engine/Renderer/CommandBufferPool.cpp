#include "Engine/Renderer/CommandBufferPool.h"
#include "Platform/PrimitiveTypes.h"

namespace Engine {
    CommandBufferPool::CommandBufferPool(Device& device, uint32 framesInFlight)
        : m_Device(device), m_FramesInFlight(framesInFlight) {
        
        m_Pool = m_Device.GetCommandPool();
        m_FrameCommandBuffers.resize(framesInFlight);
    }

    CommandBufferPool::~CommandBufferPool(){
        
    }

    Unique<CommandBuffer> CommandBufferPool::CreateCommandBuffer(CommandBufferType type, const std::string& name) {
    return CreateUnique<CommandBuffer>(m_Device, m_Pool, type, name);
}

    std::vector<Unique<CommandBuffer>> CommandBufferPool::CreateCommandBuffers(
        CommandBufferType type, uint32_t count, const std::string& baseName) {
        
        std::vector<Unique<CommandBuffer>> commandBuffers;
        commandBuffers.reserve(count);
        
        for (uint32_t i = 0; i < count; ++i) {
            std::string name = baseName.empty() ? "" : baseName + "_" + std::to_string(i);
            commandBuffers.push_back(CreateCommandBuffer(type, name));
        }
        
        return commandBuffers;
    }

    Unique<CommandBuffer> CommandBufferPool::GetFrameCommandBuffer(
        CommandBufferType type, uint32_t frameIndex, const std::string& name) {
        
        if (frameIndex >= m_FramesInFlight) {
            throw std::runtime_error("Invalid frame index: " + std::to_string(frameIndex));
        }
        
        std::string fullName = name + "_Frame" + std::to_string(frameIndex);
        return CreateCommandBuffer(type, fullName);
    }

}