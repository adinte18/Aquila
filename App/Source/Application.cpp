#include "Engine/Controller.h"
#include "EditorLayer.h"

int main(){
    auto& Engine = Engine::Controller::Get();
    auto& Editor = Editor::EditorLayer::Get();
    
    while (!Engine.GetWindow().ShouldClose()) {
        Engine.OnUpdate();

        if (auto commandBuffer = Engine.GetRenderer().BeginFrame()) {
            Engine.GetRenderer().RenderScene(commandBuffer, Engine.GetCamera());
            Editor.OnUpdate(commandBuffer);
        }

        Engine.GetRenderer().EndFrame();

        vkDeviceWaitIdle(Engine::Controller::Get().GetDevice().GetDevice());
    }

    return 0;
}