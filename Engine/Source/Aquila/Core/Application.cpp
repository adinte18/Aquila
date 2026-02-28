#include "Aquila/Core/Application.h"

#include <ranges>
#include "Aquila/Core/Defines.h"
#include "Aquila/Events/SceneEvent.h"
#include "Aquila/Graphics/Material/MaterialSystem.h"
#include "Aquila/Platform/PrimitiveTypes.h"
#include "Aquila/Utilities/Profiler.h"
#include "ImGuizmo.h"
#include "Aquila/Assets/AssetManager.h"
#include "Aquila/Core/Window.h"
#include "Aquila/Graphics/Core/Renderer.h"
#include "Aquila/Graphics/Pipeline/DescriptorAllocator.h"
#include "Aquila/Platform/Filesystem/VirtualFileSystem.h"
#include "Aquila/Rendering/Camera.h"
#include "Aquila/Scene/EntityManager.h"
#include "Aquila/Scene/Scene.h"
#include "imgui.h"

namespace Aquila::Core {

Application::Application(const ApplicationConfig &config) : m_Config(config), m_Running(false) {
	AQUILA_LOG_INFO("Creating Aquila Application");
	if (!Initialize()) {
		throw std::runtime_error("Failed to initialize application");
	}
}

Application::~Application() {
	AQUILA_LOG_INFO("Destroying Aquila Application");
	Shutdown();
	AQUILA_LOG_INFO("Application destroyed");
}

void Application::Run() {
	m_Running = true;
	AQUILA_LOG_INFO("Starting application main loop");

	try {
		OnInit();
	} catch (const std::exception &e) {
		std::string issue = e.what();
		AQUILA_LOG_ERROR("Exception during initialization: {}", issue);
		m_Running = false;
		OnShutdown();
		return;
	}

	while (m_Running && !m_Window->ShouldClose()) {
		try {
			Update();
		} catch (const std::exception &e) {
			std::string issue = e.what();
			AQUILA_LOG_ERROR("Exception in main loop: {}", issue);
			m_Running = false;
			break;
		}
	}

	OnShutdown();
	AQUILA_LOG_INFO("Application main loop ended");
}

void Application::Render() const {
	m_Renderer->BeginSceneRendering();

	{
		m_Renderer->DispatchCompute();

		for (const auto &layer : m_LayerStack) {
			layer->OnRender(); //scene rendering happens in viewport class of the editor
		}
	}

	m_Renderer->EndSceneRendering();
}

void Application::Close() {
	m_Running = false;
}

void Application::OnEvent(Event &event) {
	EventDispatcher dispatcher(event);
	dispatcher.Dispatch<WindowCloseEvent>([this](WindowCloseEvent &event) { return OnWindowClose(event); });
	dispatcher.Dispatch<WindowResizeEvent>([this](WindowResizeEvent &event) { return OnWindowResize(event); });
	dispatcher.Dispatch<ViewportResizeEvent>([this](ViewportResizeEvent &event) { return OnViewportResize(event); });
	dispatcher.Dispatch<SceneLoadedEvent>([this](SceneLoadedEvent &event) { return OnSceneLoaded(event); });
	dispatcher.Dispatch<EntityCreatedEvent>([this](EntityCreatedEvent &event) { return OnEntityCreated(event); });
	dispatcher.Dispatch<EntityDeletedEvent>([this](EntityDeletedEvent &event) { return OnEntityDeleted(event); });
	dispatcher.Dispatch<EntityVisibilityToggle>(
		[this](EntityVisibilityToggle &event) { return OnEntityToggled(event); });
	m_Input.OnEvent(event);

	for (auto &iterator : std::ranges::reverse_view(m_LayerStack)) {
		if (event.handled) {
			break;
		}
		iterator->OnEvent(event);
	}
}

void Application::PushLayer(Unique<Layer> layer) {
	m_LayerStack.PushLayer(std::move(layer));
}

void Application::PushOverlay(Unique<Layer> overlay) {
	m_LayerStack.PushOverlay(std::move(overlay));
}

bool Application::Initialize() {
	// Initialize platform
	if (!Platform::Initialize()) {
		AQUILA_LOG_ERROR("Failed to initialize platform");
		return false;
	}

	Platform::Filesystem::VirtualFileSystem::Init();
	m_VFS = Platform::Filesystem::VirtualFileSystem::Get();

	if (m_VFS == nullptr) {
		AQUILA_LOG_ERROR("Failed to initialize VFS");
		return false;
	}

	const auto assetsFolder = CreateRef<Platform::Filesystem::NativeFileSystem>(m_Config.assetPath);
	m_VFS->Mount("assets://", assetsFolder, 100, false);

	const auto shadersFolder = CreateRef<Platform::Filesystem::NativeFileSystem>(ASSET_PATH "/Shaders");
	m_VFS->Mount("assets://shaders/", shadersFolder, 100, false);

	const auto scenesFolder = CreateRef<Platform::Filesystem::NativeFileSystem>(ASSET_PATH "/Scenes");
	m_VFS->Mount("assets://scenes/", scenesFolder, 100, false);

	const auto materialsFolder = CreateRef<Platform::Filesystem::NativeFileSystem>(ASSET_PATH "/Materials");
	m_VFS->Mount("assets://materials/", materialsFolder, 100, false);

	AQUILA_LOG_INFO("VFS Initialized with {} mount points.", m_VFS->GetMountPoints().size());

	if (!m_VFS->IsDirectory("assets://")) {
		AQUILA_LOG_ERROR("VFS asset directory not accessible!");
		return false;
	}

	m_Window = CreateUnique<Window>(m_Config.windowWidth, m_Config.windowHeight, m_Config.windowTitle);
	m_Window->SetEventCallback([this](Event &event) { OnEvent(event); });

	m_Device = CreateUnique<Graphics::Device>(*m_Window);

	Graphics::RenderingPipeline::DescriptorAllocator::Init(*m_Device);

	m_EditorCamera = CreateUnique<Rendering::Camera>();
	m_EditorCamera->SetPerspectiveProjection(60.F, 16.F / 9.F, 0.1f, 1000.F);
	m_EditorCamera->SetViewYXZ(vec3(0, 0, -10), vec3(0, 0, 0));

	m_MaterialSystem = CreateRef<Graphics::Material::MaterialSystem>(*m_Device);

	m_Renderer = CreateUnique<Graphics::Renderer>(*m_Device, *m_Window, *m_MaterialSystem, *m_EditorCamera);

	m_AssetManager = CreateUnique<Assets::AssetManager>(*m_Device, *m_MaterialSystem, 0);
	m_AssetManager->Initialize();

	m_AssetManager->SetOnAssetLoaded([](const Utils::UUID &uuid, Assets::AssetType type) {});
	m_AssetManager->SetOnAssetUnloaded([](const Utils::UUID &uuid, Assets::AssetType type) {});
	m_AssetManager->SetOnSceneDestroyed([](SceneManagement::Scene *scene) {});
	m_AssetManager->SetOnSceneDeactivated([](SceneManagement::Scene *scene) {});

	auto *defaultScene = m_AssetManager->CreateScene("Default Scene");
	m_AssetManager->ActivateScene(defaultScene);

	m_Stopwatch = CreateUnique<Utils::Stopwatch>();

	AQUILA_LOG_INFO("Application initialized successfully");
	return true;
}
void Application::Update() {
	m_Stopwatch->Tick();
	m_DeltaTime = m_Stopwatch->GetDeltaTime();

	m_AssetManager->ProcessMainThreadTasks();

	m_Window->PollEvents();
	m_Input.Update();

	for (auto &layer : m_LayerStack) {
		layer->OnUpdate(m_DeltaTime);
	}

#ifdef AQUILA_DEBUG
	m_AssetManager->CheckForModifiedAssets();
#endif

	if (auto *activeScene = m_AssetManager->GetActiveScene()) {
		activeScene->GetEntityManager()->FlushDeletionQueue();
	}

	OnUpdate(m_DeltaTime);

	m_Renderer->InvalidatePasses();

	PROFILE_FRAME_BEGIN();

	auto *commandBuffer = m_Renderer->BeginFrame();

	if (commandBuffer != VK_NULL_HANDLE) {
		Render();

		{
			BeginUIFrame();
			for (const auto &layer : m_LayerStack) {
				layer->OnImGuiRender();
			}
			EndUIFrame(commandBuffer);
		}
	}

	m_Renderer->EndFrame();

	if (m_Renderer->IsPreviousFrameComplete()) {
		m_Device->GetDeletionManager().ProcessDeletions();
	}

	PROFILE_FRAME_END();
}

void Application::Shutdown() {
	if (m_Running) {
		m_Running = false;
	}

	AQUILA_LOG_INFO("Shutting down application");

	if (m_Device) {
		m_Device->Wait();
	}

	if (auto *activeScene = m_AssetManager->GetActiveScene()) {
		activeScene->GetEntityManager()->Clear();
	}

	m_LayerStack.Clear();
	m_EditorCamera.reset();

	AQUILA_LOG_DEBUG("Shutting down renderer");
	m_Renderer.reset();

	AQUILA_LOG_DEBUG("Shutting down MaterialSystem");
	if (m_MaterialSystem) {
		m_MaterialSystem->Shutdown();
		m_MaterialSystem.reset(); // Destroy it
	}

	AQUILA_LOG_DEBUG("Shutting down asset manager");
	m_AssetManager.reset();

	if (m_Device->GetDeletionManager().HasPendingDeletions()) {
		m_Device->GetDeletionManager().ProcessDeletions();
	}

	Graphics::RenderingPipeline::DescriptorAllocator::Cleanup();

	m_Device.reset();
	m_Window.reset();

	Platform::Filesystem::VirtualFileSystem::Shutdown();
	Platform::Shutdown();

	AQUILA_LOG_INFO("Application shutdown complete");
}

bool Application::OnWindowClose(WindowCloseEvent &event) {
	m_Running = event.IsClosed();
	return false;
}

bool Application::OnWindowResize(WindowResizeEvent &event) {
	// tell the renderer window has been resized

	if (m_Renderer) {
		m_Renderer->OnEvent(event);
	}

	return false;
}

bool Application::OnViewportResize(ViewportResizeEvent &event) {
	// tell the renderer viewport has been resized
	if (m_Renderer) {
		m_Renderer->OnEvent(event);
	}

	return false;
}

bool Application::OnEntityCreated(EntityCreatedEvent &event) {
	if (m_Renderer) {
		m_Renderer->OnEvent(event);
	}
	return false;
}

bool Application::OnEntityToggled(EntityVisibilityToggle &event) {
	if (m_Renderer) {
		m_Renderer->OnEvent(event);
	}
	return false;
}

bool Application::OnEntityDeleted(EntityDeletedEvent &event) {
	if (m_Renderer) {
		m_Renderer->OnEvent(event);
	}
	return false;
}

bool Application::OnSceneLoaded(SceneLoadedEvent &event) {
	if (auto *const scene = m_AssetManager->LoadScene(event.GetScenePath())) {
		m_AssetManager->ActivateScene(scene);
		m_Window->SetTitle("Current scene : " + scene->GetSceneName());
	}

	return false;
}

} // namespace Aquila::Core
