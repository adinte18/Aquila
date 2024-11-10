//
// Created by alexa on 19/10/2024.
//

#ifndef UI_H
#define UI_H

#include <Scene.h>
#include <Engine/Device.h>
#include <Engine/Framebuffer.h>

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
        void OnUpdate(VkCommandBuffer commandBuffer, ECS::Scene &scene, Engine::Framebuffer& viewportFramebuffer);
        void OnEnd();

        UI(Engine::Device& device, Engine::Window& window, const VkRenderPass renderPass)
            : device{device}, window{window}, renderPass(renderPass) {};

        ~UI() {
            vkDestroyDescriptorPool(device.vk_GetDevice(), editorDescriptorPool, nullptr);
        };

        [[nodiscard]] bool IsViewportResized() const { return viewportResized; }
        [[nodiscard]] VkExtent2D GetViewportSize() const { return viewportExtent; }
        void SetViewportResized(const bool resized) { viewportResized = resized; }

    private:
        static void SetSelectedEntity(const entt::entity entity) {
            selectedEntity = entity;
        }

        static entt::entity GetSelectedEntity() {
            return selectedEntity;
        }

        bool viewportResized = false;
        VkExtent2D viewportExtent{};

        Engine::Device& device;
        Engine::Window& window;
        VkRenderPass renderPass;
        VkDescriptorPool editorDescriptorPool{};
    };
}



#endif //UI_H
