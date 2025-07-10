#include "EditorLayer.h"

void Editor::EditorLayer::OnStart() {
    m_CurrentTime = std::chrono::steady_clock::now();

    // Initialize window
    m_Window = std::make_unique<Engine::Window>(800, 600, "Aquila Editor");
    m_Device = std::make_unique<Engine::Device>(*m_Window);

    Engine::DescriptorAllocator::Init(*m_Device);

    // Initialize renderers
    m_RenderManager = std::make_unique<Engine::RenderManager>(*m_Device, *m_Window);

    // Initialize scene
    // m_Scene = std::make_shared<Engine::Scene>();

    m_SceneManager = std::make_shared<Engine::SceneManager>();
    m_SceneManager->EnqueueScene(std::make_unique<Engine::AquilaScene>("Default Scene"));
    m_SceneManager->RequestSceneChange();
    m_SceneManager->ProcessSceneChange();

    m_EditorCamera = std::make_unique<Engine::EditorCamera>();
    m_EditorCamera->SetPerspectiveProjection(40.f, 1.778f, 0.1f, 100.f);
    m_EditorCamera->SetPosition(glm::vec3(0, 1, -10));
    m_EditorCamera->SetViewYXZ(m_EditorCamera->GetPosition(), m_EditorCamera->GetRotation());
    
    // if scene exists -> register events
    if (m_SceneManager->GetActiveScene() != nullptr) m_EventRegistry->RegisterHandlers(m_Device.get(), m_SceneManager, m_RenderManager->GetOffscreenRenderer().get());

    // m_SceneContext = std::make_unique<Engine::SceneContext>(*m_Device, *m_Scene);


    // std::array<VkDescriptorSetLayout, 2> setLayouts =
    //     {m_SceneContext->GetSceneDescriptorSetLayout().GetDescriptorSetLayout(),
    //     m_SceneContext->GetMaterialDescriptorSetLayout().GetDescriptorSetLayout()};

    // Initialize scene rendering system
    InitializeRenderingSystems();

    // Initialize UI
    m_UI = std::make_unique<UIManager>(*m_Device,
        *m_Window,
        m_RenderManager->GetImGuiRenderPass());


    // m_Scene->GetActiveCamera().SetPerspectiveProjection(80.f, m_RenderManager->GetOffscreenRenderer()->GetAspectRatio(), 0.1f, 100.f);
    // m_Scene->GetActiveCamera().SetPosition(glm::vec3(0,1,-10));
    // m_Scene->GetActiveCamera().SetViewYXZ(m_Scene->GetActiveCamera().GetPosition(), glm::vec3(0,0,0));
    

    // auto commandBuffer = m_Device->vk_BeginSingleTimeCommands();

    // m_Scene->UseEnvMap(false);
    // SampleIBL(commandBuffer);

    // m_Device->vk_EndSingleTimeCommands(commandBuffer);

    std::cout << "Everything went well, and is initialized" << std::endl;
}


void Editor::EditorLayer::OnUpdate() {
    auto newTime = std::chrono::steady_clock::now();
    m_FrameTime = std::chrono::duration<float>(newTime - m_CurrentTime).count();
    m_CurrentTime = newTime;

    // Poll events
    m_Window->PollEvents();

    if (m_RenderManager->GetOffscreenRenderer()->Resized()){
        m_RenderManager->GetOffscreenRenderer()->InvalidatePasses();
    }

    if (m_SceneManager->HasPendingSceneChange()) {
        m_SceneManager->ProcessSceneChange();

        Engine::EventBus::Get().Clear(); // clear previous event handlers before registering new ones

        m_EventRegistry->RegisterHandlers(m_Device.get(), m_SceneManager, m_RenderManager->GetOffscreenRenderer().get());
    }

    // Render offscreen - Scene
    if (auto commandBuffer = m_RenderManager->GetOffscreenRenderer()->BeginFrame()) {

        m_RenderManager->GetOffscreenRenderer()->BeginRenderPass<Engine::GeometryPass>(commandBuffer, m_RenderManager->GetOffscreenRenderer()->GetPassObject<Engine::GeometryPass>());
            
            // setup rendering context
            Engine::SceneRenderingSystem_new::RenderContext context{};
            context.commandBuffer = commandBuffer;
            context.scene = m_SceneManager->GetActiveScene();
            context.camera = m_EditorCamera.get();

            
            m_SceneRendering->Render(context);
        m_RenderManager->GetOffscreenRenderer()->EndRenderPass(commandBuffer);

        // // get images for shadows, ibl and cubemap rendering
        // auto shadowInfo = m_RenderManager->GetOffscreenRenderer()->GetPassObject<Engine::ShadowPass>()->GetImageInfo(VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL);
        // auto irradianceInfo = m_RenderManager->GetOffscreenRenderer()->GetPassObject<Engine::IrradianceSamplingPass>()->GetImageInfo(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
        // auto prefilterInfo = m_RenderManager->GetOffscreenRenderer()->GetPassObject<Engine::HDRPrefilterPass>()->GetImageInfo(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
        // auto brdfInfo = m_RenderManager->GetOffscreenRenderer()->GetPassObject<Engine::LUTPass>()->GetImageInfo(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
        // auto cubeMapInfo = m_RenderManager->GetOffscreenRenderer()->GetPassObject<Engine::HDRiToCubemapPass>()->GetImageInfo(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

        // auto descriptorSet = m_SceneContext->GetSceneDescriptorSet();
        
        // Engine::DescriptorWriter sceneWriter(m_SceneContext->GetSceneDescriptorSetLayout(), m_SceneContext->GetSceneDescriptorPool());
        // sceneWriter.writeImage(1, &shadowInfo);
        // sceneWriter.writeImage(2, &irradianceInfo);
        // sceneWriter.writeImage(3, &prefilterInfo);
        // sceneWriter.writeImage(4, &brdfInfo);
        // sceneWriter.overwrite(descriptorSet);

        // auto finalInfo = m_RenderManager->GetOffscreenRenderer()->GetPassObject<Engine::GeometryPass>()->GetImageInfo(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
        // Engine::DescriptorWriter finalWriter(
        //     *m_RenderManager->GetOffscreenRenderer()->GetPassObject<Engine::CompositePass>()->GetDescriptorSetLayout(),
        //     *Engine::DescriptorAllocator::GetSharedPool());
        // finalWriter.writeImage(0, &finalInfo);
        // auto compositeDescriptorSet = m_RenderManager->GetOffscreenRenderer()->GetPassObject<Engine::CompositePass>()->GetDescriptorSet();
        // finalWriter.overwrite(compositeDescriptorSet);

        // m_RenderManager->GetOffscreenRenderer()->BeginRenderPass<Engine::ShadowPass>(commandBuffer, m_RenderManager->GetOffscreenRenderer()->GetPassObject<Engine::ShadowPass>());
        // m_ShadowRenderingSystem->Render(commandBuffer, *m_SceneContext);
        // m_RenderManager->GetOffscreenRenderer()->EndRenderPass(commandBuffer);

        // if (hdrUpdated) {
        //     m_Scene->UseEnvMap(true);
        //     auto hdrImageInfo = m_HDRTexture->GetDescriptorSetInfo();
        //     auto hdrWriter = Engine::DescriptorWriter(m_RenderManager->GetOffscreenRenderer()->GetSharedDescriptorSetLayout(),
        //         *Engine::DescriptorAllocator::GetSharedPool());
        //     hdrWriter.writeImage(0, &hdrImageInfo);
        //     hdrWriter.overwrite(m_RenderManager->GetOffscreenRenderer()->GetPassObject<Engine::HDRiToCubemapPass>()->GetDescriptorSet());

        //     SampleIBL(commandBuffer);
        //     hdrUpdated = false;
        // }

        // m_RenderManager->GetOffscreenRenderer()->BeginRenderPass(commandBuffer,
        //     m_RenderManager->GetOffscreenRenderer()->GetPassObject<Engine::GeometryPass>());
        //     // render cubemap
        //     auto view = m_Scene->GetActiveCamera().GetView();
        //     auto projection = m_Scene->GetActiveCamera().GetProjection();
        //     m_CubemapRenderingSystem->Render(commandBuffer, m_RenderManager->GetOffscreenRenderer()->GetPassObject<Engine::HDRiToCubemapPass>()->GetDescriptorSet(), view, projection);

        //     // render scene
        //     m_SceneRenderingSystem->Render(commandBuffer, *m_SceneContext);
        //     m_GridRenderingSystem->Render(commandBuffer, *m_SceneContext);
        // m_RenderManager->GetOffscreenRenderer()->EndRenderPass(commandBuffer);

        // // transition scene from color to shader read only
        // m_RenderManager->GetOffscreenRenderer()->TransitionImages(commandBuffer, Engine::RenderPassType::GEOMETRY, Engine::RenderPassType::FINAL);

        // // and we render the final image
        // m_RenderManager->GetOffscreenRenderer()->BeginRenderPass(commandBuffer,
        //     m_RenderManager->GetOffscreenRenderer()->GetPassObject<Engine::CompositePass>());
        //     m_CompositeRenderingSystem->Render(commandBuffer, m_RenderManager->GetOffscreenRenderer()->GetPassObject<Engine::CompositePass>()->GetDescriptorSet());
        // m_RenderManager->GetOffscreenRenderer()->EndRenderPass(commandBuffer);


        m_RenderManager->GetOffscreenRenderer()->EndFrame();
    }

    m_UI->GetFinalImage(m_RenderManager->GetOffscreenRenderer()->GetPassObject<Engine::GeometryPass>()->GetDescriptorSet());

    // Render onscreen - UI
    if (auto commandBuffer = m_RenderManager->GetOnScreenRenderer()->vk_BeginFrame()) {
        m_RenderManager->GetOnScreenRenderer()->vk_BeginSwapChainRenderPass(commandBuffer);

            // Render UI
            m_UI->OnUpdate(commandBuffer, m_FrameTime, m_SceneManager->GetActiveScene());

        // End render pass
        m_RenderManager->GetOnScreenRenderer()->vk_EndSwapChainRenderPass(commandBuffer);
        m_RenderManager->GetOnScreenRenderer()->vk_EndFrame();
    }

    vkDeviceWaitIdle(m_Device->vk_GetDevice());

    // for(auto& entity : m_Scene->GetEntitesToDelete()) {
    //     m_Scene->DestroyEntity(entity);
    // }

    // m_Scene->ClearQueue();
}

void Editor::EditorLayer::OnEnd() {
    vkDeviceWaitIdle(m_Device->vk_GetDevice());
    if (m_HDRTexture) {
        m_HDRTexture->Destroy();
    }
    m_Window->CleanUp();
}

void Editor::EditorLayer::InitializeRenderingSystems(){
    // std::array<VkDescriptorSetLayout, 2> setLayouts =
    //     {m_SceneContext->GetSceneDescriptorSetLayout().GetDescriptorSetLayout(),
    //     m_SceneContext->GetMaterialDescriptorSetLayout().GetDescriptorSetLayout()};

    // // Initialize scene rendering system

    // // SCENE SYSTEMS
    // m_SceneRenderingSystem = std::make_shared<Engine::SceneRenderingSystem>(*m_Device,
    //     m_RenderManager->GetOffscreenRenderer()->GetPassObject<Engine::GeometryPass>()->GetRenderPass(),
    //     setLayouts);

    // m_GridRenderingSystem = std::make_shared<Engine::GridRenderingSystem>(*m_Device,
    //      m_RenderManager->GetOffscreenRenderer()->GetPassObject<Engine::GeometryPass>()->GetRenderPass(),
    //      setLayouts);

    // m_ShadowRenderingSystem = std::make_shared<Engine::DepthRenderingSystem>(*m_Device,
    //     m_RenderManager->GetOffscreenRenderer()->GetPassObject<Engine::ShadowPass>()->GetRenderPass(),
    //     setLayouts);


    // // HDRi SYSTEMS
    // m_EnvToCubemapRenderingSystem = std::make_shared<Engine::EnvToCubemapRenderingSystem>(*m_Device,
    //     m_RenderManager->GetOffscreenRenderer()->GetPassObject<Engine::HDRiToCubemapPass>()->GetRenderPass(),
    //     m_RenderManager->GetOffscreenRenderer()->GetSharedDescriptorSetLayout().GetDescriptorSetLayout());

    // m_CubemapRenderingSystem = std::make_shared<Engine::CubemapRenderingSystem>(*m_Device,
    //     m_RenderManager->GetOffscreenRenderer()->GetPassObject<Engine::GeometryPass>()->GetRenderPass(),
    //     m_RenderManager->GetOffscreenRenderer()->GetSharedDescriptorSetLayout().GetDescriptorSetLayout());

    // m_IrradianceRenderingSystem = std::make_shared<Engine::IrradianceRenderingSystem>(*m_Device,
    //     m_RenderManager->GetOffscreenRenderer()->GetPassObject<Engine::IrradianceSamplingPass>()->GetRenderPass(),
    //     m_RenderManager->GetOffscreenRenderer()->GetSharedDescriptorSetLayout().GetDescriptorSetLayout());

    // m_PrefilterRenderingSystem = std::make_shared<Engine::PrefilterRenderingSystem>(*m_Device,
    //     m_RenderManager->GetOffscreenRenderer()->GetPassObject<Engine::HDRPrefilterPass>()->GetRenderPass(),
    //     m_RenderManager->GetOffscreenRenderer()->GetSharedDescriptorSetLayout().GetDescriptorSetLayout());

    // m_PreethamSkyRenderingSystem = std::make_shared<Engine::PreethamSkyRenderingSystem>(*m_Device,
    //     m_RenderManager->GetOffscreenRenderer()->GetPassObject<Engine::PreethamSkyPass>()->GetRenderPass(),
    //     m_RenderManager->GetOffscreenRenderer()->GetSharedDescriptorSetLayout().GetDescriptorSetLayout());

    // m_BRDFLutRenderingSystem = std::make_shared<Engine::BRDFLutRenderingSystem>(*m_Device,
    //     m_RenderManager->GetOffscreenRenderer()->GetPassObject<Engine::LUTPass>()->GetRenderPass(),
    //     m_RenderManager->GetOffscreenRenderer()->GetSharedDescriptorSetLayout().GetDescriptorSetLayout());

    // // FINAL/COMPOSITE SYSTEM
    // m_CompositeRenderingSystem = std::make_shared<Engine::CompositeRenderingSystem>(*m_Device,
    //     m_RenderManager->GetOffscreenRenderer()->GetPassObject<Engine::CompositePass>()->GetRenderPass(),
    //     m_RenderManager->GetOffscreenRenderer()->GetPassObject<Engine::CompositePass>()->GetDescriptorSetLayout()->GetDescriptorSetLayout());

    m_SceneRendering = std::make_shared<Engine::SceneRenderingSystem_new>(*m_Device, 
        m_RenderManager->GetOffscreenRenderer()->GetPassObject<Engine::GeometryPass>()->GetRenderPass());
}

void Editor::EditorLayer::SampleIBL(VkCommandBuffer commandBuffer){
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

    // Render preetham sky to each face of the cubemap
    for (int i = 0; i < 6; i++) {
        auto view = glm::lookAt(glm::vec3(0.0f), directions[i], ups[i]);;

        m_RenderManager->GetOffscreenRenderer()->BeginRenderPass<Engine::PreethamSkyPass>(commandBuffer,
            m_RenderManager->GetOffscreenRenderer()->GetPassObject<Engine::PreethamSkyPass>(), i);
        {
            // preetham sky pass
            m_PreethamSkyRenderingSystem->Render(commandBuffer, m_RenderManager->GetOffscreenRenderer()->GetPassObject<Engine::PreethamSkyPass>()->GetDescriptorSet(), view);
        }
        m_RenderManager->GetOffscreenRenderer()->EndRenderPass(commandBuffer);
    }

    auto cubeMapInfo = m_RenderManager->GetOffscreenRenderer()->GetPassObject<Engine::PreethamSkyPass>()->GetImageInfo(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

    Engine::DescriptorWriter irradianceWriter(m_RenderManager->GetOffscreenRenderer()->GetSharedDescriptorSetLayout(),
        *Engine::DescriptorAllocator::GetSharedPool());
    irradianceWriter.writeImage(0, &cubeMapInfo);
    irradianceWriter.overwrite(m_RenderManager->GetOffscreenRenderer()->GetPassObject<Engine::IrradianceSamplingPass>()->GetDescriptorSet());

    Engine::DescriptorWriter prefilterWriter(m_RenderManager->GetOffscreenRenderer()->GetSharedDescriptorSetLayout(),
        *Engine::DescriptorAllocator::GetSharedPool());
    prefilterWriter.writeImage(0, &cubeMapInfo);
    prefilterWriter.overwrite(m_RenderManager->GetOffscreenRenderer()->GetPassObject<Engine::HDRPrefilterPass>()->GetDescriptorSet());

    Engine::DescriptorWriter cubeMapWriter(m_RenderManager->GetOffscreenRenderer()->GetSharedDescriptorSetLayout(),
        *Engine::DescriptorAllocator::GetSharedPool());
    cubeMapWriter.writeImage(0, &cubeMapInfo);
    cubeMapWriter.overwrite(m_RenderManager->GetOffscreenRenderer()->GetPassObject<Engine::HDRiToCubemapPass>()->GetDescriptorSet());
    
    // Now compute irradiance for each face
    for (int i = 0; i < 6; i++) {
        auto view = glm::lookAt(glm::vec3(0.0f), directions[i], ups[i]);;

        m_RenderManager->GetOffscreenRenderer()->BeginRenderPass<Engine::IrradianceSamplingPass>(commandBuffer,
            m_RenderManager->GetOffscreenRenderer()->GetPassObject<Engine::IrradianceSamplingPass>(), i);
        {
            m_IrradianceRenderingSystem->Render(commandBuffer, m_RenderManager->GetOffscreenRenderer()->GetPassObject<Engine::IrradianceSamplingPass>()->GetDescriptorSet(), view);
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
            auto view = glm::lookAt(glm::vec3(0.0f), directions[face], ups[face]);;

            m_RenderManager->GetOffscreenRenderer()->BeginRenderPass<Engine::HDRPrefilterPass>(
                commandBuffer,
                m_RenderManager->GetOffscreenRenderer()->GetPassObject<Engine::HDRPrefilterPass>(),
                framebufferIndex, mipExtent
            );

            {
                m_PrefilterRenderingSystem->Render(commandBuffer, m_RenderManager->GetOffscreenRenderer()->GetPassObject<Engine::HDRPrefilterPass>()->GetDescriptorSet(), view, mip);
            }

            m_RenderManager->GetOffscreenRenderer()->EndRenderPass(commandBuffer);
        }
    }

    // 2D BRDF LUT
    m_RenderManager->GetOffscreenRenderer()->BeginRenderPass(commandBuffer,
        m_RenderManager->GetOffscreenRenderer()->GetPassObject<Engine::LUTPass>());

        m_BRDFLutRenderingSystem->Render(commandBuffer, m_RenderManager->GetOffscreenRenderer()->GetPassObject<Engine::LUTPass>()->GetDescriptorSet());

    m_RenderManager->GetOffscreenRenderer()->EndRenderPass(commandBuffer);
}

