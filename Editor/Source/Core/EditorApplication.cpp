#include "Aquila/Assets/AssetManager.h"
#include "Core/EditorApplication.h"
#include "Aquila/Core/Defines.h"
#include "Aquila/Graphics/Pipeline/DynamicRenderingHelper.h"
#include "Core/EditorLayer.h"
#include "Aquila/Graphics/Core/Renderer.h"
#include "Aquila/Platform/PrimitiveTypes.h"
#include "UI/Managers/FontManager.h"
#include "UI/Managers/ThemeManager.h"
#include "UI/Panels/ContentBrowser.h"
#include "UI/Panels/Hierarchy.h"
#include "UI/Panels/MaterialEditor.h"
#include "UI/Panels/Properties.h"
#include "UI/Panels/Viewport.h"
#include "UI/Panels/Menubar.h"
#include "UI/Windows/ProfilerWindow.h"
#include "imgui.h"

namespace Editor {

Application::Application(const Aquila::Core::ApplicationConfig &config) : Aquila::Core::Application(config) {
	AQUILA_LOG_INFO("Editor Application constructed");
}

Application::~Application() {
	AQUILA_LOG_INFO("Editor Application destroyed");
}

void Application::OnInit() {
	AQUILA_LOG_INFO("    Initializing Aquila Editor");

	LoadEditorPreferences();

	InitializeImGui();
	InitializeManagers();

	InitializeEditor();

	AQUILA_LOG_INFO("Starting background asset preload...");
	GetAssetManager().PreloadDirectory("assets://", true, Aquila::Core::JobPriority::High);

	AQUILA_LOG_INFO("    Editor initialized successfully");
}

void Application::LoadEditorPreferences() {
	AQUILA_LOG_INFO("Loading editor preferences...");

	auto &prefs = Config::GetPreferences();
	prefs.LoadFromFile();

	AQUILA_LOG_INFO("Editor preferences loaded");
}

void Application::InitializeManagers() {
	AQUILA_LOG_INFO("Initializing editor managers...");

	auto &prefs = Config::GetPreferences();

	UI::FontManager::Get().Initialize(prefs.fonts);
	UI::ThemeManager::Get().Initialize(prefs.currentTheme);

	AQUILA_LOG_INFO("Editor managers initialized");
}

void Application::InitializeEditor() {
	AQUILA_LOG_INFO("Creating editor layers and windows...");

	m_MenubarLayer = CreateUnique<Menubar>(*this);

	m_EditorLayer = CreateUnique<EditorLayer>(*this);

	m_SceneHierarchyLayer = CreateUnique<SceneHierarchyPanel>(*this);
	m_PropertiesLayer = CreateUnique<PropertiesPanel>(*this);
	m_ContentBrowserLayer = CreateUnique<ContentBrowserPanel>(*this);
	m_ViewportLayer = CreateUnique<ViewportPanel>(*this);
	m_MaterialEditorLayer = CreateUnique<MaterialEditorPanel>(*this);

	m_AboutWindow = CreateUnique<UI::AboutWindow>();
	m_PreferencesWindow = CreateUnique<UI::PreferencesWindow>(*this);
	m_ProfilerWindow = CreateUnique<UI::ProfilerWindow>();

	m_MenubarLayer->SetMaterialEditorPanel(m_MaterialEditorLayer.get());
	m_MenubarLayer->SetAboutWindow(m_AboutWindow.get());
	m_MenubarLayer->SetPreferencesWindow(m_PreferencesWindow.get());

	m_PreferencesWindow->SetDeferredOperationCallback([this]() { m_FontReloadRequested = true; });

	PushLayer(std::move(m_MenubarLayer));
	PushLayer(std::move(m_EditorLayer));
	PushLayer(std::move(m_SceneHierarchyLayer));
	PushLayer(std::move(m_PropertiesLayer));
	PushLayer(std::move(m_ContentBrowserLayer));
	PushLayer(std::move(m_ViewportLayer));
	PushLayer(std::move(m_MaterialEditorLayer));
	PushLayer(std::move(m_AboutWindow));
	PushLayer(std::move(m_PreferencesWindow));
	PushLayer(std::move(m_ProfilerWindow));

	AQUILA_LOG_INFO("Editor layers and windows created");
}

void Application::InitializeImGui() {
	AQUILA_LOG_INFO("Initializing ImGui...");

	IMGUI_CHECKVERSION();
	ImGui::CreateContext();

	ImGuiIO &io = ImGui::GetIO();
	io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
	io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

	const auto &window = GetWindow();
	auto &device = GetDevice();
	auto &renderer = GetRenderer();

	m_ImGuiColorFormat = renderer.GetSwapchain()->GetImageFormat();

	ImGui_ImplGlfw_InitForVulkan(window.GetNativeWindow(), true);

	ImGui_ImplVulkan_InitInfo init_info = {};
	init_info.Instance = device.GetInstance();
	init_info.PhysicalDevice = device.GetPhysicalDevice();
	init_info.Device = device.GetDevice();
	init_info.Queue = device.GetGraphicsQueue();
	init_info.DescriptorPool =
		Aquila::Graphics::RenderingPipeline::DescriptorAllocator::GetSharedPool()->GetDescriptorPool();
	init_info.MinImageCount = 2;
	init_info.ImageCount = 2;
	init_info.Allocator = VK_NULL_HANDLE;
	init_info.UseDynamicRendering = true;

	init_info.PipelineInfoMain.RenderPass = VK_NULL_HANDLE;
	init_info.PipelineInfoMain.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
	init_info.PipelineInfoMain.Subpass = 0;
	init_info.PipelineInfoMain.PipelineRenderingCreateInfo = { .sType =
																   VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO };
	init_info.PipelineInfoMain.PipelineRenderingCreateInfo.colorAttachmentCount = 1;
	init_info.PipelineInfoMain.PipelineRenderingCreateInfo.pColorAttachmentFormats = &m_ImGuiColorFormat;

	ImGui_ImplVulkan_Init(&init_info);
	AQUILA_LOG_INFO("ImGui initialized");
}

void Application::ProcessDeferredOperations() {
	if (m_FontReloadRequested) {
		UI::FontManager::Get().Shutdown();
		UI::FontManager::Get().Initialize(Config::GetPreferences().fonts);

		m_FontReloadRequested = false;
	}
}

void Application::BeginUIFrame() {
	ProcessDeferredOperations();

	ImGui_ImplVulkan_NewFrame();
	ImGui_ImplGlfw_NewFrame();
	ImGui::NewFrame();
	ImGuizmo::BeginFrame();
}

void Application::EndUIFrame(VkCommandBuffer commandBuffer) {
	ImGui::Render();

	const auto &io = ImGui::GetIO();

	if ((io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) != 0) {
		ImGui::UpdatePlatformWindows();
		ImGui::RenderPlatformWindowsDefault();
	}

	auto &renderer = GetRenderer();
	auto *currentImageView = renderer.GetSwapchain()->GetCurrentImageView(renderer.GetCurrentImageIndex());
	auto *currentImage = renderer.GetSwapchain()->GetCurrentImage(renderer.GetCurrentImageIndex());
	auto extent = renderer.GetSwapchain()->GetExtent();

	Aquila::Graphics::Helpers::DynamicRendering::BeginSwapchain(commandBuffer, currentImageView, extent, false);
	ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), commandBuffer);
	Aquila::Graphics::Helpers::DynamicRendering::End(commandBuffer);

	auto &device = GetDevice();
	Aquila::Graphics::Helpers::DynamicRendering::TransitionImages(
		device, commandBuffer, { currentImage }, renderer.GetSwapchain()->GetImageFormat(), VK_NULL_HANDLE,
		VK_FORMAT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);

	ImGui::EndFrame();
}

void Application::OnUpdate(f32 deltaTime) {}

void Application::OnRender(VkCommandBuffer commandBuffer) {}

void Application::OnShutdown() {
	AQUILA_LOG_INFO("Shutting down editor...");

	GetDevice().Wait();

	ShutdownImGui();
	ShutdownManagers();

	Config::GetPreferences().SaveToFile();

	AQUILA_LOG_INFO("Editor shutdown complete");
}

void Application::ShutdownManagers() {
	AQUILA_LOG_INFO("Shutting down editor managers...");

	UI::ThemeManager::Get().Shutdown();
	UI::FontManager::Get().Shutdown();

	AQUILA_LOG_INFO("Editor managers shut down");
}

void Application::ShutdownImGui() {
	AQUILA_LOG_INFO("Shutting down ImGui...");

	ImGui_ImplVulkan_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();

	AQUILA_LOG_INFO("ImGui shutdown complete");
}

} // namespace Editor
