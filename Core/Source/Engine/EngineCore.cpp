#include "Engine/EngineCore.h"

namespace Engine {
    void EngineCore::OnStart() {
        // Init device and window
        m_Window = std::make_unique<Window>(800, 600, "Aquila Editor");
        m_Device = std::make_unique<Device>(*m_Window);

        // allocates global descriptor pool
        DescriptorAllocator::Init(*m_Device);

        // initialize rendermanager 
        m_RenderManager = std::make_unique<RenderManager>(*m_Device, *m_Window);
        

    }

    void EngineCore::OnUpdate() {
        
    }

    void EngineCore::OnEnd() {

    }

}