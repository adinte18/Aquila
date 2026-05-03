#include "Aquila/Application/ApplicationNew.h"
#include "Aquila/Platform/Input.h"
#include "Aquila/Foundation/Macros.h"

#include "Aquila/GFX/GfxCommandList.h"
#include "Aquila/RHI/Vulkan/VulkanShaderCompiler.h"

#include "Aquila/Scene/EntityManager.h"
#include "Aquila/Scene/Components/TransformComponent.h"
#include "Aquila/Scene/Components/CameraComponent.h"
#include "Aquila/Scene/Components/MeshComponent.h"
#include "Aquila/Graphics/Resources/Mesh.h"

#include "Aquila/Rendering/Systems/DepthPrepassSystem.h"
#include "Aquila/Rendering/Systems/GeometrySystem.h"
#include "Aquila/UI/Core/ViewSystem.h"
#include "Aquila/UI/Core/Label.h"
#include "Aquila/UI/Rendering/ViewRenderingSystem.h"
#include "Aquila/UI/Style/StyleLength.h"
#include "Aquila/UI/Style/StyleTypes.h"

namespace Aquila::Application {

using namespace SceneManagement;
using namespace SceneManagement::Components;

Application::Application(const ApplicationSpec &spec) : m_Spec(spec) {
	m_Timer = CreateUnique<Foundation::Stopwatch>();
	m_Window = CreateUnique<Window>(spec.Width, spec.Height, spec.Name);

	m_Window->SetEventCallback([this](Events::Event &event) {
		Aquila::Platform::Input::OnEvent(event);
		OnEvent(event);
	});

	m_Window->SetRefreshCallback([this]() {
		m_Timer->Tick();
		OnUpdate(m_Timer->GetDeltaTime());
	});

	m_Timer->Start();

	// THIS SHOULD GO, WE SHOULD HAVE A GENERIC SHADER COMPILER
}

Application::~Application() {
	m_Ctx->WaitIdle();

	UI::Core::ViewSystem::Shutdown();

	m_RenderPipeline.reset();
	m_Font.reset();
	m_Swapchain.reset();
	m_Ctx.reset();

	// This should go aswell, bleeds vulkan stuff in Application
	RHI::VulkanShaderCompiler::Shutdown();
}

void Application::Run() {
	OnStart();
	while (m_Running && !m_Window->ShouldClose()) {
		m_Timer->Tick();
		m_Window->PollEvents();
		OnUpdate(m_Timer->GetDeltaTime());
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

	auto addCube = [&](const char *name, vec3 pos) {
		auto entity = entityManager->CreateEntity(name);
		auto mesh = CreateRef<Graphics::Resources::Mesh>(name);
		mesh->LoadFromData(Graphics::Resources::Mesh::GenerateCube(0.5F));
		entity.AddComponent<MeshComponent>().SetMesh(mesh);
		entity.GetComponent<TransformComponent>().SetLocalPosition(pos);
	};
	addCube("CubeA", { -1.5f, 0.f, 2.f });
	addCube("CubeB", { 1.5f, 0.f, 2.f });

	SetupScene();
}

void Application::SetupScene() {
	auto &canvas = UI::Core::ViewSystem::Get()->GetLayer(UI::Core::UILayer::Screen);

	UI::StyleProperties rootStyle;
	rootStyle.width = UI::StyleLength::Grow();
	rootStyle.height = UI::StyleLength::Grow();
	rootStyle.alignItems = UI::AlignItems::Center;
	rootStyle.justifyContent = UI::JustifyContent::Center;
	canvas.GetRoot()->SetStyle(rootStyle);

	auto box = CreateUnique<UI::Core::View>();
	box->SetId("blue-box");

	UI::StyleProperties props{};
	props.backgroundColor = vec4(0.2f, 0.4f, 0.8f, 1.f);
	props.borderColor = vec4(1.F);
	props.borderWidth = 2.0F;
	props.width = UI::StyleLength::Pixel(300.F);
	props.height = UI::StyleLength::Pixel(150.F);
	props.borderRadius = vec4(20.0f, 20.0f, 20.0f, 20.0f);
	props.alignItems = UI::AlignItems::Center;
	props.justifyContent = UI::JustifyContent::Start;
	props.padding = UI::StyleEdges::All(UI::StyleLength::Pixel(10.f));
	box->SetStyle(props);

	m_Font = UI::Text::FontAtlas::CreateFromFile(
		*m_Ctx, "C:/Programming/Aquila/Resources/Engine/Fonts/Lexend-Regular.ttf", 32.f);

	auto *label = box->AddChild(CreateUnique<UI::Core::Label>("Hello World", m_Font.get()));
	label->SetId("my-label");

	UI::StyleProperties style{};
	style.color = vec4(1, 1, 1, 1);
	vec2 measured = dynamic_cast<UI::Core::Label *>(label)->Measure();
	style.width = UI::StyleLength::Pixel(measured.x);
	style.height = UI::StyleLength::Pixel(measured.y);
	label->SetStyle(style);

	canvas.GetRoot()->AddChild(std::move(box));
}

void Application::OnShutdown() {}

void Application::InitRendering(uint32 width, uint32 height) {
	RHI::VulkanShaderCompiler::Initialize();
	UI::Core::ViewSystem::Init(m_Window->GetWidth(), m_Window->GetHeight());

	m_Ctx = GFX::GfxContext::Create(*GetWindow().GetNativeWindow());

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
	m_Renderer->AddSystem<Rendering::GeometrySystem>();
	m_Renderer2D->AddSystem<UI::Rendering::ViewRenderingSystem>();
	// !
}

void Application::OnUpdate(f32 deltaTime) {
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
	if (!m_Swapchain->AcquireNextImage(imageIndex)) {
		return;
	}

	m_Renderer->SetSwapchainTarget(*m_Swapchain, imageIndex);
	m_Renderer2D->SetSwapchainTarget(*m_Swapchain, imageIndex);

	auto cmd = m_Ctx->CreateCommandList(RHI::CommandListType::Graphics, "MainFrame");
	cmd->Begin();

	m_RenderPipeline->Render(*cmd, *m_Scene, deltaTime);

	m_Ctx->SubmitFrame(*cmd, m_Swapchain.get(), imageIndex);

	m_Ctx->ProcessPendingDeletions();
}

void Application::OnEvent(Events::Event &event) {
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
}

} // namespace Aquila::Application
