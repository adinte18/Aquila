#include "EditorLayer.h"

void Editor::EditorLayer::OnStart() {
    Engine::Core::Get().GetClock().Reset();


    m_EditorCamera = std::make_unique<Engine::EditorCamera>();
    m_EditorCamera->SetPerspectiveProjection(40.f, 1.778f, 0.1f, 100.f);
    m_EditorCamera->SetPosition(glm::vec3(0, 1, -10));
    m_EditorCamera->SetViewYXZ(m_EditorCamera->GetPosition(), m_EditorCamera->GetRotation());

    InitializeRenderingSystems();

    m_UI = std::make_unique<UIManager>();

    // auto commandBuffer = m_Device->vk_BeginSingleTimeCommands();

    // m_Scene->UseEnvMap(false);
    // SampleIBL(commandBuffer);

    // m_Device->vk_EndSingleTimeCommands(commandBuffer);

    std::cout << "Everything went well, and is initialized" << std::endl;
}


void Editor::EditorLayer::OnUpdate() {
    // Poll events and calculate delta time
    Engine::Core::Get().OnUpdate();

    // If viewport is resized, invalidate passes (recreate basically)
    Engine::Core::Get().InvalidatePasses();

    // Handle scene change
    Engine::Core::Get().HandleSceneChange();

    // Render offscreen - Scene
    if (auto commandBuffer = Engine::Core::Get().GetRenderManager().GetOffscreenRenderer()->BeginFrame()) {

        Engine::Core::Get().GetRenderManager().GetOffscreenRenderer()->BeginRenderPass<Engine::GeometryPass>(commandBuffer, Engine::Core::Get().GetRenderManager().GetOffscreenRenderer()->GetPassObject<Engine::GeometryPass>());
            
            // setup rendering context
            Engine::SceneRenderingSystem_new::RenderContext context{};
            context.commandBuffer = commandBuffer;
            context.scene = Engine::Core::Get().GetSceneManager().GetActiveScene();
            context.camera = m_EditorCamera.get();

            
            m_SceneRendering->Render(context);
        Engine::Core::Get().GetRenderManager().GetOffscreenRenderer()->EndRenderPass(commandBuffer);

        // // get images for shadows, ibl and cubemap rendering
        // auto shadowInfo = Engine::Core::Get().GetRenderManager().GetOffscreenRenderer()->GetPassObject<Engine::ShadowPass>()->GetImageInfo(VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL);
        // auto irradianceInfo = Engine::Core::Get().GetRenderManager().GetOffscreenRenderer()->GetPassObject<Engine::IrradianceSamplingPass>()->GetImageInfo(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
        // auto prefilterInfo = Engine::Core::Get().GetRenderManager().GetOffscreenRenderer()->GetPassObject<Engine::HDRPrefilterPass>()->GetImageInfo(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
        // auto brdfInfo = Engine::Core::Get().GetRenderManager().GetOffscreenRenderer()->GetPassObject<Engine::LUTPass>()->GetImageInfo(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
        // auto cubeMapInfo = Engine::Core::Get().GetRenderManager().GetOffscreenRenderer()->GetPassObject<Engine::HDRiToCubemapPass>()->GetImageInfo(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

        // auto descriptorSet = m_SceneContext->GetSceneDescriptorSet();
        
        // Engine::DescriptorWriter sceneWriter(m_SceneContext->GetSceneDescriptorSetLayout(), m_SceneContext->GetSceneDescriptorPool());
        // sceneWriter.writeImage(1, &shadowInfo);
        // sceneWriter.writeImage(2, &irradianceInfo);
        // sceneWriter.writeImage(3, &prefilterInfo);
        // sceneWriter.writeImage(4, &brdfInfo);
        // sceneWriter.overwrite(descriptorSet);

        // auto finalInfo = Engine::Core::Get().GetRenderManager().GetOffscreenRenderer()->GetPassObject<Engine::GeometryPass>()->GetImageInfo(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
        // Engine::DescriptorWriter finalWriter(
        //     *Engine::Core::Get().GetRenderManager().GetOffscreenRenderer()->GetPassObject<Engine::CompositePass>()->GetDescriptorSetLayout(),
        //     *Engine::DescriptorAllocator::GetSharedPool());
        // finalWriter.writeImage(0, &finalInfo);
        // auto compositeDescriptorSet = Engine::Core::Get().GetRenderManager().GetOffscreenRenderer()->GetPassObject<Engine::CompositePass>()->GetDescriptorSet();
        // finalWriter.overwrite(compositeDescriptorSet);

        // Engine::Core::Get().GetRenderManager().GetOffscreenRenderer()->BeginRenderPass<Engine::ShadowPass>(commandBuffer, Engine::Core::Get().GetRenderManager().GetOffscreenRenderer()->GetPassObject<Engine::ShadowPass>());
        // m_ShadowRenderingSystem->Render(commandBuffer, *m_SceneContext);
        // Engine::Core::Get().GetRenderManager().GetOffscreenRenderer()->EndRenderPass(commandBuffer);

        // if (hdrUpdated) {
        //     m_Scene->UseEnvMap(true);
        //     auto hdrImageInfo = m_HDRTexture->GetDescriptorSetInfo();
        //     auto hdrWriter = Engine::DescriptorWriter(Engine::Core::Get().GetRenderManager().GetOffscreenRenderer()->GetSharedDescriptorSetLayout(),
        //         *Engine::DescriptorAllocator::GetSharedPool());
        //     hdrWriter.writeImage(0, &hdrImageInfo);
        //     hdrWriter.overwrite(Engine::Core::Get().GetRenderManager().GetOffscreenRenderer()->GetPassObject<Engine::HDRiToCubemapPass>()->GetDescriptorSet());

        //     SampleIBL(commandBuffer);
        //     hdrUpdated = false;
        // }

        // Engine::Core::Get().GetRenderManager().GetOffscreenRenderer()->BeginRenderPass(commandBuffer,
        //     Engine::Core::Get().GetRenderManager().GetOffscreenRenderer()->GetPassObject<Engine::GeometryPass>());
        //     // render cubemap
        //     auto view = m_Scene->GetActiveCamera().GetView();
        //     auto projection = m_Scene->GetActiveCamera().GetProjection();
        //     m_CubemapRenderingSystem->Render(commandBuffer, Engine::Core::Get().GetRenderManager().GetOffscreenRenderer()->GetPassObject<Engine::HDRiToCubemapPass>()->GetDescriptorSet(), view, projection);

        //     // render scene
        //     m_SceneRenderingSystem->Render(commandBuffer, *m_SceneContext);
        //     m_GridRenderingSystem->Render(commandBuffer, *m_SceneContext);
        // Engine::Core::Get().GetRenderManager().GetOffscreenRenderer()->EndRenderPass(commandBuffer);

        // // transition scene from color to shader read only
        // Engine::Core::Get().GetRenderManager().GetOffscreenRenderer()->TransitionImages(commandBuffer, Engine::RenderPassType::GEOMETRY, Engine::RenderPassType::FINAL);

        // // and we render the final image
        // Engine::Core::Get().GetRenderManager().GetOffscreenRenderer()->BeginRenderPass(commandBuffer,
        //     Engine::Core::Get().GetRenderManager().GetOffscreenRenderer()->GetPassObject<Engine::CompositePass>());
        //     m_CompositeRenderingSystem->Render(commandBuffer, Engine::Core::Get().GetRenderManager().GetOffscreenRenderer()->GetPassObject<Engine::CompositePass>()->GetDescriptorSet());
        // Engine::Core::Get().GetRenderManager().GetOffscreenRenderer()->EndRenderPass(commandBuffer);


        Engine::Core::Get().GetRenderManager().GetOffscreenRenderer()->EndFrame();
    }

    m_UI->GetFinalImage(Engine::Core::Get().GetRenderManager().GetOffscreenRenderer()->GetPassObject<Engine::GeometryPass>()->GetDescriptorSet());

    // Render onscreen - UI
    if (auto commandBuffer = Engine::Core::Get().GetRenderManager().GetOnScreenRenderer()->vk_BeginFrame()) {
        Engine::Core::Get().GetRenderManager().GetOnScreenRenderer()->vk_BeginSwapChainRenderPass(commandBuffer);

            // Render UI
            m_UI->OnUpdate(commandBuffer, Engine::Core::Get().GetClock().GetDeltaTime());

        // End render pass
        Engine::Core::Get().GetRenderManager().GetOnScreenRenderer()->vk_EndSwapChainRenderPass(commandBuffer);
        Engine::Core::Get().GetRenderManager().GetOnScreenRenderer()->vk_EndFrame();
    }

    vkDeviceWaitIdle(Engine::Core::Get().GetDevice().vk_GetDevice());

    // for(auto& entity : m_Scene->GetEntitesToDelete()) {
    //     m_Scene->DestroyEntity(entity);
    // }

    // m_Scene->ClearQueue();
}

void Editor::EditorLayer::OnEnd() {
    vkDeviceWaitIdle(Engine::Core::Get().GetDevice().vk_GetDevice());
    if (m_HDRTexture) {
        m_HDRTexture->Destroy();
    }
    // m_Window->CleanUp();
}

void Editor::EditorLayer::InitializeRenderingSystems(){
    // std::array<VkDescriptorSetLayout, 2> setLayouts =
    //     {m_SceneContext->GetSceneDescriptorSetLayout().GetDescriptorSetLayout(),
    //     m_SceneContext->GetMaterialDescriptorSetLayout().GetDescriptorSetLayout()};

    // // Initialize scene rendering system

    // // SCENE SYSTEMS
    // m_SceneRenderingSystem = std::make_shared<Engine::SceneRenderingSystem>(*m_Device,
    //     Engine::Core::Get().GetRenderManager().GetOffscreenRenderer()->GetPassObject<Engine::GeometryPass>()->GetRenderPass(),
    //     setLayouts);

    // m_GridRenderingSystem = std::make_shared<Engine::GridRenderingSystem>(*m_Device,
    //      Engine::Core::Get().GetRenderManager().GetOffscreenRenderer()->GetPassObject<Engine::GeometryPass>()->GetRenderPass(),
    //      setLayouts);

    // m_ShadowRenderingSystem = std::make_shared<Engine::DepthRenderingSystem>(*m_Device,
    //     Engine::Core::Get().GetRenderManager().GetOffscreenRenderer()->GetPassObject<Engine::ShadowPass>()->GetRenderPass(),
    //     setLayouts);


    // // HDRi SYSTEMS
    // m_EnvToCubemapRenderingSystem = std::make_shared<Engine::EnvToCubemapRenderingSystem>(*m_Device,
    //     Engine::Core::Get().GetRenderManager().GetOffscreenRenderer()->GetPassObject<Engine::HDRiToCubemapPass>()->GetRenderPass(),
    //     Engine::Core::Get().GetRenderManager().GetOffscreenRenderer()->GetSharedDescriptorSetLayout().GetDescriptorSetLayout());

    // m_CubemapRenderingSystem = std::make_shared<Engine::CubemapRenderingSystem>(*m_Device,
    //     Engine::Core::Get().GetRenderManager().GetOffscreenRenderer()->GetPassObject<Engine::GeometryPass>()->GetRenderPass(),
    //     Engine::Core::Get().GetRenderManager().GetOffscreenRenderer()->GetSharedDescriptorSetLayout().GetDescriptorSetLayout());

    // m_IrradianceRenderingSystem = std::make_shared<Engine::IrradianceRenderingSystem>(*m_Device,
    //     Engine::Core::Get().GetRenderManager().GetOffscreenRenderer()->GetPassObject<Engine::IrradianceSamplingPass>()->GetRenderPass(),
    //     Engine::Core::Get().GetRenderManager().GetOffscreenRenderer()->GetSharedDescriptorSetLayout().GetDescriptorSetLayout());

    // m_PrefilterRenderingSystem = std::make_shared<Engine::PrefilterRenderingSystem>(*m_Device,
    //     Engine::Core::Get().GetRenderManager().GetOffscreenRenderer()->GetPassObject<Engine::HDRPrefilterPass>()->GetRenderPass(),
    //     Engine::Core::Get().GetRenderManager().GetOffscreenRenderer()->GetSharedDescriptorSetLayout().GetDescriptorSetLayout());

    // m_PreethamSkyRenderingSystem = std::make_shared<Engine::PreethamSkyRenderingSystem>(*m_Device,
    //     Engine::Core::Get().GetRenderManager().GetOffscreenRenderer()->GetPassObject<Engine::PreethamSkyPass>()->GetRenderPass(),
    //     Engine::Core::Get().GetRenderManager().GetOffscreenRenderer()->GetSharedDescriptorSetLayout().GetDescriptorSetLayout());

    // m_BRDFLutRenderingSystem = std::make_shared<Engine::BRDFLutRenderingSystem>(*m_Device,
    //     Engine::Core::Get().GetRenderManager().GetOffscreenRenderer()->GetPassObject<Engine::LUTPass>()->GetRenderPass(),
    //     Engine::Core::Get().GetRenderManager().GetOffscreenRenderer()->GetSharedDescriptorSetLayout().GetDescriptorSetLayout());

    // // FINAL/COMPOSITE SYSTEM
    // m_CompositeRenderingSystem = std::make_shared<Engine::CompositeRenderingSystem>(*m_Device,
    //     Engine::Core::Get().GetRenderManager().GetOffscreenRenderer()->GetPassObject<Engine::CompositePass>()->GetRenderPass(),
    //     Engine::Core::Get().GetRenderManager().GetOffscreenRenderer()->GetPassObject<Engine::CompositePass>()->GetDescriptorSetLayout()->GetDescriptorSetLayout());

    m_SceneRendering = std::make_shared<Engine::SceneRenderingSystem_new>(Engine::Core::Get().GetDevice(), 
        Engine::Core::Get().GetRenderManager().GetOffscreenRenderer()->GetPassObject<Engine::GeometryPass>()->GetRenderPass());
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

        Engine::Core::Get().GetRenderManager().GetOffscreenRenderer()->BeginRenderPass<Engine::PreethamSkyPass>(commandBuffer,
            Engine::Core::Get().GetRenderManager().GetOffscreenRenderer()->GetPassObject<Engine::PreethamSkyPass>(), i);
        {
            // preetham sky pass
            m_PreethamSkyRenderingSystem->Render(commandBuffer, Engine::Core::Get().GetRenderManager().GetOffscreenRenderer()->GetPassObject<Engine::PreethamSkyPass>()->GetDescriptorSet(), view);
        }
        Engine::Core::Get().GetRenderManager().GetOffscreenRenderer()->EndRenderPass(commandBuffer);
    }

    auto cubeMapInfo = Engine::Core::Get().GetRenderManager().GetOffscreenRenderer()->GetPassObject<Engine::PreethamSkyPass>()->GetImageInfo(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

    Engine::DescriptorWriter irradianceWriter(Engine::Core::Get().GetRenderManager().GetOffscreenRenderer()->GetSharedDescriptorSetLayout(),
        *Engine::DescriptorAllocator::GetSharedPool());
    irradianceWriter.writeImage(0, &cubeMapInfo);
    irradianceWriter.overwrite(Engine::Core::Get().GetRenderManager().GetOffscreenRenderer()->GetPassObject<Engine::IrradianceSamplingPass>()->GetDescriptorSet());

    Engine::DescriptorWriter prefilterWriter(Engine::Core::Get().GetRenderManager().GetOffscreenRenderer()->GetSharedDescriptorSetLayout(),
        *Engine::DescriptorAllocator::GetSharedPool());
    prefilterWriter.writeImage(0, &cubeMapInfo);
    prefilterWriter.overwrite(Engine::Core::Get().GetRenderManager().GetOffscreenRenderer()->GetPassObject<Engine::HDRPrefilterPass>()->GetDescriptorSet());

    Engine::DescriptorWriter cubeMapWriter(Engine::Core::Get().GetRenderManager().GetOffscreenRenderer()->GetSharedDescriptorSetLayout(),
        *Engine::DescriptorAllocator::GetSharedPool());
    cubeMapWriter.writeImage(0, &cubeMapInfo);
    cubeMapWriter.overwrite(Engine::Core::Get().GetRenderManager().GetOffscreenRenderer()->GetPassObject<Engine::HDRiToCubemapPass>()->GetDescriptorSet());
    
    // Now compute irradiance for each face
    for (int i = 0; i < 6; i++) {
        auto view = glm::lookAt(glm::vec3(0.0f), directions[i], ups[i]);;

        Engine::Core::Get().GetRenderManager().GetOffscreenRenderer()->BeginRenderPass<Engine::IrradianceSamplingPass>(commandBuffer,
            Engine::Core::Get().GetRenderManager().GetOffscreenRenderer()->GetPassObject<Engine::IrradianceSamplingPass>(), i);
        {
            m_IrradianceRenderingSystem->Render(commandBuffer, Engine::Core::Get().GetRenderManager().GetOffscreenRenderer()->GetPassObject<Engine::IrradianceSamplingPass>()->GetDescriptorSet(), view);
        }
        Engine::Core::Get().GetRenderManager().GetOffscreenRenderer()->EndRenderPass(commandBuffer);
    }

    // Get mip levels from the pass object
    auto mipLevels = Engine::Core::Get().GetRenderManager().GetOffscreenRenderer()
        ->GetPassObject<Engine::HDRPrefilterPass>()->GetMipLevels();

    for (uint32_t mip = 0; mip < mipLevels; ++mip) {
        int mipExtent  = static_cast<unsigned int>(128 * std::pow(0.5, mip));

        for (uint32_t face = 0; face < 6; ++face) {
            uint32_t framebufferIndex = mip * 6 + face;
            auto view = glm::lookAt(glm::vec3(0.0f), directions[face], ups[face]);;

            Engine::Core::Get().GetRenderManager().GetOffscreenRenderer()->BeginRenderPass<Engine::HDRPrefilterPass>(
                commandBuffer,
                Engine::Core::Get().GetRenderManager().GetOffscreenRenderer()->GetPassObject<Engine::HDRPrefilterPass>(),
                framebufferIndex, mipExtent
            );

            {
                m_PrefilterRenderingSystem->Render(commandBuffer, Engine::Core::Get().GetRenderManager().GetOffscreenRenderer()->GetPassObject<Engine::HDRPrefilterPass>()->GetDescriptorSet(), view, mip);
            }

            Engine::Core::Get().GetRenderManager().GetOffscreenRenderer()->EndRenderPass(commandBuffer);
        }
    }

    // 2D BRDF LUT
    Engine::Core::Get().GetRenderManager().GetOffscreenRenderer()->BeginRenderPass(commandBuffer,
        Engine::Core::Get().GetRenderManager().GetOffscreenRenderer()->GetPassObject<Engine::LUTPass>());

        m_BRDFLutRenderingSystem->Render(commandBuffer, Engine::Core::Get().GetRenderManager().GetOffscreenRenderer()->GetPassObject<Engine::LUTPass>()->GetDescriptorSet());

    Engine::Core::Get().GetRenderManager().GetOffscreenRenderer()->EndRenderPass(commandBuffer);
}

