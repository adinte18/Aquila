#include "Engine/Controller.h"
#include "EditorLayer.h"

int main(){
    // init systems
    Engine::Controller::Init();
    Editor::EditorLayer::Init();

    
    auto Engine = Engine::Controller::Get();
    auto Editor = Editor::EditorLayer::Get();
    
    while (!Engine->GetWindow().ShouldClose()) {
        Engine->OnUpdate();

        if (auto commandBuffer = Engine->GetRenderer().BeginFrame()) {
            Engine->GetRenderer().RenderScene();

            Editor->RenderUI(commandBuffer);
        }

        Engine->GetRenderer().EndFrame();
    }

    Engine->GetDevice().Wait();

    // shutdown systems
    Editor::EditorLayer::Shutdown();
    Engine::Controller::Shutdown();

    return 0;
}