//
// Created by alexa on 19/10/2024.
//

#ifndef COMMANDBUFFER_H
#define COMMANDBUFFER_H

#include <iostream>

#include "AquilaCore.h"

namespace Engine {

    class CommandBuffer {
    public:
        CommandBuffer(VkDevice device, VkCommandPool commandPool)
            : device(device), commandPool(commandPool) {
            AllocateCommandBuffer();
        }

        ~CommandBuffer() {
            vkFreeCommandBuffers(device, commandPool, 1, &commandBuffer);
        }

        void Begin() const;

        void End() const;

        [[nodiscard]] VkCommandBuffer GetCommandBuffer() const;

    private:
        VkDevice device;
        VkCommandPool commandPool;
        VkCommandBuffer commandBuffer{};

        void AllocateCommandBuffer();
    };

}



#endif //COMMANDBUFFER_H
