
#ifndef EDITORLAYER_H
#define EDITORLAYER_H
#include <memory>
#include <UI.h>

#include <Engine/Window.h>
#include <Engine/Device.h>
#include <Engine/Renderer.h>
#include <ECS/Scene.h>
#include <Engine/Buffer.h>

#include "Engine/OffscreenRenderer.h"
#include "RenderingSystems/SceneRenderingSystem.h"
#include "RenderingSystems/DepthRenderingSystem.h"
#include "RenderingSystems/GridRenderingSystem.h"


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
        std::shared_ptr<RenderingSystem::DepthRenderingSystem> shadowRenderingSystem;
        std::shared_ptr<RenderingSystem::GridRenderingSystem> gridRenderingSystem;


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
            offscreenRenderer = std::make_unique<Engine::OffscreenRenderer>(*device);
            offscreenRenderer->Initialize(800, 600);

            // Initialize scene
            scene = std::make_unique<ECS::Scene>(*device, *offscreenRenderer);

            std::array<VkDescriptorSetLayout, 2> setLayouts =
                {scene->GetSceneDescriptorSetLayout().getDescriptorSetLayout(),
                scene->GetMaterialDescriptorSetLayout().getDescriptorSetLayout()};

            // Initialize scene rendering system
            sceneRenderingSystem = std::make_shared<RenderingSystem::SceneRenderingSystem>(*device,
                offscreenRenderer->GetRenderPass(Engine::RenderPassType::SCENE),
                setLayouts);

            gridRenderingSystem = std::make_shared<RenderingSystem::GridRenderingSystem>(*device,
                 offscreenRenderer->GetRenderPass(Engine::RenderPassType::SCENE),
                 setLayouts);

            shadowRenderingSystem = std::make_shared<RenderingSystem::DepthRenderingSystem>(*device,
                offscreenRenderer->GetRenderPass(Engine::RenderPassType::SHADOW),
                setLayouts);


            // Initialize UI
            ui = std::make_unique<UI>(*device, *window, renderer->vk_GetCurrentRenderPass());
            ui->OnStart();

            scene->GetActiveCamera().SetPerspectiveProjection(glm::radians(80.f), offscreenRenderer->GetAspectRatio(), 1.0f, 100.f);
            scene->GetActiveCamera().SetPosition(glm::vec3(0,1,-10));
            scene->GetActiveCamera().SetViewYXZ(scene->GetActiveCamera().GetPosition(), glm::vec3(0,0,0));
        }

        virtual void OnUpdate() {
            // Poll events
            window->pollEvents();

            scene->OnUpdate();

            // Verify if the viewport was resized
            if (ui->IsViewportResized()) {
                offscreenRenderer->Resize(ui->GetViewportSize());
                ui->SetViewportResized(false);
                scene->GetActiveCamera().OnResize(ui->GetViewportSize().width, ui->GetViewportSize().height);
            }

            auto testInfo = offscreenRenderer->GetDepthInfo(Engine::RenderPassType::SHADOW);
            auto descriptorSet = scene->GetSceneDescriptorSet();
            Engine::DescriptorWriter writer(scene->GetSceneDescriptorSetLayout(), scene->GetSceneDescriptorPool());
            writer.writeImage(1, &testInfo);
            writer.overwrite(descriptorSet);

            if (auto commandBuffer = renderer->vk_BeginFrame()) {
                 //*************************************************
                 //*             Offscreen rendering             *//
                 //*************************************************
                 // !!Render scene here!!

                offscreenRenderer->BeginRenderPass(commandBuffer, Engine::RenderPassType::SHADOW);
                shadowRenderingSystem->Render(commandBuffer, *scene);
                offscreenRenderer->EndRenderPass(commandBuffer);


                offscreenRenderer->BeginRenderPass(commandBuffer, Engine::RenderPassType::SCENE);
                sceneRenderingSystem->Render(commandBuffer, *scene);

                // Transparent grid
                gridRenderingSystem->Render(commandBuffer, *scene);
                offscreenRenderer->EndRenderPass(commandBuffer);

                 //*************************************************
                 //*             Onscreen rendering              *//
                 //*************************************************
                 renderer->vk_BeginSwapChainRenderPass(commandBuffer);

                 //Maybe expensive? Temporary
                 if (scene->sceneView != offscreenRenderer->GetRenderPassImage(Engine::RenderPassType::SCENE)) {
                     scene->sceneView = offscreenRenderer->GetRenderPassImage(Engine::RenderPassType::SCENE);
                 }

                 //Render UI
                 ui->OnUpdate(commandBuffer, *scene);

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
