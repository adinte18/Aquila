#include "Engine/Controller.h"
#include "Platform/Filesystem/NativeFileSystem.h"
#include "Platform/Filesystem/VirtualFileSystem.h"
#include "Utilities/Log.h"
#include "Utilities/Singleton.h"

namespace Engine {

Controller::Controller() { OnStart(); }

Controller::~Controller() { OnEnd(); }

void Controller::OnStart() {
  if (Core::Platform::Initialize()) {
    const auto &info = Core::Platform::GetPlatformInfo();
    AQUILA_LOG_INFO("Running on: " + std::string(info.name) + " (" +
                    std::string(info.version) + ")");
    AQUILA_LOG_INFO("CPU Cores: " + std::to_string(info.cpuCores));
    AQUILA_LOG_INFO("Total Memory: " +
                    std::to_string(info.totalMemory / (1024 * 1024)) + " MB");
  } else {
    AQUILA_LOG_ERROR("Failed to initialize platform");
    return;
  }

  VFS::VirtualFileSystem::Init();
  m_VFS = VFS::VirtualFileSystem::Get();

  auto assetsFolder = CreateRef<VFS::NativeFileSystem>(ASSET_PATH);
  m_VFS->Mount("/Assets", assetsFolder, 100, false);
  AQUILA_LOG_INFO("VFS Initialized with " +
                  std::to_string(m_VFS->GetMountPoints().size()) +
                  " mount points.");

  m_Window = CreateUnique<Window>(800, 600, "Aquila Editor");
  m_Device = CreateUnique<Device>(*m_Window);
  m_Stopwatch = CreateUnique<Utility::Stopwatch>();

  DescriptorAllocator::Init(*m_Device); // setup global descriptor pool

  m_EditorCamera = CreateUnique<EditorCamera>();
  m_EditorCamera->SetPerspectiveProjection(60.f, 1.778f, 0.1f, 1000.f);
  m_EditorCamera->SetPosition(vec3(0, 1, -10));
  m_EditorCamera->SetViewYXZ(m_EditorCamera->GetPosition(),
                             m_EditorCamera->GetRotation());

  m_Renderer = CreateUnique<Renderer>(*m_Device, *m_Window);

  m_SceneManager = CreateUnique<SceneManager>();
  m_SceneManager->EnqueueScene(CreateUnique<AquilaScene>("Default Scene"));
  m_SceneManager->RequestSceneChange();
  m_SceneManager->ProcessSceneChange();

  if (m_SceneManager->GetActiveScene() != nullptr) {
    EventBus::Init();
    m_EventRegistry->RegisterHandlers(m_Device, m_SceneManager, m_Renderer);
  } else {
    AQUILA_LOG_ERROR("No active scene to register handlers for.");
  }

  AQUILA_LOG_INFO("Engine core initialized");
}

void Controller::OnUpdate() {
  m_Stopwatch->Tick();
  m_DeltaTime = m_Stopwatch->GetDeltaTime();

  m_Renderer->InvalidatePasses();

  if (m_SceneManager && m_SceneManager->HasPendingSceneChange()) {
    m_SceneManager->ProcessSceneChange();
  }

  m_Window->PollEvents();
}

void Controller::OnEnd() {
  AQUILA_LOG_INFO("Engine controller destructor called");

  VFS::VirtualFileSystem::Shutdown();
  EventBus::Shutdown();
  Core::Platform::Shutdown();
}

void Controller::HandleSceneChange() {
  if (m_SceneManager && m_SceneManager->HasPendingSceneChange()) {
    m_SceneManager->ProcessSceneChange();
  }
}
} // namespace Engine