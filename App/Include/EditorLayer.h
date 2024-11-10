
#ifndef EDITORLAYER_H
#define EDITORLAYER_H
#include <memory>
#include <UI.h>

#include <Engine/Window.h>
#include <Engine/Device.h>
#include <Engine/Renderpass.h>
#include <Engine/Renderer.h>
#include <ECS/Scene.h>
#include <Engine/Buffer.h>

#include "Engine/OffscreenRenderer.h"
#include "RenderingSystems/SceneRenderingSystem.h"


namespace Editor {

    class EditorLayer {
    public:
        virtual ~EditorLayer() {

        };

        std::unique_ptr<Engine::Window> window;
        std::unique_ptr<Engine::Device> device;
        std::unique_ptr<Engine::Renderer> renderer;
        std::unique_ptr<Engine::OffscreenRenderer> offscreenRenderer;
        std::shared_ptr<RenderingSystem::SceneRenderingSystem> sceneRenderingSystem;

        std::unique_ptr<UI> ui;
        std::unique_ptr<ECS::Scene> scene;

        double lastX = 0;
        double lastY = 0;

        virtual void OnStart() {
            // Initialize window
            window = std::make_unique<Engine::Window>(800, 600, "Aquila Editor");
            device = std::make_unique<Engine::Device>(*window);

            // Initialize renderers
            renderer = std::make_unique<Engine::Renderer>(*window, *device);
            offscreenRenderer = std::make_unique<Engine::OffscreenRenderer>(*device,
                VkExtent2D{1, 1});

            // Initialize scene
            scene = std::make_unique<ECS::Scene>(*device);

            std::array<VkDescriptorSetLayout, 2> setLayouts =
                {scene->GetSceneDescriptorSetLayout().getDescriptorSetLayout(),
                scene->GetMaterialDescriptorSetLayout().getDescriptorSetLayout()};

            // Initialize scene rendering system
            sceneRenderingSystem = std::make_shared<RenderingSystem::SceneRenderingSystem>(*device,
                offscreenRenderer->GetRenderPass(),
                setLayouts);

            // Initialize UI
            ui = std::make_unique<UI>(*device, *window, renderer->vk_GetCurrentRenderPass());
            ui->OnStart();

            scene->GetActiveCamera().SetPerspectiveProjection(glm::radians(80.f), renderer->vk_GetAspectRatio(), 0.1, 1000.f);
        }

        virtual void OnUpdate() {
            // Poll events
            window->pollEvents();

            scene->OnUpdate();

            // Verify if the viewport was resized
            if (ui->IsViewportResized()) {
                offscreenRenderer->Recreate(ui->GetViewportSize());
                ui->SetViewportResized(false);
                scene->GetActiveCamera().OnResize(ui->GetViewportSize().width, ui->GetViewportSize().height);
            }

            //Render ImGui to screen (to swapchain)
            if (auto commandBuffer = renderer->vk_BeginFrame()) {
                //*************************************************
                //*             Offscreen rendering             *//
                //*************************************************
                offscreenRenderer->BeginRenderPass(commandBuffer);
                // !!Render scene here!!

                sceneRenderingSystem->Render(commandBuffer, *scene);

                offscreenRenderer->EndRenderPass(commandBuffer);

                //*************************************************
                //*             Onscreen rendering              *//
                //*************************************************
                renderer->vk_BeginSwapChainRenderPass(commandBuffer);


                //Maybe expensive? Temporary
                if (scene->sceneView != offscreenRenderer->GetFramebuffer().GetDescriptorSet()) {
                    scene->sceneView = offscreenRenderer->GetFramebuffer().GetDescriptorSet();
                }

                // Render UI
                Engine::Framebuffer& framebuffer = offscreenRenderer->GetFramebuffer();
                ui->OnUpdate(commandBuffer, *scene, framebuffer);

                //End render pass
                renderer->vk_EndSwapChainRenderPass(commandBuffer);

                // End recording
                renderer->vk_EndFrame();
            }

            // Wait for everything to finish before deleting entities
            vkDeviceWaitIdle(device->vk_GetDevice());

            // Delete entities that were queued for destruction last frame
            for(auto& entity : scene->queuedForDestruction) {
                scene->DestroyEntity(entity);
            }

            scene->queuedForDestruction.clear();
        }

        virtual void OnEnd() {

            // Wait for everything to finish before exiting
            vkDeviceWaitIdle(device->vk_GetDevice());

            // Cleanup
            ImGui_ImplVulkan_Shutdown();
            ImGui_ImplGlfw_Shutdown();
            ImGui::DestroyContext();

            window->cleanup();
        }

        virtual void OnEvent() {
            // Handle events related to the editor
        }
    };

}



#endif //EDITORLAYER_H
