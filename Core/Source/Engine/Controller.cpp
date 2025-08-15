#include "Engine/Controller.h"
#include "Engine/EditorCamera.h"
#include "Engine/Renderer/DescriptorAllocator.h"
#include "Engine/Renderer/Renderer.h"
#include "vulkan/vulkan_core.h"

namespace Engine {
    
    Controller::Controller() {
        OnStart();    
    }

    Controller::~Controller() {
        OnEnd();
    }

    void Controller::OnStart(){
        if (Core::Platform::Initialize()) {
            const auto& info = Core::Platform::GetPlatformInfo();
            Debug::Log("Running on: " + std::string(info.name) + " (" + std::string(info.version) + ")");
            Debug::Log("CPU Cores: " + std::to_string(info.cpuCores));
            Debug::Log("Total Memory: " + std::to_string(info.totalMemory / (1024 * 1024)) + " MB");
        } else {
            Debug::LogError("Failed to initialize platform");
            return;
        }

        m_Window = std::make_unique<Window>(800, 600, "Aquila Editor");
        m_Device = std::make_unique<Device>(*m_Window);
        m_Stopwatch = std::make_unique<Timer::Stopwatch>();

        DescriptorAllocator::Init(*m_Device); // setup global descriptor pool 

        m_EditorCamera = std::make_unique<EditorCamera>();
        m_EditorCamera->SetPerspectiveProjection(40.f, 1.778f, 0.1f, 100.f);
        m_EditorCamera->SetPosition(glm::vec3(0, 1, -10));
        m_EditorCamera->SetViewYXZ(m_EditorCamera->GetPosition(), m_EditorCamera->GetRotation());


        m_Renderer = std::make_unique<Renderer>(*m_Device, *m_Window);

        m_SceneManager = std::make_unique<SceneManager>();
        m_SceneManager->EnqueueScene(std::make_unique<AquilaScene>("Default Scene"));
        m_SceneManager->RequestSceneChange();
        m_SceneManager->ProcessSceneChange();
        
        if (m_SceneManager->GetActiveScene() != nullptr) {
            m_EventRegistry->RegisterHandlers(m_Device.get(), m_SceneManager.get(), m_Renderer.get());
        } else {
            Debug::LogError("No active scene to register handlers for.");
        }

        Debug::Log("Initialized");
    }

    void Controller::OnUpdate(){
        m_Stopwatch->Tick();
        m_DeltaTime = m_Stopwatch->GetDeltaTime();

        m_Renderer->InvalidatePasses();

        // Handle scene change
        if (m_SceneManager && m_SceneManager->HasPendingSceneChange()) {
            m_SceneManager->ProcessSceneChange();

            EventBus::Get().Clear();
            m_EventRegistry->RegisterHandlers(m_Device.get(), m_SceneManager.get(), m_Renderer.get());
        }

        m_Window->PollEvents();
    }

    void Controller::OnEnd(){
        m_Window->CleanUp();
        Core::Platform::Shutdown();

    }

    void Controller::HandleSceneChange() {
        if (m_SceneManager && m_SceneManager->HasPendingSceneChange()) {
            m_SceneManager->ProcessSceneChange();

            EventBus::Get().Clear();
            m_EventRegistry->RegisterHandlers(m_Device.get(), m_SceneManager.get(), m_Renderer.get());
        }
    }
}