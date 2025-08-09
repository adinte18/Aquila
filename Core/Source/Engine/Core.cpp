#include "Engine/Core.h"
#include "Engine/DescriptorAllocator.h"

namespace Engine {
    
    Core::Core() {
        OnStart();    
    }

    Core::~Core() {
        OnEnd();
    }

    void Core::OnStart(){
        m_Window = std::make_unique<Engine::Window>(800, 600, "Aquila Editor");
        m_Device = std::make_unique<Engine::Device>(*m_Window);
        m_Clock = std::make_unique<Clock>();

        DescriptorAllocator::Init(*m_Device); // setup global descriptor pool 

//        m_RenderManager = std::make_unique<Engine::RenderManager>(*m_Device, *m_Window);

        m_Renderer = std::make_unique<Engine::Renderer>(*m_Device, *m_Window);

        m_SceneManager = std::make_unique<Engine::SceneManager>();
        m_SceneManager->EnqueueScene(std::make_unique<Engine::AquilaScene>("Default Scene"));
        m_SceneManager->RequestSceneChange();
        m_SceneManager->ProcessSceneChange();
        
        if (m_SceneManager->GetActiveScene() != nullptr) {
            m_EventRegistry->RegisterHandlers(m_Device.get(), m_SceneManager.get(), m_Renderer.get());
        } else {
            AQUILA_OUT("[CORE] No active scene to register handlers for.");
        }

        AQUILA_OUT("[CORE] Initialized");
    }

    void Core::OnUpdate(){
        m_Clock->Tick();
        m_DeltaTime = m_Clock->GetDeltaTime();

        m_Renderer->InvalidatePasses();

        // Handle scene change
        HandleSceneChange();

        m_Window->PollEvents();
    }

    void Core::OnEnd(){
        m_Window->CleanUp();
    }

    void Core::HandleSceneChange() {
        if (m_SceneManager && m_SceneManager->HasPendingSceneChange()) {
            m_SceneManager->ProcessSceneChange();

            Engine::EventBus::Get().Clear();
            m_EventRegistry->RegisterHandlers(m_Device.get(), m_SceneManager.get(), m_Renderer.get());
        }
    }

    // void Core::Run() {
    //     while (!m_Window->ShouldClose()) {
    //         OnUpdate();
    //         InvalidatePasses();
    //         HandleSceneChange();

    //         if (m_SceneManager->HasScene()) {
    //             m_SceneManager->GetActiveScene()->OnUpdate(m_Clock->GetDeltaTime());
    //         }

    //         m_RenderManager->Render();
    //     }
    // }

}