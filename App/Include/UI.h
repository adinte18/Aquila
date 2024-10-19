//
// Created by alexa on 19/10/2024.
//

#ifndef UI_H
#define UI_H

#include <Engine/Device.h>

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_vulkan.h"
#include "nfd.h"

namespace Editor{
    class UI {
    public:
        Engine::Device& device;
        Engine::Window& window;
        VkRenderPass renderPass;
        VkDescriptorPool editorDescriptorPool{};

        UI(Engine::Device& device, Engine::Window& window, const VkRenderPass renderPass)
            : device{device}, window{window}, renderPass(renderPass) {};

        ~UI() {
            vkDestroyDescriptorPool(device.vk_GetDevice(), editorDescriptorPool, nullptr);
        };

        void OnStart();
        void OnUpdate(VkCommandBuffer commandBuffer, VkDescriptorSet sceneView);
        void OnEnd();
    };
}



#endif //UI_H
