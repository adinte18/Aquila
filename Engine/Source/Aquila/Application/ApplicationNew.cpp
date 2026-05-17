#include "Aquila/Application/ApplicationNew.h"
#include "Aquila/Foundation/Profiler.h"
#include "Aquila/Platform/Filesystem/VirtualFileSystem.h"
#include "Aquila/Platform/Input.h"
#include "Aquila/Foundation/Macros.h"

#include "Aquila/GFX/GfxCommandList.h"
#include "Aquila/RHI/Vulkan/VulkanShaderCompiler.h"

#include "Aquila/Rendering/Systems/LightCullingSystem.h"
#include "Aquila/Scene/EntityManager.h"
#include "Aquila/Scene/Components/TransformComponent.h"
#include "Aquila/Scene/Components/CameraComponent.h"
#include "Aquila/Scene/Components/MeshComponent.h"
#include "Aquila/Scene/Components/MaterialComponent.h"
#include "Aquila/Scene/Components/LightComponent.h"
#include "Aquila/Graphics/Resources/Mesh.h"
#include "Aquila/Graphics/Material/MaterialFactory.h"
#include "Aquila/Foundation/SharedConstants.h"

#include "Aquila/Rendering/Systems/DepthPrepassSystem.h"
#include "Aquila/Rendering/Systems/GeometrySystem.h"
#include "Aquila/Rendering/Systems/ComputeTestSystem.h"
#include "Aquila/UI/Core/ViewSystem.h"
#include "Aquila/UI/Rendering/ViewRenderingSystem.h"
#include "UIWidgetTest.h"

namespace Aquila::Application {

using namespace SceneManagement;
using namespace SceneManagement::Components;

Application::Application(const ApplicationSpec &spec) : m_Spec(spec) {
	m_Timer = CreateUnique<Foundation::Stopwatch>();
	m_Window = CreateUnique<Window>(spec.Width, spec.Height, spec.Name);
	Foundation::Profiler::Profiler::Init();
	Platform::Filesystem::VirtualFileSystem::Init();
	m_Window->SetEventCallback([this](Events::Event &event) {
		Aquila::Platform::Input::OnEvent(event);
		OnEvent(event);
	});

	m_Window->SetRefreshCallback([this]() {
		m_Timer->Tick();
		OnUpdate(m_Timer->GetDeltaTime());
	});

	m_Timer->Start();
}

Application::~Application() {
	m_Ctx->WaitIdle();

	Platform::Filesystem::VirtualFileSystem::Shutdown();
	Foundation::Profiler::Profiler::Shutdown();
	UI::Core::ViewSystem::Shutdown();
	Graphics::MaterialFactory::Shutdown();

	m_Scene.reset();
	m_RenderPipeline.reset();
	for (auto &font : m_Fonts) {
		font.reset();
	}
	for (auto &cache : m_TextureCaches) {
		cache.reset();
	}
	m_Swapchain.reset();
	m_Ctx.reset();

	// This should go aswell, bleeds vulkan stuff in Application
	RHI::VulkanShaderCompiler::Shutdown();
}

void Application::Run() {
	OnStart();

	while (m_Running && !m_Window->ShouldClose()) {
		m_Timer->Tick();
		// PROFILE_FRAME_BEGIN();
		m_Window->PollEvents();
		OnUpdate(m_Timer->GetDeltaTime());
		// PROFILE_FRAME_END();
	}

	OnShutdown();
}

void Application::Close() {
	m_Running = false;
}
void Application::OnStart() {
	InitRendering(m_Window->GetWidth(), m_Window->GetHeight());

	m_Scene = CreateUnique<Scene>("Main");

	auto *entityManager = m_Scene->GetEntityManager();
	auto cam = entityManager->CreateEntity("Camera");
	auto &camComp = cam.AddComponent<CameraComponent>();
	camComp.fov = 60.f;
	camComp.nearPlane = 0.1f;
	camComp.farPlane = 500.f;
	camComp.aspectRatio = static_cast<f32>(m_Window->GetWidth()) / static_cast<f32>(m_Window->GetHeight());
	camComp.primary = true;
	cam.GetComponent<TransformComponent>().SetLocalPosition({ 0.F, 1.5F, -5.F });
	m_Scene->SetActiveCamera(cam);

	Graphics::MaterialFactory::Get()->EnableHotReload(true);

	auto litMat = Graphics::MaterialFactory::Get()->Create(*m_Ctx, SharedConstants::SHADERS_DIR + "Basic.slang",
														   {
															   .type = Graphics::MaterialType::Lit,
															   .colorFormats = { RHI::TextureFormat::RGBA16F },
															   .depthTest = true,
															   .depthWrite = false,
														   });

	auto addCube = [&](const char *name, vec3 pos, Ref<Graphics::Material> mat) {
		auto entity = entityManager->CreateEntity(name);
		auto mesh = CreateRef<Graphics::Resources::Mesh>(name);
		mesh->LoadFromData(Graphics::Resources::Mesh::GenerateCube(0.5F));
		entity.AddComponent<MeshComponent>().SetMesh(mesh);
		entity.GetComponent<TransformComponent>().SetLocalPosition(pos);

		auto &comp = entity.AddComponent<MaterialComponent>(mat);
		comp.surfaceProperties.albedo = vec4(0.8f, 0.6f, 0.4f, 1.f);
		comp.surfaceProperties.metallic = 0.0f;
		comp.surfaceProperties.roughness = 0.6f;
	};
	addCube("CubeA", { -1.5f, 0.f, 2.f }, litMat);
	addCube("CubeB", { 1.5f, 0.f, 2.f }, litMat);
	addCube("Floor", { 0.0f, 0.5f, 2.f }, litMat);

	{
		auto e = entityManager->CreateEntity("SunLight");
		auto &light = e.AddComponent<LightComponent>(LightComponent::Type::Directional, vec3(1.0f, 0.95f, 0.8f), 2.0f);
		light.SetDirection(glm::normalize(vec3(0.4f, -1.0f, 0.6f)));
	}

	{
		auto e = entityManager->CreateEntity("PointA");
		e.GetComponent<TransformComponent>().SetLocalPosition({ -1.5f, -0.5f, 1.5f });
		auto &light = e.AddComponent<LightComponent>(LightComponent::Type::Point, vec3(1.0f, 0.4f, 0.1f), 5.0f);
		light.SetRange(6.0f);
	}

	{
		auto e = entityManager->CreateEntity("PointB");
		e.GetComponent<TransformComponent>().SetLocalPosition({ 1.5f, -0.5f, 1.5f });
		auto &light = e.AddComponent<LightComponent>(LightComponent::Type::Point, vec3(0.2f, 0.5f, 1.0f), 5.0f);
		light.SetRange(6.0f);
	}

	SetupScene();
}

void Application::SetupScene() {
	auto &editorCanvas = UI::Core::ViewSystem::Get()->GetLayer(UI::Core::UILayer::Editor);
	m_ViewportImage = SetupWidgetTest(*m_Ctx, editorCanvas, m_Fonts, m_TextureCaches,
									  &m_RenderPipeline->GetOutput());
}

void Application::OnShutdown() {}

void Application::InitRendering(uint32 width, uint32 height) {
	// THIS SHOULD GO, WE SHOULD HAVE A GENERIC SHADER COMPILER

	RHI::VulkanShaderCompiler::Initialize();
	Graphics::MaterialFactory::Init();

	m_Ctx = GFX::GfxContext::Create(*GetWindow().GetNativeWindow());
	UI::Core::ViewSystem::Init(m_Window->GetWidth(), m_Window->GetHeight());

	m_Swapchain = m_Ctx->CreateSwapchain({
		.width = width,
		.height = height,
		.format = RHI::TextureFormat::BGRA8,
		.imageCount = 2,
		.vsync = false,
	});

	m_RenderPipeline = CreateUnique<Rendering::RenderPipeline>(*m_Ctx, width, height);

	m_Renderer = &m_RenderPipeline->Add<Rendering::Renderer>();
	m_Renderer2D = &m_RenderPipeline->Add<Rendering::Renderer2D>();

	// ! IMPORTANT : Systems go here
	m_Renderer->AddSystem<Rendering::DepthPrepassSystem>();
	m_Renderer->AddSystem<Rendering::ClusterComputeSystem>();
	m_Renderer->AddSystem<Rendering::LightCullingSystem>();
	m_Renderer->AddSystem<Rendering::GeometrySystem>();
	m_Renderer2D->AddSystem<UI::Rendering::ViewRenderingSystem>();
	// !
}

void Application::OnUpdate(f32 deltaTime) {
	PROFILE_SCOPE("OnUpdate");
	if (m_PendingResize || m_Swapchain->NeedsResize()) {
		m_PendingResize = false;
		const uint32 width = m_Window->GetWidth();
		const uint32 height = m_Window->GetHeight();
		if (width == 0 || height == 0) {
			return;
		}
		HandleResize();
	}

	uint32 imageIndex = 0;
	{
		PROFILE_SCOPE("AcquireNextImage");
		if (!m_Swapchain->AcquireNextImage(imageIndex)) {
			return;
		}
	}

	m_Renderer->SetSwapchainTarget(*m_Swapchain, imageIndex);
	m_Renderer2D->SetSwapchainTarget(*m_Swapchain, imageIndex);

	auto cmd = m_Ctx->CreateCommandList(RHI::CommandListType::Graphics, "MainFrame");
	cmd->Begin();

	{
		PROFILE_SCOPE("ViewSystem::Update");
		UI::Core::ViewSystem::Get()->Update(deltaTime);
	}

	{
		PROFILE_SCOPE("ViewSystem::Compute");
		UI::Core::ViewSystem::Get()->Compute();
	}

	{
		const bool uiDirty =
			UI::Core::ViewSystem::Get()->IsAnyLayerDirty(UI::Core::UILayer::ScreenOverlay, UI::Core::UILayer::Editor);
		m_Renderer2D->SetUIDirty(uiDirty);
		if (uiDirty) {
			UI::Core::ViewSystem::Get()->ClearLayerDirtyFlags(UI::Core::UILayer::ScreenOverlay,
															  UI::Core::UILayer::Editor);
		}
	}

	{
		Graphics::MaterialFactory::Get()->Tick(*m_Ctx);
	}

	{
		PROFILE_SCOPE("RenderPipeline::Render");
		m_RenderPipeline->Render(*cmd, *m_Scene, deltaTime);
	}

	{
		PROFILE_SCOPE("SubmitFrame");
		m_Ctx->SubmitFrame(*cmd, m_Swapchain.get(), imageIndex);
	}

	{
		PROFILE_SCOPE("ProcessPendingDeletions");
		m_Ctx->ProcessPendingDeletions();
	}
}

void Application::OnEvent(Events::Event &event) {
	UI::Core::ViewSystem::Get()->OnEvent(event);

	Events::EventDispatcher dispatcher(event);

	dispatcher.Dispatch<Events::WindowCloseEvent>([this](Events::WindowCloseEvent &) {
		m_Running = false;
		return true;
	});

	dispatcher.Dispatch<Events::WindowResizeEvent>([this](Events::WindowResizeEvent &event) {
		if (event.GetWidth() > 0 && event.GetHeight() > 0) {
			m_PendingResize = true;
		}
		return true;
	});
}

void Application::HandleResize() {
	const uint32 width = m_Window->GetWidth();
	const uint32 height = m_Window->GetHeight();
	if (width == 0 || height == 0) {
		return;
	}

	m_Ctx->WaitIdle();

	m_Swapchain->Resize(width, height);
	m_RenderPipeline->Resize(width, height);
	UI::Core::ViewSystem::Get()->Resize(width, height);

	// RebuildTargets() creates a new GfxTexture — re-bind the raw pointer.
	if (m_ViewportImage) {
		m_ViewportImage->SetTexture(&m_RenderPipeline->GetOutput());
	}
}

} // namespace Aquila::Application
