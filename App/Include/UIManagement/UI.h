//
// Created by alexa on 19/10/2024.
//

#ifndef UI_H
#define UI_H

#include <Scene.h>
#include <Engine/Device.h>
#include <Engine/Framebuffer.h>
#include <Events/EventDispatcher.h>


#include "Icon.h"
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_vulkan.h"
#include "nfd.h"
#include "SceneContext.h"
#include "ImGuizmo/ImGuizmo.h"

namespace Editor {
    static entt::entity selectedEntity = entt::null;

    class UIManager {
        Engine::Device& m_Device;
        Engine::Window& m_Window;

        VkRenderPass m_RenderPass;
        VkDescriptorPool m_UiDescriptorPool{};
        bool m_ReloadShaders = false;
        bool m_ViewportResized = false;
        VkExtent2D m_ViewportExtent{};

        void InitializeImGui();

        static entt::entity GetSelectedEntity() { return selectedEntity; }
        static void SetSelectedEntity(entt::entity entity) { selectedEntity = entity; }

        ImVec2 lastViewportSize = {0, 0};

        VkDescriptorSet m_FinalImage{};

        EventDispatcher& m_Dispatcher;

        entt::registry& m_Registry;

    public:
        UIManager(Engine::Device& device, Engine::Window& window, VkRenderPass renderPass, EventDispatcher& dispatcher, entt::registry& registry)
            : m_Device(device), m_Window(window), m_RenderPass(renderPass), m_Dispatcher(dispatcher), m_Registry(registry) {
            OnStart();
        }

        ~UIManager() {
            OnEnd();
        }

        [[nodiscard]] bool IsViewportResized() const { return m_ViewportResized; }
        [[nodiscard]] VkExtent2D GetViewportSize() const { return m_ViewportExtent; }
        void SetViewportResized(const bool resized) { m_ViewportResized = resized; }
        void UpdateDescriptorSets(ECS::SceneContext &sceneContext);

        void GetFinalImage(VkDescriptorSet finalImage);

        void OnStart();
        void OnUpdate(VkCommandBuffer commandBuffer, ECS::SceneContext &sceneContext, float deltaTime);
        // void Render(VkCommandBuffer commandBuffer);
        void OnEnd();


        // public:
        //     ImVec2 lastViewportSize = {0, 0};
        //
        //     void OnStart();
        //     void OnUpdate(VkCommandBuffer commandBuffer, ECS::Scene &scene);
        //     void OnEnd();
        //
        //     UI(Engine::Device& device, Engine::Window& window, const VkRenderPass renderPass)
        //         : device{device}, window{window}, renderPass(renderPass) {};
        //
        //     ~UI() {
        //         vkDestroyDescriptorPool(device.vk_GetDevice(), editorDescriptorPool, nullptr);
        //     };
        //
        //     [[nodiscard]] bool IsViewportResized() const { return viewportResized; }
        //     [[nodiscard]] VkExtent2D GetViewportSize() const { return viewportExtent; }
        //     void SetViewportResized(const bool resized) { viewportResized = resized; }
        //     void GetTimings(float time);
        //     void UpdateDescriptorSets(ECS::Scene& scene);
        //     [[nodiscard]] bool NeedsShaderReloading() const { return reloadShaders; }
        //     [[nodiscard]] bool SetNeedsShaderReloading(bool reload) { reloadShaders = reload; return reloadShaders; }
        //
        // private:
        //     static void SetSelectedEntity(const entt::entity entity) {
        //         selectedEntity = entity;
        //     }
        //
        //     static entt::entity GetSelectedEntity() {
        //         return selectedEntity;
        //     }
        //
        //     bool viewportResized = false;
        //     VkExtent2D viewportExtent{};
        //
        //     Engine::Device& device;
        //     Engine::Window& window;
        //     VkRenderPass renderPass;
        //     VkDescriptorPool editorDescriptorPool{};
        //
        //     bool reloadShaders = false;
        //
        //     float gpuTime{0.0f};
        // };
    };
}


#endif //UI_H
