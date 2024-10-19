
#ifndef EDITORLAYER_H
#define EDITORLAYER_H
#include <memory>
#include <UI.h>

#include <Engine/Window.h>
#include <Engine/Device.h>
#include <Engine/Renderpass.h>
#include <Engine/Renderer.h>

#include "Engine/OffscreenRenderer.h"

namespace Editor {

    class EditorLayer {
    public:
        virtual ~EditorLayer() {

        };

        std::unique_ptr<Engine::Window> window;
        std::unique_ptr<Engine::Device> device;
        std::unique_ptr<Engine::Renderer> renderer;
        std::unique_ptr<Engine::OffscreenRenderer> offscreenRenderer;
        std::unique_ptr<UI> ui;

        virtual void OnStart() {
            // Initialize window
            window = std::make_unique<Engine::Window>(800, 600, "Aquila Editor");
            device = std::make_unique<Engine::Device>(*window);
            renderer = std::make_unique<Engine::Renderer>(*window, *device);
            offscreenRenderer = std::make_unique<Engine::OffscreenRenderer>(*device,
                VkExtent2D{300, 300});

            ui = std::make_unique<UI>(*device, *window, renderer->vk_GetCurrentRenderPass());
            ui->OnStart();
        }

        virtual void OnUpdate() {
            // Poll events
            window->pollEvents();

            //Render ImGui to screen (to swapchain)
            if (auto commandBuffer = renderer->vk_BeginFrame()) {
                //*************************************************
                //*             Offscreen rendering             *//
                //*************************************************
                offscreenRenderer->BeginRenderPass(commandBuffer);
                // !!Render scene here!!


                offscreenRenderer->EndRenderPass(commandBuffer);

                //*************************************************
                //*             Onscreen rendering              *//
                //*************************************************
                renderer->vk_BeginSwapChainRenderPass(commandBuffer);

                // Render UI
                ui->OnUpdate(commandBuffer, offscreenRenderer->GetFramebuffer().GetDescriptorSet());

                //End render pass
                renderer->vk_EndSwapChainRenderPass(commandBuffer);

                // End recording
                renderer->vk_EndFrame();
            }
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
