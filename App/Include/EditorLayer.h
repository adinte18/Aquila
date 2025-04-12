
#ifndef EDITORLAYER_H
#define EDITORLAYER_H
#include <memory>

#include <UIManagement/UI.h>
#include <Events/EventDispatcher.h>

#include <Engine/Window.h>
#include <Engine/Device.h>
#include <Engine/RenderManager.h>

#include "SceneContext.h"
#include "RenderingSystems/CompositeRenderingSystem.h"
#include "RenderingSystems/EnvToCubemapRenderingSystem.h"
#include "RenderingSystems/SceneRenderingSystem.h"
#include "RenderingSystems/DepthRenderingSystem.h"
#include "RenderingSystems/GridRenderingSystem.h"
#include "RenderingSystems/CubemapRenderingSystem.h"
#include "RenderingSystems/PPRenderingSystem.h"
#include "RenderingSystems/IrradianceRenderingSystem.h"


namespace Editor {

    class EditorLayer {
    public:
        [[nodiscard]] Engine::Window& GetWindow() const { return *m_Window; }
        [[nodiscard]] Engine::Device& GetDevice() const { return *m_Device; }
        [[nodiscard]] Engine::RenderManager& GetRenderManager() const { return *m_RenderManager; }
        [[nodiscard]] UIManager& GetUIManager() const { return *m_UI; }

        EditorLayer() {
            OnStart();
        }

        ~EditorLayer() {
            OnEnd();
        }

        void OnStart();
        void OnUpdate();
        void OnEnd();

    private:
        void HandleAddEntity() {
            std::cout << "Adding a new entity..." << std::endl;
            const auto entity = m_Scene->CreateEntity();
            entity->AddComponent<ECS::Transform>();
        }

        void HandleRemoveEntity(const RemoveEntityEvent& event) {
            std::cout << "Removing entity with ID: " << static_cast<int>(event.entityID) << std::endl;
        }

        void HandleAddEnvMap(const std::string& path) {
            std::cout << "Adding a new environment map..." << std::endl;
            hdrUpdated = true;
            m_HDRTexture = Engine::Texture2D::create(*m_Device);
            m_HDRTexture->CreateHDRTexture(path);
        }


        // Time
        float m_FrameTime{0.f};
        std::chrono::time_point<std::chrono::steady_clock> m_CurrentTime{};

        // testing
        bool hdrUpdated = false;
        std::shared_ptr<Engine::Texture2D> m_HDRTexture;

        // Window and device
        std::unique_ptr<Engine::Window> m_Window;
        std::unique_ptr<Engine::Device> m_Device;

        std::unique_ptr<EventDispatcher> m_EventDispatcher;

        // Renderer manager (onscreen/offscreen)
        std::unique_ptr<Engine::RenderManager> m_RenderManager;

        // Rendering systems
        std::shared_ptr<RenderingSystem::SceneRenderingSystem> sceneRenderingSystem;
        std::shared_ptr<RenderingSystem::DepthRenderingSystem> shadowRenderingSystem;
        std::shared_ptr<RenderingSystem::GridRenderingSystem> gridRenderingSystem;
        std::shared_ptr<RenderingSystem::PPRenderingSystem> postprocessingRenderingSystem;
        std::shared_ptr<RenderingSystem::CompositeRenderingSystem> compositeRenderingSystem;
        std::shared_ptr<RenderingSystem::EnvToCubemapRenderingSystem> envToCubemapRenderingSystem;
        std::shared_ptr<RenderingSystem::CubemapRenderingSystem> cubemapRenderingSystem;
        std::shared_ptr<RenderingSystem::IrradianceRenderingSystem> irradianceRenderingSystem;

        // UI Manager
        std::unique_ptr<UIManager> m_UI;

        // Scene
        std::shared_ptr<ECS::Scene> m_Scene;

        // Scene context
        std::unique_ptr<ECS::SceneContext> m_SceneContext;

        std::unique_ptr<Engine::DescriptorPool> envToCubemapDescriptorPool{};
        std::unique_ptr<Engine::DescriptorSetLayout> envToCubemapDescriptorSetLayout{};
        VkDescriptorSet envToCubemapDescriptorSet;

        std::unique_ptr<Engine::DescriptorPool> cubemapDescriptorPool{};
        std::unique_ptr<Engine::DescriptorSetLayout> cubemapDescriptorSetLayout{};
        VkDescriptorSet cubemapDescriptorSet;

        std::unique_ptr<Engine::DescriptorPool> irradianceDescriptorPool{};
        std::unique_ptr<Engine::DescriptorSetLayout> irradianceDescriptorSetLayout{};
        VkDescriptorSet irradianceDescriptorSet;
    };
    // public:
    //     virtual ~EditorLayer() {
    //
    //     };
    //
    //     std::unique_ptr<Engine::Window> window;
    //     std::unique_ptr<Engine::Device> device;
    //     std::unique_ptr<Engine::Renderer> renderer;
    //     std::unique_ptr<Engine::OffscreenRenderer> offscreenRenderer;
    //     std::shared_ptr<RenderingSystem::SceneRenderingSystem> sceneRenderingSystem;
    //     std::shared_ptr<RenderingSystem::DepthRenderingSystem> shadowRenderingSystem;
    //     std::shared_ptr<RenderingSystem::GridRenderingSystem> gridRenderingSystem;
    //     std::shared_ptr<RenderingSystem::PPRenderingSystem> postprocessingRenderingSystem;
    //     std::shared_ptr<RenderingSystem::CompositeRenderingSystem> compositeRenderingSystem;
    //
    //
    //     //TESTING PURPOSES
    //     std::shared_ptr<Engine::DescriptorSetLayout> ppDescriptorSetLayout;
    //     std::shared_ptr<Engine::DescriptorPool> ppDescriptorPool;
    //     VkDescriptorSet ppDescriptorSet;
    //
    //     std::shared_ptr<Engine::DescriptorSetLayout> finalDescriptorSetLayout;
    //     std::shared_ptr<Engine::DescriptorPool> finalDescriptorPool;
    //     VkDescriptorSet finalDescriptorSet;
    //
    //
    //     std::unique_ptr<UI> ui;
    //     std::unique_ptr<ECS::Scene> scene;
    //
    //     double lastX = 0;
    //     double lastY = 0;
    //
    //     virtual void OnStart() {
    //         // Initialize window
    //         window = std::make_unique<Engine::Window>(800, 600, "Aquila Editor");
    //         device = std::make_unique<Engine::Device>(*window);
    //
    //         // Initialize renderers
    //         renderer = std::make_unique<Engine::Renderer>(*window, *device);
    //         offscreenRenderer = std::make_unique<Engine::OffscreenRenderer>(*device);
    //         offscreenRenderer->Initialize(800, 600);
    //
    //         // Initialize scene
    //         scene = std::make_unique<ECS::Scene>(*device, *offscreenRenderer);
    //
    // std::array<VkDescriptorSetLayout, 2> setLayouts =
    //     {scene->GetSceneDescriptorSetLayout().getDescriptorSetLayout(),
    //     scene->GetMaterialDescriptorSetLayout().getDescriptorSetLayout()};
    //
    // // Initialize scene rendering system
    // sceneRenderingSystem = std::make_shared<RenderingSystem::SceneRenderingSystem>(*device,
    //     offscreenRenderer->GetRenderPass(Engine::RenderPassType::GEOMETRY),
    //     setLayouts);
    //
    // gridRenderingSystem = std::make_shared<RenderingSystem::GridRenderingSystem>(*device,
    //      offscreenRenderer->GetRenderPass(Engine::RenderPassType::GEOMETRY),
    //      setLayouts);
    //
    // shadowRenderingSystem = std::make_shared<RenderingSystem::DepthRenderingSystem>(*device,
    //     offscreenRenderer->GetRenderPass(Engine::RenderPassType::SHADOW),
    //     setLayouts);
    //
    // // Post processing system
    // ppDescriptorPool = Engine::DescriptorPool::Builder(*device)
    //         .setMaxSets(100)
    //         .addPoolSize(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000)
    //         .build();
    //
    // ppDescriptorSetLayout = Engine::DescriptorSetLayout::Builder(*device)
    //         .addBinding(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT) //scene image
    //         .build();
    //
    // ppDescriptorPool->allocateDescriptor(ppDescriptorSetLayout->getDescriptorSetLayout(), ppDescriptorSet);
    //
    // postprocessingRenderingSystem = std::make_shared<RenderingSystem::PPRenderingSystem>(*device,
    //     offscreenRenderer->GetRenderPass(Engine::RenderPassType::POST_PROCESSING),
    //     ppDescriptorSetLayout->getDescriptorSetLayout());
    //
    // finalDescriptorPool = Engine::DescriptorPool::Builder(*device)
    //         .setMaxSets(100)
    //         .addPoolSize(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000)
    //         .build();
    //
    // finalDescriptorSetLayout = Engine::DescriptorSetLayout::Builder(*device)
    //         .addBinding(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT) //final image
    //         .build();
    //
    // finalDescriptorPool->allocateDescriptor(finalDescriptorSetLayout->getDescriptorSetLayout(), finalDescriptorSet);
    //
    // compositeRenderingSystem = std::make_shared<RenderingSystem::CompositeRenderingSystem>(*device,
    //     offscreenRenderer->GetRenderPass(Engine::RenderPassType::FINAL),
    //     finalDescriptorSetLayout->getDescriptorSetLayout());
    //
    //         // Initialize UI
    //         ui = std::make_unique<UI>(*device, *window, renderer->vk_GetCurrentRenderPass());
    //         ui->OnStart();
    //
    //         scene->GetActiveCamera().SetPerspectiveProjection(glm::radians(80.f), offscreenRenderer->GetAspectRatio(), 1.0f, 100.f);
    //         scene->GetActiveCamera().SetPosition(glm::vec3(0,1,-10));
    //         scene->GetActiveCamera().SetViewYXZ(scene->GetActiveCamera().GetPosition(), glm::vec3(0,0,0));
    //     }
    //
    //
    //     virtual void OnUpdate() {
    //         // Poll events
    //         window->pollEvents();
    //
    //         scene->OnUpdate();
    //
    //         // Verify if the viewport was resized
    //         if (ui->IsViewportResized()) {
    //             offscreenRenderer->Resize(ui->GetViewportSize());
    //             ui->SetViewportResized(false);
    //             scene->GetActiveCamera().OnResize(ui->GetViewportSize().width, ui->GetViewportSize().height);
    //         }
    //
    //         auto shadowInfo = offscreenRenderer->GetImageInfo(Engine::RenderPassType::SHADOW, VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL);
    //         auto descriptorSet = scene->GetSceneDescriptorSet();
    //         Engine::DescriptorWriter writer(scene->GetSceneDescriptorSetLayout(), scene->GetSceneDescriptorPool());
    //         writer.writeImage(1, &shadowInfo);
    //         writer.overwrite(descriptorSet);
    //
    //         auto sceneInfo = offscreenRenderer->GetImageInfo(Engine::RenderPassType::GEOMETRY, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    //         Engine::DescriptorWriter ppWriter(*ppDescriptorSetLayout, *ppDescriptorPool);
    //         ppWriter.writeImage(0, &sceneInfo);
    //         ppWriter.overwrite(ppDescriptorSet);
    //
    //         auto finalInfo = offscreenRenderer->GetImageInfo(Engine::RenderPassType::POST_PROCESSING, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    //         Engine::DescriptorWriter finalWriter(*finalDescriptorSetLayout, *finalDescriptorPool);
    //         finalWriter.writeImage(0, &finalInfo);
    //         finalWriter.overwrite(finalDescriptorSet);
    //
    //         ui->UpdateDescriptorSets(*scene);
    //
    //         if (auto commandBuffer = renderer->vk_BeginFrame()) {
    //              //*************************************************
    //              //*             Offscreen rendering             *//
    //              //*************************************************
    //              // !!Render scene here!!
    //
    //             offscreenRenderer->BeginRenderPass(commandBuffer, Engine::RenderPassType::SHADOW);
    //             shadowRenderingSystem->Render(commandBuffer, *scene);
    //             offscreenRenderer->EndRenderPass(commandBuffer);
    //
    //
    //             offscreenRenderer->BeginRenderPass(commandBuffer, Engine::RenderPassType::GEOMETRY);
    //             sceneRenderingSystem->Render(commandBuffer, *scene);
    //
    //             // Transparent grid
    //             gridRenderingSystem->Render(commandBuffer, *scene);
    //             offscreenRenderer->EndRenderPass(commandBuffer);
    //
    //             // transition scene from color to shader read only
    //             offscreenRenderer->TransitionImages(commandBuffer, Engine::RenderPassType::GEOMETRY, Engine::RenderPassType::POST_PROCESSING);
    //
    //             // here we do post-processing on the scene image and we store the image as color
    //             offscreenRenderer->BeginRenderPass(commandBuffer, Engine::RenderPassType::POST_PROCESSING);
    //             postprocessingRenderingSystem->Render(commandBuffer, ppDescriptorSet);
    //             offscreenRenderer->EndRenderPass(commandBuffer);
    //
    //             // now again - we transition from color to shader read only for the gui to use it
    //             offscreenRenderer->TransitionImages(commandBuffer, Engine::RenderPassType::POST_PROCESSING, Engine::RenderPassType::FINAL);
    //
    //             // and we render the final image
    //             offscreenRenderer->BeginRenderPass(commandBuffer, Engine::RenderPassType::FINAL);
    //             compositeRenderingSystem->Render(commandBuffer, finalDescriptorSet);
    //             offscreenRenderer->EndRenderPass(commandBuffer);
    //
    //              //*************************************************
    //              //*             Onscreen rendering              *//
    //              //*************************************************
    //              renderer->vk_BeginSwapChainRenderPass(commandBuffer);
    //
    //              //Maybe expensive? Temporary
    //              if (scene->sceneView != offscreenRenderer->GetRenderPassImage(Engine::RenderPassType::FINAL)) {
    //                  scene->sceneView = offscreenRenderer->GetRenderPassImage(Engine::RenderPassType::FINAL);
    //              }
    //
    //              //Render UI
    //              ui->OnUpdate(commandBuffer, *scene);
    //
    //             //End render pass
    //             renderer->vk_EndSwapChainRenderPass(commandBuffer);
    //
    //             // End recording
    //             renderer->vk_EndFrame();
    //         }
    //
    //         vkDeviceWaitIdle(device->vk_GetDevice());
    //
    //         for(auto& entity : scene->queuedForDestruction) {
    //             scene->DestroyEntity(entity);
    //         }
    //
    //         scene->queuedForDestruction.clear();
    //
    //         if (ui->NeedsShaderReloading()) {
    //             sceneRenderingSystem->RecreatePipeline(offscreenRenderer->GetRenderPass(Engine::RenderPassType::GEOMETRY));
    //             gridRenderingSystem->RecreatePipeline(offscreenRenderer->GetRenderPass(Engine::RenderPassType::GEOMETRY));
    //             shadowRenderingSystem->RecreatePipeline(offscreenRenderer->GetRenderPass(Engine::RenderPassType::SHADOW));
    //             postprocessingRenderingSystem->RecreatePipeline(offscreenRenderer->GetRenderPass(Engine::RenderPassType::POST_PROCESSING));
    //             compositeRenderingSystem->RecreatePipeline(offscreenRenderer->GetRenderPass(Engine::RenderPassType::FINAL));
    //
    //             ui->SetNeedsShaderReloading(false);
    //         }
    //     }
    //
    //     virtual void OnEnd() {
    //
    //         // Wait for everything to finish before exiting
    //         vkDeviceWaitIdle(device->vk_GetDevice());
    //
    //         // Cleanup
    //         ImGui_ImplVulkan_Shutdown();
    //         ImGui_ImplGlfw_Shutdown();
    //         ImGui::DestroyContext();
    //
    //         window->cleanup();
    //     }
    //
    //     virtual void OnEvent() {
    //         // Handle events related to the editor
    //     }
    // };

}



#endif //EDITORLAYER_H
