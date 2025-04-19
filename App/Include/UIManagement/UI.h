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
#include "SceneContext.h"
#include "ImGuizmo/ImGuizmo.h"
#include <filesystem>

namespace Editor {
    static entt::entity selectedEntity = entt::null;
    static std::filesystem::path currentDirectory = ASSET_PATH;


    class UIManager {
        Engine::Device& m_Device;
        Engine::Window& m_Window;

        VkRenderPass m_RenderPass;
        VkDescriptorPool m_UiDescriptorPool{};

        bool m_ViewportResized = false;
        VkExtent2D m_ViewportExtent{};

        void InitializeImGui() const;

        static entt::entity GetSelectedEntity() { return selectedEntity; }
        static void SetSelectedEntity(entt::entity entity) { selectedEntity = entity; }

        ImVec2 m_LastViewportSize = {0, 0};

        VkDescriptorSet m_FinalImage{};

        entt::registry& m_Registry;

        ImGuiID m_DockspaceId;

    public:
        UIManager(Engine::Device& device, Engine::Window& window, VkRenderPass renderPass, entt::registry& registry)
            : m_Device(device), m_Window(window), m_RenderPass(renderPass), m_Registry(registry) {
            OnStart();
        }

        ~UIManager() {
            OnEnd();
        }

        [[nodiscard]] bool IsViewportResized() const { return m_ViewportResized; }
        [[nodiscard]] VkExtent2D GetViewportSize() const { return m_ViewportExtent; }
        void SetViewportResized(const bool resized) { m_ViewportResized = resized; }
        void UpdateDescriptorSets(const ECS::SceneContext &sceneContext) const;

        void GetFinalImage(VkDescriptorSet finalImage);

        void OnStart();
        void OnUpdate(VkCommandBuffer commandBuffer, ECS::SceneContext &sceneContext, float deltaTime);
        void OnEnd() const;
    };
}


#endif //UI_H
