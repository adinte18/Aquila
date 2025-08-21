#include "Engine/Renderer/CommandBuffer.h"

namespace Engine {

    CommandBuffer::CommandBuffer(Device& device, VkCommandPool commandPool, CommandBufferType type, const std::string name)
        : m_Device(device), m_Type(type), m_Name(name) {
        
        VkCommandBufferAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocInfo.commandPool = commandPool;
        allocInfo.commandBufferCount = 1;

        if (vkAllocateCommandBuffers(device.GetDevice(), &allocInfo, &m_CommandBuffer) != VK_SUCCESS){
            throw std::runtime_error("Failed to allocate command buffer");
        }
    }

    CommandBuffer::~CommandBuffer() {
        // Note(A) : freed automatically when comand pool destroyed
    }

    void CommandBuffer::Begin(VkCommandBufferUsageFlags flags) {
        if (m_IsRecording){
            throw std::runtime_error("Command buffer " + m_Name + " is in usage already");
        }

        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.flags = flags;

        if (vkBeginCommandBuffer(m_CommandBuffer, &beginInfo) != VK_SUCCESS) {
            throw std::runtime_error("Failed to begin command buffer");
        }

        m_IsRecording = true;
    }

    void CommandBuffer::End() {
        if (!m_IsRecording) {
            throw std::runtime_error("Command buffer " + m_Name + " is not recording!");
        }
        
        if (vkEndCommandBuffer(m_CommandBuffer) != VK_SUCCESS) {
            throw std::runtime_error("Failed to record command buffer: " + m_Name);
        }
        
        m_IsRecording = false;
    }

    void CommandBuffer::Reset() {
        vkResetCommandBuffer(m_CommandBuffer, 0);
        m_IsRecording = false;
    }

    VkCommandBuffer CommandBuffer::GetHandle() const { return m_CommandBuffer; }
    CommandBufferType CommandBuffer::GetType() const { return m_Type; }
    const std::string& CommandBuffer::GetName() const { return m_Name; }
    bool CommandBuffer::IsRecording() const { return m_IsRecording; }



}