#include "EditorLayer.h"

void Editor::EditorLayer::OnStart() {
    Engine::Core::Get().GetClock().Reset();

    m_EditorCamera = std::make_unique<Engine::EditorCamera>();
    m_EditorCamera->SetPerspectiveProjection(40.f, 1.778f, 0.1f, 100.f);
    m_EditorCamera->SetPosition(glm::vec3(0, 1, -10));
    m_EditorCamera->SetViewYXZ(m_EditorCamera->GetPosition(), m_EditorCamera->GetRotation());

    m_UI = std::make_unique<UIManager>();

    std::cout << "Everything went well, and is initialized" << std::endl;
}


void Editor::EditorLayer::OnUpdate() {
    auto& core = Engine::Core::Get();
    auto& renderer = core.GetRenderer();

    // Poll events and calculate delta time
    core.OnUpdate();

    // Render offscreen - Scene
    if (auto commandBuffer = renderer.BeginFrame()) {

        renderer.BeginOffscreenRenderPass<Engine::GeometryPass>(commandBuffer);
        {
            Engine::SceneRenderSystem::SceneRenderingContext context{};
            context.commandBuffer = commandBuffer;
            context.camera = m_EditorCamera.get();
            renderer.GetRenderingSystem<Engine::SceneRenderSystem>()->Render(context);
        }
        renderer.EndRenderPass(commandBuffer);

        m_UI->GetFinalImage(renderer.GetPassObject<Engine::GeometryPass>()->GetDescriptorSet());

        // Render onscreen - UI
        renderer.BeginSwapchainRenderPass(commandBuffer);
        {
            // Render UI
            m_UI->OnUpdate(commandBuffer, Engine::Core::Get().GetClock().GetDeltaTime());
        }
        // End render pass
        renderer.EndRenderPass(commandBuffer);
    }

    renderer.EndFrame();

    vkDeviceWaitIdle(Engine::Core::Get().GetDevice().vk_GetDevice());
}

void Editor::EditorLayer::OnEnd() {

}
