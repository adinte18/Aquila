#include "EditorLayer.h"


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
    m_EventDispatcher = std::make_unique<EventDispatcher>();

    // Testing purposes
    m_EventDispatcher->RegisterHandler<AddEntityEvent>([this](const AddEntityEvent& event) {
        this->HandleAddEntity();
    });

    m_EventDispatcher->RegisterHandler<RemoveEntityEvent>([this](const RemoveEntityEvent& event) {
        this->HandleRemoveEntity(event);
    });

    m_EventDispatcher->RegisterHandler<AddEnvMapEvent>([this](const AddEnvMapEvent& event) {
        this->HandleAddEnvMap(event.GetTexturePath());
    });


    std::array<VkDescriptorSetLayout, 2> setLayouts =
        {m_SceneContext->GetSceneDescriptorSetLayout().getDescriptorSetLayout(),
        m_SceneContext->GetMaterialDescriptorSetLayout().getDescriptorSetLayout()};

    // TODO : HDR TESTING - TO DELETE
    // create new descriptor set layout
    envToCubemapDescriptorPool = Engine::DescriptorPool::Builder(*m_Device)
            .setMaxSets(1)
            .addPoolSize(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1)
            .build();

    envToCubemapDescriptorSetLayout = Engine::DescriptorSetLayout::Builder(*m_Device)
             .addBinding(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT)
             .build();
    envToCubemapDescriptorPool->allocateDescriptor(envToCubemapDescriptorSetLayout->getDescriptorSetLayout(), envToCubemapDescriptorSet);

    cubemapDescriptorPool = Engine::DescriptorPool::Builder(*m_Device)
            .setMaxSets(1)
            .addPoolSize(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1)
            .build();

    cubemapDescriptorSetLayout = Engine::DescriptorSetLayout::Builder(*m_Device)
             .addBinding(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT)
             .build();
    cubemapDescriptorPool->allocateDescriptor(cubemapDescriptorSetLayout->getDescriptorSetLayout(), cubemapDescriptorSet);

    irradianceDescriptorPool = Engine::DescriptorPool::Builder(*m_Device)
            .setMaxSets(1)
            .addPoolSize(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1)
            .build();

    irradianceDescriptorSetLayout = Engine::DescriptorSetLayout::Builder(*m_Device)
             .addBinding(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT)
             .build();
    irradianceDescriptorPool->allocateDescriptor(irradianceDescriptorSetLayout->getDescriptorSetLayout(), irradianceDescriptorSet);


    // Initialize scene rendering system


    sceneRenderingSystem = std::make_shared<RenderingSystem::SceneRenderingSystem>(*m_Device,
        m_RenderManager->GetOffscreenRenderer()->GetRenderPass(Engine::RenderPassType::GEOMETRY),
        setLayouts);

    envToCubemapRenderingSystem = std::make_shared<RenderingSystem::EnvToCubemapRenderingSystem>(*m_Device,
        m_RenderManager->GetOffscreenRenderer()->GetRenderPass(Engine::RenderPassType::ENV_TO_CUBEMAP),
        envToCubemapDescriptorSetLayout->getDescriptorSetLayout());

    cubemapRenderingSystem = std::make_shared<RenderingSystem::CubemapRenderingSystem>(*m_Device,
        m_RenderManager->GetOffscreenRenderer()->GetRenderPass(Engine::RenderPassType::GEOMETRY),
        cubemapDescriptorSetLayout->getDescriptorSetLayout());

    irradianceRenderingSystem = std::make_shared<RenderingSystem::IrradianceRenderingSystem>(*m_Device,
        m_RenderManager->GetOffscreenRenderer()->GetRenderPass(Engine::RenderPassType::IBL),
        irradianceDescriptorSetLayout->getDescriptorSetLayout());

    gridRenderingSystem = std::make_shared<RenderingSystem::GridRenderingSystem>(*m_Device,
         m_RenderManager->GetOffscreenRenderer()->GetRenderPass(Engine::RenderPassType::GEOMETRY),
         setLayouts);

    shadowRenderingSystem = std::make_shared<RenderingSystem::DepthRenderingSystem>(*m_Device,
        m_RenderManager->GetOffscreenRenderer()->GetRenderPass(Engine::RenderPassType::SHADOW),
        setLayouts);

    postprocessingRenderingSystem = std::make_shared<RenderingSystem::PPRenderingSystem>(*m_Device,
        m_RenderManager->GetOffscreenRenderer()->GetRenderPass(Engine::RenderPassType::POST_PROCESSING),
        m_RenderManager->GetOffscreenRenderer()->GetRenderPassDescriptorLayout(Engine::RenderPassType::POST_PROCESSING)->getDescriptorSetLayout());

    compositeRenderingSystem = std::make_shared<RenderingSystem::CompositeRenderingSystem>(*m_Device,
        m_RenderManager->GetOffscreenRenderer()->GetRenderPass(Engine::RenderPassType::FINAL),
        m_RenderManager->GetOffscreenRenderer()->GetRenderPassDescriptorLayout(Engine::RenderPassType::FINAL)->getDescriptorSetLayout());

    // Initialize UI
    m_UI = std::make_unique<UIManager>(*m_Device,
        *m_Window,
        m_RenderManager->GetOnScreenRenderer()->vk_GetCurrentRenderPass(), *m_EventDispatcher, m_Scene->GetRegistry());

    m_Scene->GetActiveCamera().SetPerspectiveProjection(glm::radians(80.f), m_RenderManager->GetOffscreenRenderer()->GetAspectRatio(), 1.0f, 100.f);
    m_Scene->GetActiveCamera().SetPosition(glm::vec3(0,1,-10));
    m_Scene->GetActiveCamera().SetViewYXZ(m_Scene->GetActiveCamera().GetPosition(), glm::vec3(0,0,0));
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

    // Update scene
    m_SceneContext->UpdateGPUData();

    // Render offscreen - Scene
    if (auto commandBuffer = m_RenderManager->GetOffscreenRenderer()->BeginFrame()) {

        // get images for shadows, ibl and cubemap rendering
        auto shadowInfo = m_RenderManager->GetOffscreenRenderer()->GetImageInfo(Engine::RenderPassType::SHADOW, VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL);
        auto irradianceInfo = m_RenderManager->GetOffscreenRenderer()->GetImageInfo(Engine::RenderPassType::IBL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
        auto cubeMapInfo = m_RenderManager->GetOffscreenRenderer()->GetImageInfo(Engine::RenderPassType::ENV_TO_CUBEMAP, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
        auto descriptorSet = m_SceneContext->GetSceneDescriptorSet();
        Engine::DescriptorWriter sceneWriter(m_SceneContext->GetSceneDescriptorSetLayout(), m_SceneContext->GetSceneDescriptorPool());
        sceneWriter.writeImage(1, &shadowInfo);
        sceneWriter.writeImage(2, &irradianceInfo);
        sceneWriter.overwrite(descriptorSet);

        // this should be done once when the HDR texture is created
        if (hdrUpdated) {
            auto info = m_HDRTexture->GetDescriptorSetInfo();
            auto testWriter = Engine::DescriptorWriter(*envToCubemapDescriptorSetLayout, *envToCubemapDescriptorPool);
            testWriter.writeImage(0, &info);
            testWriter.overwrite(envToCubemapDescriptorSet);

            Engine::DescriptorWriter cubeMapWriter(*cubemapDescriptorSetLayout, *cubemapDescriptorPool);
            cubeMapWriter.writeImage(0, &cubeMapInfo);
            cubeMapWriter.overwrite(cubemapDescriptorSet);

            Engine::DescriptorWriter irradianceWriter(*irradianceDescriptorSetLayout, *irradianceDescriptorPool);
            irradianceWriter.writeImage(0, &cubeMapInfo);
            irradianceWriter.overwrite(irradianceDescriptorSet);
        }

        auto sceneInfo = m_RenderManager->GetOffscreenRenderer()->GetImageInfo(Engine::RenderPassType::GEOMETRY, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
        Engine::DescriptorWriter ppWriter(*m_RenderManager->GetOffscreenRenderer()->GetRenderPassDescriptorLayout(Engine::RenderPassType::POST_PROCESSING), *m_RenderManager->GetOffscreenRenderer()->GetRenderPassDescriptorPool(Engine::RenderPassType::POST_PROCESSING));
        ppWriter.writeImage(0, &sceneInfo);
        ppWriter.overwrite(m_RenderManager->GetOffscreenRenderer()->GetRenderPassDescriptorSet(Engine::RenderPassType::POST_PROCESSING));

        auto finalInfo = m_RenderManager->GetOffscreenRenderer()->GetImageInfo(Engine::RenderPassType::POST_PROCESSING, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
        Engine::DescriptorWriter finalWriter(*m_RenderManager->GetOffscreenRenderer()->GetRenderPassDescriptorLayout(Engine::RenderPassType::FINAL), *m_RenderManager->GetOffscreenRenderer()->GetRenderPassDescriptorPool(Engine::RenderPassType::FINAL));
        finalWriter.writeImage(0, &finalInfo);
        finalWriter.overwrite(m_RenderManager->GetOffscreenRenderer()->GetRenderPassDescriptorSet(Engine::RenderPassType::FINAL));

        m_RenderManager->GetOffscreenRenderer()->BeginRenderPass(commandBuffer, Engine::RenderPassType::SHADOW);
        shadowRenderingSystem->Render(commandBuffer, *m_SceneContext);
        m_RenderManager->GetOffscreenRenderer()->EndRenderPass(commandBuffer);

        if (hdrUpdated) {
            // Render each face of the cubemap
            for (int i = 0; i < 6; i++) {
                auto view = GetCubemapViewMatrix(i);

                // Render ENV_TO_CUBEMAP pass for each face
                m_RenderManager->GetOffscreenRenderer()->BeginRenderPass(commandBuffer, Engine::RenderPassType::ENV_TO_CUBEMAP, i);
                {
                    envToCubemapRenderingSystem->Render(commandBuffer, envToCubemapDescriptorSet, view);
                }
                m_RenderManager->GetOffscreenRenderer()->EndRenderPass(commandBuffer);
            }

            // Now compute irradiance for each face
            for (int i = 0; i < 6; i++) {
                auto view = GetCubemapViewMatrix(i);

                m_RenderManager->GetOffscreenRenderer()->BeginRenderPass(commandBuffer, Engine::RenderPassType::IBL, i);
                {
                    irradianceRenderingSystem->Render(commandBuffer, irradianceDescriptorSet, view);
                }
                m_RenderManager->GetOffscreenRenderer()->EndRenderPass(commandBuffer);
            }

            hdrUpdated = false;
        }

        m_RenderManager->GetOffscreenRenderer()->BeginRenderPass(commandBuffer, Engine::RenderPassType::GEOMETRY);
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
        m_RenderManager->GetOffscreenRenderer()->TransitionImages(commandBuffer, Engine::RenderPassType::GEOMETRY, Engine::RenderPassType::POST_PROCESSING);

        // here we do post-processing on the scene image and we store the image as color
        m_RenderManager->GetOffscreenRenderer()->BeginRenderPass(commandBuffer, Engine::RenderPassType::POST_PROCESSING);
            postprocessingRenderingSystem->Render(commandBuffer, m_RenderManager->GetOffscreenRenderer()->GetRenderPassDescriptorSet(Engine::RenderPassType::POST_PROCESSING));
        m_RenderManager->GetOffscreenRenderer()->EndRenderPass(commandBuffer);

        // now again - we transition from color to shader read only for the gui to use it
        m_RenderManager->GetOffscreenRenderer()->TransitionImages(commandBuffer, Engine::RenderPassType::POST_PROCESSING, Engine::RenderPassType::FINAL);

        // and we render the final image
        m_RenderManager->GetOffscreenRenderer()->BeginRenderPass(commandBuffer, Engine::RenderPassType::FINAL);
            compositeRenderingSystem->Render(commandBuffer, m_RenderManager->GetOffscreenRenderer()->GetRenderPassDescriptorSet(Engine::RenderPassType::FINAL));
        m_RenderManager->GetOffscreenRenderer()->EndRenderPass(commandBuffer);


        m_RenderManager->GetOffscreenRenderer()->EndFrame();
    }

    m_UI->GetFinalImage(m_RenderManager->GetOffscreenRenderer()->GetRenderPassDescriptorSet(Engine::RenderPassType::FINAL));

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
