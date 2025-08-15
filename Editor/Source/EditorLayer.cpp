#include "EditorLayer.h"
#include "Engine/Renderer/Renderer.h"
#include "UI/UI.h"

namespace Editor {
    EditorLayer::EditorLayer() {
        OnAttach();
    }

    EditorLayer::~EditorLayer() {
        OnDetach();
    }

    void EditorLayer::OnAttach() {
        UIManager::Get().OnStart();

        Debug::Log("Editor started");
    }


    void EditorLayer::OnUpdate(VkCommandBuffer commandBuffer) {
        // render UI
        auto& renderer = Engine::Controller::Get().GetRenderer();

        Engine::RenderPassConfig config{};
        config.type = Engine::RenderType::PRESENT;

        renderer.BeginRenderPass(commandBuffer, config);
        {
            UIManager::Get().OnUpdate(commandBuffer);
        }
        renderer.EndRenderPass(commandBuffer);
    }

    void EditorLayer::OnDetach() {
        UIManager::Get().OnEnd();

        Debug::Log("Editor stopped");
    }

}