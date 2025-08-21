#ifndef COMMAND_BUFFER_H
#define COMMAND_BUFFER_H

#include "Engine/Renderer/Device.h"
#include "vulkan/vulkan_core.h"


namespace Engine {
    enum class CommandBufferType {
        PRESENT,
        OFFSCREEN,
        COMPUTE
    };

    class CommandBuffer {
        public:
            CommandBuffer(Device& device, VkCommandPool commandPool, CommandBufferType type, const std::string name);
            ~CommandBuffer();

            void Begin(VkCommandBufferUsageFlags flags = 0);
            void Reset();
            void End();

            VkCommandBuffer GetHandle() const;
            CommandBufferType GetType() const;
            const std::string& GetName() const;
            bool IsRecording() const;            

        private:
            VkCommandBuffer m_CommandBuffer = VK_NULL_HANDLE;
            CommandBufferType m_Type;
            std::string m_Name;
            bool m_IsRecording = false;
            Device& m_Device;
    };
}

#endif // COMMAND_BUFFER_H