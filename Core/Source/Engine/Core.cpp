#include "Engine/Core.h"

namespace Engine {
    
    void Core::OnStart(){
        // Initialize window
        m_Window = std::make_unique<Engine::Window>(800, 600, "Aquila Editor");
        m_Device = std::make_unique<Engine::Device>(*m_Window);

        AQUILA_OUT("[CORE] Initialized");
    }

    void Core::OnUpdate(){
        m_Clock->Tick();
        m_DeltaTime = m_Clock->GetDeltaTime();

        m_Window->PollEvents();
    }

    void Core::OnEnd(){
        m_Window->CleanUp();
    }
}