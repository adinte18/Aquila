#include "EditorLayer.h"

#include "Events/EventBus.h"


void Editor::EditorLayer::OnStart() {
    m_CurrentTime = std::chrono::steady_clock::now();

    // Initialize window
    m_Window = std::make_unique<Engine::Window>(800, 600, "Aquila Editor");
    m_Device = std::make_unique<Engine::Device>(*m_Window);

    // Initialize renderers
    m_RenderManager = std::make_unique<Engine::RenderManager>(*m_Device, *m_Window);
    m_RenderManager->Initialize();

    // Initialize scene
    m_Scene = std::make_shared<ECS::Scene>();
    m_SceneContext = std::make_unique<ECS::SceneContext>(*m_Device, *m_Scene);


    std::array<VkDescriptorSetLayout, 2> setLayouts =
        {m_SceneContext->GetSceneDescriptorSetLayout().getDescriptorSetLayout(),
        m_SceneContext->GetMaterialDescriptorSetLayout().getDescriptorSetLayout()};

    // TODO : HDR TESTING - TO DELETE
    cubemapDescriptorPool = Engine::DescriptorPool::Builder(*m_Device)
            .setMaxSets(1)
            .addPoolSize(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1)
            .build();

    cubemapDescriptorSetLayout = Engine::DescriptorSetLayout::Builder(*m_Device)
             .addBinding(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT)
             .build();
    cubemapDescriptorPool->allocateDescriptor(cubemapDescriptorSetLayout->getDescriptorSetLayout(), cubemapDescriptorSet);

    // Initialize scene rendering system

    // SCENE SYSTEMS
    sceneRenderingSystem = std::make_shared<RenderingSystem::SceneRenderingSystem>(*m_Device,
        m_RenderManager->GetOffscreenRenderer()->GetPassObject<Engine::GeometryPass>()->GetRenderPass(),
        setLayouts);

    gridRenderingSystem = std::make_shared<RenderingSystem::GridRenderingSystem>(*m_Device,
         m_RenderManager->GetOffscreenRenderer()->GetPassObject<Engine::GeometryPass>()->GetRenderPass(),
         setLayouts);

    shadowRenderingSystem = std::make_shared<RenderingSystem::DepthRenderingSystem>(*m_Device,
        m_RenderManager->GetOffscreenRenderer()->GetPassObject<Engine::ShadowPass>()->GetRenderPass(),
        setLayouts);


    // HDRi SYSTEMS
    envToCubemapRenderingSystem = std::make_shared<RenderingSystem::EnvToCubemapRenderingSystem>(*m_Device,
        m_RenderManager->GetOffscreenRenderer()->GetPassObject<Engine::HDRiToCubemapPass>()->GetRenderPass(),
        m_RenderManager->GetOffscreenRenderer()->GetPassLayout<Engine::HDRiToCubemapPass>()->getDescriptorSetLayout());

    cubemapRenderingSystem = std::make_shared<RenderingSystem::CubemapRenderingSystem>(*m_Device,
        m_RenderManager->GetOffscreenRenderer()->GetPassObject<Engine::GeometryPass>()->GetRenderPass(),
        cubemapDescriptorSetLayout->getDescriptorSetLayout());

    irradianceRenderingSystem = std::make_shared<RenderingSystem::IrradianceRenderingSystem>(*m_Device,
        m_RenderManager->GetOffscreenRenderer()->GetPassObject<Engine::IrradianceSamplingPass>()->GetRenderPass(),
        m_RenderManager->GetOffscreenRenderer()->GetPassLayout<Engine::IrradianceSamplingPass>()->getDescriptorSetLayout());

    prefilterRenderingSystem = std::make_shared<RenderingSystem::PrefilterRenderingSystem>(*m_Device,
        m_RenderManager->GetOffscreenRenderer()->GetPassObject<Engine::HDRPrefilterPass>()->GetRenderPass(),
        m_RenderManager->GetOffscreenRenderer()->GetPassLayout<Engine::HDRPrefilterPass>()->getDescriptorSetLayout());

    brdfLutRenderingSystem = std::make_shared<RenderingSystem::BRDFLutRenderingSystem>(*m_Device,
        m_RenderManager->GetOffscreenRenderer()->GetPassObject<Engine::LUTPass>()->GetRenderPass(),
        m_RenderManager->GetOffscreenRenderer()->GetPassLayout<Engine::LUTPass>()->getDescriptorSetLayout());

    // FINAL/COMPOSITE SYSTEM
    compositeRenderingSystem = std::make_shared<RenderingSystem::CompositeRenderingSystem>(*m_Device,
        m_RenderManager->GetOffscreenRenderer()->GetPassObject<Engine::CompositePass>()->GetRenderPass(),
        m_RenderManager->GetOffscreenRenderer()->GetPassLayout<Engine::CompositePass>()->getDescriptorSetLayout());

    // Initialize UI
    m_UI = std::make_unique<UIManager>(*m_Device,
        *m_Window,
        m_RenderManager->GetOnScreenRenderer()->vk_GetCurrentRenderPass(), m_Scene->GetRegistry());

    this->RegisterHandlers();

    m_Scene->GetActiveCamera().SetPerspectiveProjection(glm::radians(80.f), m_RenderManager->GetOffscreenRenderer()->GetAspectRatio(), 1.0f, 100.f);
    m_Scene->GetActiveCamera().SetPosition(glm::vec3(0,1,-10));
    m_Scene->GetActiveCamera().SetViewYXZ(m_Scene->GetActiveCamera().GetPosition(), glm::vec3(0,0,0));

    std::cout << "Everything went well, and is initialized" << std::endl;
}

glm::mat4 GetCubemapViewMatrix(int face) {
    glm::vec3 directions[6] = {
        { 1.0f,  0.0f,  0.0f}, // +X
        {-1.0f,  0.0f,  0.0f}, // -X
        { 0.0f,  1.0f,  0.0f}, // +Y
        { 0.0f, -1.0f,  0.0f}, // -Y
        { 0.0f,  0.0f,  1.0f}, // +Z
        { 0.0f,  0.0f, -1.0f}  // -Z
    };

    glm::vec3 ups[6] = {
        {0.0f, -1.0f,  0.0f}, // +X
        {0.0f, -1.0f,  0.0f}, // -X
        {0.0f,  0.0f,  1.0f}, // +Y
        {0.0f,  0.0f, -1.0f}, // -Y
        {0.0f, -1.0f,  0.0f}, // +Z
        {0.0f, -1.0f,  0.0f}  // -Z
    };

    return glm::lookAt(glm::vec3(0.0f), directions[face], ups[face]);
}


void Editor::EditorLayer::OnUpdate() {
    auto newTime = std::chrono::steady_clock::now();
    m_FrameTime = std::chrono::duration<float>(newTime - m_CurrentTime).count();
    m_CurrentTime = newTime;

    // Poll events
    m_Window->PollEvents();

    // should be an event
    if (m_UI->IsViewportResized()) {
        m_RenderManager->GetOffscreenRenderer()->Resize(m_UI->GetViewportSize());
        m_UI->SetViewportResized(false);
        m_Scene->GetActiveCamera().OnResize(m_UI->GetViewportSize().width, m_UI->GetViewportSize().height);
    }

    m_UI->UpdateDescriptorSets(*m_SceneContext);

    // Update scene
    m_SceneContext->UpdateGPUData();

    // Render offscreen - Scene
    if (auto commandBuffer = m_RenderManager->GetOffscreenRenderer()->BeginFrame()) {

        // get images for shadows, ibl and cubemap rendering
        auto shadowInfo = m_RenderManager->GetOffscreenRenderer()->GetPassObject<Engine::ShadowPass>()->GetImageInfo(VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL);
        auto irradianceInfo = m_RenderManager->GetOffscreenRenderer()->GetPassObject<Engine::IrradianceSamplingPass>()->GetImageInfo(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
        auto prefilterInfo = m_RenderManager->GetOffscreenRenderer()->GetPassObject<Engine::HDRPrefilterPass>()->GetImageInfo(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
        auto brdfInfo = m_RenderManager->GetOffscreenRenderer()->GetPassObject<Engine::LUTPass>()->GetImageInfo(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
        auto cubeMapInfo = m_RenderManager->GetOffscreenRenderer()->GetPassObject<Engine::HDRiToCubemapPass>()->GetImageInfo(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

        auto descriptorSet = m_SceneContext->GetSceneDescriptorSet();
        
        Engine::DescriptorWriter sceneWriter(m_SceneContext->GetSceneDescriptorSetLayout(), m_SceneContext->GetSceneDescriptorPool());
        sceneWriter.writeImage(1, &shadowInfo);
        sceneWriter.writeImage(2, &irradianceInfo);
        sceneWriter.writeImage(3, &prefilterInfo);
        sceneWriter.writeImage(4, &brdfInfo);
        sceneWriter.overwrite(descriptorSet);

        // this should be done once when the HDR texture is created
        if (hdrUpdated) {
            auto info = m_HDRTexture->GetDescriptorSetInfo();
            auto testWriter = Engine::DescriptorWriter(*m_RenderManager->GetOffscreenRenderer()->GetPassObject<Engine::HDRiToCubemapPass>()->GetDescriptorSetLayout(), *m_RenderManager->GetOffscreenRenderer()->GetPassObject<Engine::HDRiToCubemapPass>()->GetDescriptorPool());
            testWriter.writeImage(0, &info);
            testWriter.overwrite(m_RenderManager->GetOffscreenRenderer()->GetPassObject<Engine::HDRiToCubemapPass>()->GetDescriptorSet());

            Engine::DescriptorWriter irradianceWriter(*m_RenderManager->GetOffscreenRenderer()->GetPassObject<Engine::IrradianceSamplingPass>()->GetDescriptorSetLayout(), *m_RenderManager->GetOffscreenRenderer()->GetPassObject<Engine::IrradianceSamplingPass>()->GetDescriptorPool());
            irradianceWriter.writeImage(0, &cubeMapInfo);
            irradianceWriter.overwrite(m_RenderManager->GetOffscreenRenderer()->GetPassObject<Engine::IrradianceSamplingPass>()->GetDescriptorSet());

            Engine::DescriptorWriter prefilterWriter(*m_RenderManager->GetOffscreenRenderer()->GetPassObject<Engine::HDRPrefilterPass>()->GetDescriptorSetLayout(), *m_RenderManager->GetOffscreenRenderer()->GetPassObject<Engine::HDRPrefilterPass>()->GetDescriptorPool());
            prefilterWriter.writeImage(0, &cubeMapInfo);
            prefilterWriter.overwrite(m_RenderManager->GetOffscreenRenderer()->GetPassObject<Engine::HDRPrefilterPass>()->GetDescriptorSet());

            Engine::DescriptorWriter cubeMapWriter(*cubemapDescriptorSetLayout, *cubemapDescriptorPool);
            cubeMapWriter.writeImage(0, &cubeMapInfo);
            cubeMapWriter.overwrite(cubemapDescriptorSet);
        }

        auto finalInfo = m_RenderManager->GetOffscreenRenderer()->GetPassObject<Engine::GeometryPass>()->GetImageInfo(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
        Engine::DescriptorWriter finalWriter(
            *m_RenderManager->GetOffscreenRenderer()->GetPassObject<Engine::CompositePass>()->GetDescriptorSetLayout(),
            *m_RenderManager->GetOffscreenRenderer()->GetPassObject<Engine::CompositePass>()->GetDescriptorPool());
        finalWriter.writeImage(0, &finalInfo);
        auto compositeDescriptorSet = m_RenderManager->GetOffscreenRenderer()->GetPassObject<Engine::CompositePass>()->GetDescriptorSet();
        finalWriter.overwrite(compositeDescriptorSet);

        m_RenderManager->GetOffscreenRenderer()->BeginRenderPass<Engine::ShadowPass>(commandBuffer, m_RenderManager->GetOffscreenRenderer()->GetPassObject<Engine::ShadowPass>());
        shadowRenderingSystem->Render(commandBuffer, *m_SceneContext);
        m_RenderManager->GetOffscreenRenderer()->EndRenderPass(commandBuffer);

        if (hdrUpdated) {
            // Render each face of the cubemap
            for (int i = 0; i < 6; i++) {
                auto view = GetCubemapViewMatrix(i);

                // Render ENV_TO_CUBEMAP pass for each face
                m_RenderManager->GetOffscreenRenderer()->BeginRenderPass<Engine::HDRiToCubemapPass>(commandBuffer,
                    m_RenderManager->GetOffscreenRenderer()->GetPassObject<Engine::HDRiToCubemapPass>(), i);
                {
                    envToCubemapRenderingSystem->Render(commandBuffer, m_RenderManager->GetOffscreenRenderer()->GetPassObject<Engine::HDRiToCubemapPass>()->GetDescriptorSet(), view);
                }
                m_RenderManager->GetOffscreenRenderer()->EndRenderPass(commandBuffer);
            }

            // Now compute irradiance for each face
            for (int i = 0; i < 6; i++) {
                auto view = GetCubemapViewMatrix(i);

                m_RenderManager->GetOffscreenRenderer()->BeginRenderPass<Engine::IrradianceSamplingPass>(commandBuffer,
                    m_RenderManager->GetOffscreenRenderer()->GetPassObject<Engine::IrradianceSamplingPass>(), i);
                {
                    irradianceRenderingSystem->Render(commandBuffer, m_RenderManager->GetOffscreenRenderer()->GetPassObject<Engine::IrradianceSamplingPass>()->GetDescriptorSet(), view);
                }
                m_RenderManager->GetOffscreenRenderer()->EndRenderPass(commandBuffer);
            }

            // Get mip levels from the pass object
            auto mipLevels = m_RenderManager->GetOffscreenRenderer()
                ->GetPassObject<Engine::HDRPrefilterPass>()->GetMipLevels();

            for (uint32_t mip = 0; mip < mipLevels; ++mip) {
                int mipExtent  = static_cast<unsigned int>(128 * std::pow(0.5, mip));

                for (uint32_t face = 0; face < 6; ++face) {
                    uint32_t framebufferIndex = mip * 6 + face;
                    auto view = GetCubemapViewMatrix(face);

                    m_RenderManager->GetOffscreenRenderer()->BeginRenderPass<Engine::HDRPrefilterPass>(
                        commandBuffer,
                        m_RenderManager->GetOffscreenRenderer()->GetPassObject<Engine::HDRPrefilterPass>(),
                        framebufferIndex, mipExtent
                    );

                    {
                        prefilterRenderingSystem->Render(commandBuffer, m_RenderManager->GetOffscreenRenderer()->GetPassObject<Engine::HDRPrefilterPass>()->GetDescriptorSet(), view, mip);
                    }

                    m_RenderManager->GetOffscreenRenderer()->EndRenderPass(commandBuffer);
                }
            }

            // 2D BRDF LUT
            m_RenderManager->GetOffscreenRenderer()->BeginRenderPass(commandBuffer,
                m_RenderManager->GetOffscreenRenderer()->GetPassObject<Engine::LUTPass>());

                brdfLutRenderingSystem->Render(commandBuffer, m_RenderManager->GetOffscreenRenderer()->GetPassObject<Engine::LUTPass>()->GetDescriptorSet());

            m_RenderManager->GetOffscreenRenderer()->EndRenderPass(commandBuffer);


            hdrUpdated = false;
        }

        m_RenderManager->GetOffscreenRenderer()->BeginRenderPass(commandBuffer,
            m_RenderManager->GetOffscreenRenderer()->GetPassObject<Engine::GeometryPass>());
            // render cubemap
            if (m_HDRTexture) {
                auto view = m_Scene->GetActiveCamera().GetView();
                auto projection = m_Scene->GetActiveCamera().GetProjection();
                cubemapRenderingSystem->Render(commandBuffer, cubemapDescriptorSet, view, projection);
            }

            // render scene
            sceneRenderingSystem->Render(commandBuffer, *m_SceneContext);
            gridRenderingSystem->Render(commandBuffer, *m_SceneContext);
        m_RenderManager->GetOffscreenRenderer()->EndRenderPass(commandBuffer);

        // transition scene from color to shader read only
        m_RenderManager->GetOffscreenRenderer()->TransitionImages(commandBuffer, Engine::RenderPassType::GEOMETRY, Engine::RenderPassType::FINAL);

        // and we render the final image
        m_RenderManager->GetOffscreenRenderer()->BeginRenderPass(commandBuffer,
            m_RenderManager->GetOffscreenRenderer()->GetPassObject<Engine::CompositePass>());
            compositeRenderingSystem->Render(commandBuffer, m_RenderManager->GetOffscreenRenderer()->GetPassObject<Engine::CompositePass>()->GetDescriptorSet());
        m_RenderManager->GetOffscreenRenderer()->EndRenderPass(commandBuffer);


        m_RenderManager->GetOffscreenRenderer()->EndFrame();
    }

    m_UI->GetFinalImage(m_RenderManager->GetOffscreenRenderer()->GetPassObject<Engine::CompositePass>()->GetDescriptorSet());

    // Render onscreen - UI
    if (auto commandBuffer = m_RenderManager->GetOnScreenRenderer()->vk_BeginFrame()) {
        m_RenderManager->GetOnScreenRenderer()->vk_BeginSwapChainRenderPass(commandBuffer);

            // Render UI
            m_UI->OnUpdate(commandBuffer, *m_SceneContext, m_FrameTime);

        // End render pass
        m_RenderManager->GetOnScreenRenderer()->vk_EndSwapChainRenderPass(commandBuffer);
        m_RenderManager->GetOnScreenRenderer()->vk_EndFrame();
    }

    vkDeviceWaitIdle(m_Device->vk_GetDevice());

    for(auto& entity : m_Scene->GetEntitesToDelete()) {
        m_Scene->DestroyEntity(entity);
    }

    m_Scene->ClearQueue();
}

void Editor::EditorLayer::OnEnd() {
    vkDeviceWaitIdle(m_Device->vk_GetDevice());
    if (m_HDRTexture) {
        m_HDRTexture->Destroy();
    }
    m_Window->CleanUp();
}
