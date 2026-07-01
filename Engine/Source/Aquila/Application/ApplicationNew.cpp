#include "Aquila/Application/ApplicationNew.h"
#include "Aquila/Foundation/Profiler.h"
#include "Aquila/Platform/Filesystem/VirtualFileSystem.h"
#include "Aquila/Platform/Input.h"

#include "Aquila/GFX/GfxCommandList.h"
#include "Aquila/Graphics/Core/QuadBatcher.h"
#include "Aquila/RHI/Vulkan/VulkanShaderCompiler.h"

#include <glm/gtc/matrix_transform.hpp>

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

	m_Window->SetEventCallback([this](Events::Event &event) { RouteWindowEvent(event); });

	m_Window->SetRefreshCallback([this]() {
		m_Timer->Tick();
		InternalUpdate(m_Timer->GetDeltaTime());
	});

	m_Timer->Start();
}

void Application::RouteWindowEvent(Events::Event &event) {
	Platform::Input::OnEvent(event);

	auto *source = event.GetSource();

	if (!source) {
		return;
	}

	if (source == m_Window.get()) {
		InternalOnMainWindowEvent(event);
		return;
	}

	for (auto &rw : m_SecondaryWindows) {
		if (source == rw->window.get()) {
			InternalOnSecondaryWindowEvent(*rw, event);
			return;
		}
	}
}

Application::~Application() {
	m_Ctx->WaitIdle();

	Platform::Filesystem::VirtualFileSystem::Shutdown();
	Foundation::Profiler::Profiler::Shutdown();
	Graphics::MaterialFactory::Shutdown();
	Rendering::FrameScheduler::Shutdown();

	m_Scene.reset();
	m_RenderPipeline.reset();
	m_SecondaryWindows.clear();
	m_SecondaryBatcher.reset();
	m_Swapchain.reset();
	m_Ctx.reset();

	// TODO: move to a generic shader compiler abstraction
	RHI::VulkanShaderCompiler::Shutdown();
}

void Application::Run() {
	InitRendering(m_Window->GetWidth(), m_Window->GetHeight());
	m_Scene = CreateUnique<Scene>("Main");
	OnInit();

	while (m_Running) {
		const bool hasFrames = Rendering::FrameScheduler::Get()->Consume();

		if (hasFrames) {
			m_Window->PollEvents();
		} else {
			m_Window->WaitEvents();
		}

		m_Window->FlushPendingEvents();

		for (auto &rw : m_SecondaryWindows) {
			rw->window->FlushPendingEvents();
		}

		bool anyClosing = false;
		for (const auto &rw : m_SecondaryWindows) {
			if (rw->window->ShouldClose()) {
				anyClosing = true;
				break;
			}
		}
		if (anyClosing) {
			m_Ctx->WaitIdle();
			std::erase_if(m_SecondaryWindows, [](const Unique<RenderWindow> &rw) {
				if (!rw->window->ShouldClose()) {
					return false;
				}
				if (rw->onClose) {
					rw->onClose();
				}
				Platform::Input::OnWindowDestroyed(rw->window.get());
				return true;
			});
		}

		if (m_Window->ShouldClose()) {
			m_Running = false;
			break;
		}

		if (hasFrames) {
			m_Timer->Tick();

			PROFILE_FRAME_BEGIN();
			InternalUpdate(m_Timer->GetDeltaTime());
			PROFILE_FRAME_END();
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

	m_SecondaryBatcher = CreateUnique<Graphics::QuadBatcher>(*m_Ctx);
}

RenderWindow &Application::CreateSecondaryWindow(uint32 width, uint32 height, const std::string &title) {
	auto rw = CreateUnique<RenderWindow>();
	rw->window = CreateUnique<Window>(width, height, title, false);
	rw->window->SetEventCallback([this](Events::Event &event) { RouteWindowEvent(event); });
	rw->window->SetRefreshCallback([this, p = rw.get()]() { RenderOneSecondaryWindow(*p); });
	rw->swapchain = m_Ctx->CreateSwapchain({
		.width = width,
		.height = height,
		.format = RHI::TextureFormat::BGRA8,
		.imageCount = 2,
		.vsync = false,
		.nativeWindowHandle = rw->window->GetNativeWindow(),
	});

	RenderWindow &ref = *rw;
	m_SecondaryWindows.push_back(std::move(rw));
	return ref;
}

void Application::EnsureWindowTargets(RenderWindow &rw, uint32 width, uint32 height) {
	if (!rw.msaaColor) {
		rw.msaaColor = m_Ctx->CreateTexture({
			.width = width,
			.height = height,
			.format = RHI::TextureFormat::BGRA8,
			.usage = RHI::TextureUsage::ColorAttachment,
			.samples = RHI::SampleCount::x4,
			.debugName = "SecondaryUIMSAA",
		});
		rw.renderPass = m_Ctx->CreateRenderPass({
			.colorAttachments = { {
				.texture = &rw.msaaColor->GetRHI(),
				.loadOp = RHI::AttachmentLoadOp::Clear,
				.storeOp = RHI::AttachmentStoreOp::DontCare,
			} },
			.useSwapchainAsResolve = true,
			.debugName = "SecondaryUI",
		});
	}
}

void Application::RenderSecondaryWindows() {
	for (auto &rw : m_SecondaryWindows) {
		RenderOneSecondaryWindow(*rw);
	}
}

void Application::RenderOneSecondaryWindow(RenderWindow &rw) {
	const uint32 width = rw.window->GetWidth();
	const uint32 height = rw.window->GetHeight();
	if (width == 0 || height == 0) {
		return;
	}

	if (rw.needsResize || rw.swapchain->NeedsResize()) {
		m_Ctx->WaitIdle();
		rw.swapchain->Resize(width, height);
		rw.needsResize = false;
		rw.msaaColor.reset(); // force render-target rebuild at the new size
		rw.renderPass.reset();
	}

	if (rw.onUpdate) {
		rw.onUpdate(m_Timer->GetDeltaTime());
	}

	if (!rw.onRender) {
		uint32 imageIndex = 0;
		if (!rw.swapchain->AcquireNextImage(imageIndex, false)) {
			return;
		}
		m_Ctx->GetDevice().PresentFrame(rw.swapchain->GetRHI(), imageIndex, vec4(0.f, 0.f, 0.f, 1.f));
		return;
	}

	EnsureWindowTargets(rw, width, height);

	uint32 imageIndex = 0;
	if (!rw.swapchain->AcquireNextImage(imageIndex, false)) {
		return;
	}

	auto cmd = m_Ctx->CreateCommandList(RHI::CommandListType::Graphics, "SecondaryUICmd");
	cmd->Begin();

	const mat4 ortho = glm::ortho(0.f, static_cast<float>(width), static_cast<float>(height), 0.f, -1.f, 1.f);
	rw.renderPass->Begin(*cmd, rw.swapchain.get(), imageIndex);
	m_SecondaryBatcher->Begin(*cmd, RHI::TextureFormat::BGRA8, RHI::SampleCount::x4, ortho);
	rw.onRender(*m_SecondaryBatcher, *cmd);
	m_SecondaryBatcher->End();
	rw.renderPass->End(*cmd);

	m_Ctx->SubmitFrame(*cmd, rw.swapchain.get(), imageIndex);
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

	RenderSecondaryWindows();
}

void Application::InternalOnMainWindowEvent(Events::Event &event) {
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

void Application::InternalOnSecondaryWindowEvent(RenderWindow &rw, Events::Event &event) {
	Rendering::FrameScheduler::Get()->RequestFrame();

	Events::EventDispatcher dispatcher(event);
	dispatcher.Dispatch<Events::WindowResizeEvent>([&](auto &) {
		rw.needsResize = true; // swapchain + targets rebuilt in RenderOneSecondaryWindow
		return false;
	});

	if (rw.onEvent) {
		rw.onEvent(event);
	}
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
