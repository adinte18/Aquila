#include "Aquila/Application/ApplicationNew.h"
#include "Aquila/Foundation/Profiler.h"
#include "Aquila/Platform/Filesystem/VirtualFileSystem.h"
#include "Aquila/Platform/Input.h"
#include "Aquila/Foundation/Macros.h"

#include "Aquila/GFX/GfxCommandList.h"
#include "Aquila/RHI/Vulkan/VulkanShaderCompiler.h"

#include "Aquila/Rendering/Systems/LightCullingSystem.h"
#include "Aquila/Graphics/Material/MaterialFactory.h"
#include "Aquila/Foundation/SharedConstants.h"

#include "Aquila/Rendering/Systems/DepthPrepassSystem.h"
#include "Aquila/Rendering/Systems/GeometrySystem.h"
#include "Aquila/Rendering/Systems/ComputeTestSystem.h"
#include "Aquila/Rendering/FrameScheduler.h"
#include "Aquila/Platform/Filesystem/NativeFileSystem.h"

namespace Aquila::Application {

using namespace SceneManagement;

Application::Application(const ApplicationSpec &spec) : m_Spec(spec) {
	m_Timer = CreateUnique<Foundation::Stopwatch>();
	m_Window = CreateUnique<Window>(spec.Width, spec.Height, spec.Name);
	Foundation::Profiler::Profiler::Init();
	Platform::Filesystem::VirtualFileSystem::Init();

	m_Window->SetEventCallback([this](Events::Event &event) {
		Aquila::Platform::Input::OnEvent(event);
		InternalOnEvent(event);
	});

	m_Window->SetRefreshCallback([this]() {
		m_Timer->Tick();
		InternalUpdate(m_Timer->GetDeltaTime());
	});

	m_Timer->Start();
}

Application::~Application() {
	m_Ctx->WaitIdle();

	Platform::Filesystem::VirtualFileSystem::Shutdown();
	Foundation::Profiler::Profiler::Shutdown();
	Graphics::MaterialFactory::Shutdown();
	Rendering::FrameScheduler::Shutdown();

	m_Scene.reset();
	m_RenderPipeline.reset();
	m_Swapchain.reset();
	m_Ctx.reset();

	// TODO: move to a generic shader compiler abstraction
	RHI::VulkanShaderCompiler::Shutdown();
}

void Application::Run() {
	InitRendering(m_Window->GetWidth(), m_Window->GetHeight());
	m_Scene = CreateUnique<Scene>("Main");
	OnInit();

	while (m_Running && !m_Window->ShouldClose()) {
		if (Rendering::FrameScheduler::Get()->Consume()) {
			m_Window->PollEvents();
			m_Timer->Tick();
			PROFILE_FRAME_BEGIN();
			InternalUpdate(m_Timer->GetDeltaTime());
			PROFILE_FRAME_END();
			PROFILE_PRINT_SUMMARY_EVERY_N_FRAMES(1000);
		} else {
			m_Window->WaitEvents();
		}
	}

	m_Ctx->WaitIdle();
	OnShutdown();
}

void Application::Close() {
	m_Running = false;
}

void Application::InitRendering(uint32 width, uint32 height) {
	// TODO: replace with a generic shader compiler abstraction
	RHI::VulkanShaderCompiler::Initialize();
	Graphics::MaterialFactory::Init();
	Rendering::FrameScheduler::Init();

	using namespace Platform::Filesystem;
	VirtualFileSystem::Get()->Mount("/resources", CreateRef<NativeFileSystem>(SharedConstants::RESOURCES_DIR));
	VirtualFileSystem::Get()->Mount("/shaders", CreateRef<NativeFileSystem>(SharedConstants::SHADERS_DIR));

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

	m_Renderer->AddSystem<Rendering::DepthPrepassSystem>();
	m_Renderer->AddSystem<Rendering::ClusterComputeSystem>();
	m_Renderer->AddSystem<Rendering::LightCullingSystem>();
	m_Renderer->AddSystem<Rendering::GeometrySystem>();
}

void Application::InternalUpdate(f32 deltaTime) {
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

	uint32 frameSlot = m_Swapchain->GetCurrentFrameSlot();
	auto &cmd = m_Ctx->AcquireFrameCommandList(frameSlot);
	cmd.Begin();

	OnPreRender(deltaTime);

	{
		Graphics::MaterialFactory::Get()->Tick(*m_Ctx);
	}

	{
		PROFILE_SCOPE("RenderPipeline::Render");
		m_RenderPipeline->Render(cmd, *m_Scene, deltaTime);
	}

	{
		PROFILE_SCOPE("SubmitFrame");
		m_Ctx->SubmitFrame(cmd, m_Swapchain.get(), imageIndex);
	}
}

void Application::InternalOnEvent(Events::Event &event) {
	const bool isCursorEvent = (event.GetCategory() & Events::EventCategory::Mouse) &&
							   !(event.GetCategory() & Events::EventCategory::MouseButton);
	if (!isCursorEvent) {
		Rendering::FrameScheduler::Get()->RequestFrame();
	}

	OnEvent(event);

	Events::EventDispatcher dispatcher(event);

	dispatcher.Dispatch<Events::WindowCloseEvent>([this](Events::WindowCloseEvent &) {
		m_Running = false;
		return true;
	});

	dispatcher.Dispatch<Events::WindowResizeEvent>([this](Events::WindowResizeEvent &ev) {
		if (ev.GetWidth() > 0 && ev.GetHeight() > 0) {
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

	OnResize(width, height);
}

} // namespace Aquila::Application
