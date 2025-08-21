#include "Engine/Controller.h"
#include "Platform/DebugLog.h"
#include "Platform/Filesystem/NativeFileSystem.h"
#include "Platform/Filesystem/VirtualFileSystem.h"

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

        VFS::VirtualFileSystem::Init();
        m_VFS = VFS::VirtualFileSystem::Get();
        
        auto assetsFolder = CreateRef<VFS::NativeFileSystem>(ASSET_PATH);
        m_VFS->Mount("/Assets", assetsFolder, 100, false);
        Debug::Log("VFS Initialized with " + std::to_string(m_VFS->GetMountPoints().size()) + " mount points.");

        m_Window = CreateUnique<Window>(800, 600, "Aquila Editor");
        m_Device = CreateUnique<Device>(*m_Window);
        m_Stopwatch = CreateUnique<Timer::Stopwatch>();

        DescriptorAllocator::Init(*m_Device); // setup global descriptor pool 

        m_EditorCamera = CreateUnique<EditorCamera>();
        m_EditorCamera->SetPerspectiveProjection(40.f, 1.778f, 0.1f, 100.f);
        m_EditorCamera->SetPosition(glm::vec3(0, 1, -10));
        m_EditorCamera->SetViewYXZ(m_EditorCamera->GetPosition(), m_EditorCamera->GetRotation());


        m_Renderer = CreateUnique<Renderer>(*m_Device, *m_Window);

        m_SceneManager = CreateUnique<SceneManager>();
        m_SceneManager->EnqueueScene(CreateUnique<AquilaScene>("Default Scene"));
        m_SceneManager->RequestSceneChange();
        m_SceneManager->ProcessSceneChange();
        
        if (m_SceneManager->GetActiveScene() != nullptr) {
            EventBus::Init();
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

            EventBus::Get()->Clear();
            m_EventRegistry->RegisterHandlers(m_Device.get(), m_SceneManager.get(), m_Renderer.get());
        }

        m_Window->PollEvents();
    }

    void Controller::OnEnd(){
        Debug::Log("Engine controller destructor called");
        m_Window->CleanUp();


        VFS::VirtualFileSystem::Shutdown();
        EventBus::Shutdown();
        Core::Platform::Shutdown();

    }

    void Controller::HandleSceneChange() {
        if (m_SceneManager && m_SceneManager->HasPendingSceneChange()) {
            m_SceneManager->ProcessSceneChange();

            EventBus::Get()->Clear();
            m_EventRegistry->RegisterHandlers(m_Device.get(), m_SceneManager.get(), m_Renderer.get());
        }
    }
}