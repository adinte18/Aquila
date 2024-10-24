//
// Created by alexa on 19/10/2024.
//

#ifndef UI_H
#define UI_H

#include <Scene.h>
#include <Engine/Device.h>

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_vulkan.h"
#include "nfd.h"
#include "ImGuizmo/ImGuizmo.h"

namespace Editor{
    static entt::entity selectedEntity = entt::null;

    class UI {
    public:
        void OnStart();
        void OnUpdate(VkCommandBuffer commandBuffer, ECS::Scene &scene);
        void OnEnd();

        UI(Engine::Device& device, Engine::Window& window, const VkRenderPass renderPass)
            : device{device}, window{window}, renderPass(renderPass) {};

        ~UI() {
            vkDestroyDescriptorPool(device.vk_GetDevice(), editorDescriptorPool, nullptr);
        };

    private:
        static void SetSelectedEntity(entt::entity entity) {
            selectedEntity = entity;
        }

        static entt::entity GetSelectedEntity() {
            return selectedEntity;
        }

        Engine::Device& device;
        Engine::Window& window;
        VkRenderPass renderPass;
        VkDescriptorPool editorDescriptorPool{};
    };
}



#endif //UI_H
