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

Application::Application(const ApplicationSpec &spec) : m_spec(spec) {
	m_timer = create_unique<Foundation::Stopwatch>();
	m_window = create_unique<Window>(spec.width, spec.height, spec.name);
	Foundation::Profiler::Profiler::init();
	Platform::Filesystem::VirtualFileSystem::init();

	m_window->set_event_callback([this](Events::Event &event) { route_window_event(event); });

	m_window->set_refresh_callback([this]() {
		m_timer->tick();
		internal_update(m_timer->get_delta_time());
	});

	m_timer->start();
}

void Application::route_window_event(Events::Event &event) {
	Platform::Input::on_event(event);

	auto *source = event.get_source();

	if (!source) {
		return;
	}

	if (source == m_window.get()) {
		internal_on_main_window_event(event);
		return;
	}

	for (auto &rw : m_secondary_windows) {
		if (source == rw->window.get()) {
			internal_on_secondary_window_event(*rw, event);
			return;
		}
	}
}

Application::~Application() {
	m_ctx->wait_idle();

	Platform::Filesystem::VirtualFileSystem::shutdown();
	Foundation::Profiler::Profiler::shutdown();
	Graphics::MaterialFactory::shutdown();
	Rendering::FrameScheduler::shutdown();

	m_scene.reset();
	m_render_pipeline.reset();
	m_secondary_windows.clear();
	m_secondary_batcher.reset();
	m_swapchain.reset();
	m_ctx.reset();

	// TODO: move to a generic shader compiler abstraction
	RHI::VulkanShaderCompiler::shutdown();
}

void Application::run() {
	init_rendering(m_window->get_width(), m_window->get_height());
	m_scene = create_unique<Scene>("Main");
	on_init();

	while (m_running) {
		const bool has_frames = Rendering::FrameScheduler::get()->consume();

		if (has_frames) {
			m_window->poll_events();
		} else {
			m_window->wait_events();
		}

		m_window->flush_pending_events();

		for (auto &rw : m_secondary_windows) {
			rw->window->flush_pending_events();
		}

		bool any_closing = false;
		for (const auto &rw : m_secondary_windows) {
			if (rw->window->should_close()) {
				any_closing = true;
				break;
			}
		}
		if (any_closing) {
			m_ctx->wait_idle();
			std::erase_if(m_secondary_windows, [](const Unique<RenderWindow> &rw) {
				if (!rw->window->should_close()) {
					return false;
				}
				if (rw->on_close) {
					rw->on_close();
				}
				Platform::Input::on_window_destroyed(rw->window.get());
				return true;
			});
		}

		if (m_window->should_close()) {
			m_running = false;
			break;
		}

		if (has_frames) {
			m_timer->tick();

			PROFILE_FRAME_BEGIN();
			internal_update(m_timer->get_delta_time());
			PROFILE_FRAME_END();
		}
	}

	m_ctx->wait_idle();
	on_shutdown();
}

void Application::close() {
	m_running = false;
}

void Application::init_rendering(Uint32 width, Uint32 height) {
	// TODO: replace with a generic shader compiler abstraction
	RHI::VulkanShaderCompiler::initialize();
	Graphics::MaterialFactory::init();
	Rendering::FrameScheduler::init();

	using namespace Platform::Filesystem;
	VirtualFileSystem::get()->mount("/resources", create_ref<NativeFileSystem>(SharedConstants::RESOURCES_DIR));
	VirtualFileSystem::get()->mount("/shaders", create_ref<NativeFileSystem>(SharedConstants::SHADERS_DIR));

	m_ctx = GFX::GfxContext::create(*get_window().get_native_window());

	m_swapchain = m_ctx->create_swapchain({
		.width = width,
		.height = height,
		.format = RHI::TextureFormat::BGRA8,
		.image_count = 2,
		.vsync = false,
	});

	m_render_pipeline = create_unique<Rendering::RenderPipeline>(*m_ctx, width, height);
	m_renderer = &m_render_pipeline->add<Rendering::Renderer>();
	m_renderer2_d = &m_render_pipeline->add<Rendering::Renderer2D>();

	m_renderer->add_system<Rendering::DepthPrepassSystem>();
	m_renderer->add_system<Rendering::ClusterComputeSystem>();
	m_renderer->add_system<Rendering::LightCullingSystem>();
	m_renderer->add_system<Rendering::GeometrySystem>();

	m_secondary_batcher = create_unique<Graphics::QuadBatcher>(*m_ctx);
}

RenderWindow &Application::create_secondary_window(Uint32 width, Uint32 height, const std::string &title) {
	auto rw = create_unique<RenderWindow>();
	rw->window = create_unique<Window>(width, height, title, false);
	rw->window->set_event_callback([this](Events::Event &event) { route_window_event(event); });
	rw->window->set_refresh_callback([this, p = rw.get()]() { render_one_secondary_window(*p); });
	rw->swapchain = m_ctx->create_swapchain({
		.width = width,
		.height = height,
		.format = RHI::TextureFormat::BGRA8,
		.image_count = 2,
		.vsync = false,
		.native_window_handle = rw->window->get_native_window(),
	});

	RenderWindow &ref = *rw;
	m_secondary_windows.push_back(std::move(rw));
	return ref;
}

void Application::ensure_window_targets(RenderWindow &rw, Uint32 width, Uint32 height) {
	if (!rw.msaa_color) {
		rw.msaa_color = m_ctx->create_texture({
			.width = width,
			.height = height,
			.format = RHI::TextureFormat::BGRA8,
			.usage = RHI::TextureUsage::ColorAttachment,
			.samples = RHI::SampleCount::X4,
			.debug_name = "SecondaryUIMSAA",
		});
		rw.render_pass = m_ctx->create_render_pass({
			.color_attachments = { {
				.texture = &rw.msaa_color->get_rhi(),
				.load_op = RHI::AttachmentLoadOp::Clear,
				.store_op = RHI::AttachmentStoreOp::DontCare,
			} },
			.use_swapchain_as_resolve = true,
			.debug_name = "SecondaryUI",
		});
	}
}

void Application::render_secondary_windows() {
	for (auto &rw : m_secondary_windows) {
		render_one_secondary_window(*rw);
	}
}

void Application::render_one_secondary_window(RenderWindow &rw) {
	const Uint32 width = rw.window->get_width();
	const Uint32 height = rw.window->get_height();
	if (width == 0 || height == 0) {
		return;
	}

	if (rw.needs_resize || rw.swapchain->needs_resize()) {
		m_ctx->wait_idle();
		rw.swapchain->resize(width, height);
		rw.needs_resize = false;
		rw.msaa_color.reset(); // force render-target rebuild at the new size
		rw.render_pass.reset();
	}

	if (rw.on_update) {
		rw.on_update(m_timer->get_delta_time());
	}

	if (!rw.on_render) {
		Uint32 image_index = 0;
		if (!rw.swapchain->acquire_next_image(image_index, false)) {
			return;
		}
		m_ctx->get_device().present_frame(rw.swapchain->get_rhi(), image_index, Vec4(0.F, 0.F, 0.F, 1.F));
		return;
	}

	ensure_window_targets(rw, width, height);

	Uint32 image_index = 0;
	if (!rw.swapchain->acquire_next_image(image_index, false)) {
		return;
	}

	auto cmd = m_ctx->create_command_list(RHI::CommandListType::Graphics, "SecondaryUICmd");
	cmd->begin();

	const Mat4 ortho = glm::ortho(0.F, static_cast<float>(width), static_cast<float>(height), 0.F, -1.F, 1.F);
	rw.render_pass->begin(*cmd, rw.swapchain.get(), image_index);
	m_secondary_batcher->begin(*cmd, RHI::TextureFormat::BGRA8, RHI::SampleCount::X4, ortho);
	rw.on_render(*m_secondary_batcher, *cmd);
	m_secondary_batcher->end();
	rw.render_pass->end(*cmd);

	m_ctx->submit_frame(*cmd, rw.swapchain.get(), image_index);
}

void Application::internal_update(F32 delta_time) {
	PROFILE_SCOPE("OnUpdate");

	if (m_pending_resize || m_swapchain->needs_resize()) {
		m_pending_resize = false;
		const Uint32 width = m_window->get_width();
		const Uint32 height = m_window->get_height();
		if (width == 0 || height == 0) {
			return;
		}
		handle_resize();
	}

	Uint32 image_index = 0;
	{
		PROFILE_SCOPE("AcquireNextImage");
		if (!m_swapchain->acquire_next_image(image_index)) {
			return;
		}
	}

	m_renderer->set_swapchain_target(*m_swapchain, image_index);
	m_renderer2_d->set_swapchain_target(*m_swapchain, image_index);

	Uint32 frame_slot = m_swapchain->get_current_frame_slot();
	auto &cmd = m_ctx->acquire_frame_command_list(frame_slot);
	cmd.begin();

	on_pre_render(delta_time);

	{
		Graphics::MaterialFactory::get()->tick(*m_ctx);
	}

	{
		PROFILE_SCOPE("RenderPipeline::Render");
		m_render_pipeline->render(cmd, *m_scene, delta_time);
	}

	{
		PROFILE_SCOPE("SubmitFrame");
		m_ctx->submit_frame(cmd, m_swapchain.get(), image_index);
	}

	render_secondary_windows();
}

void Application::internal_on_main_window_event(Events::Event &event) {
	const bool is_cursor_event = (event.get_category() & Events::EventCategory::Mouse) &&
		!(event.get_category() & Events::EventCategory::MouseButton);
	if (!is_cursor_event) {
		Rendering::FrameScheduler::get()->request_frame();
	}

	on_event(event);

	Events::EventDispatcher dispatcher(event);

	dispatcher.dispatch<Events::WindowCloseEvent>([this](Events::WindowCloseEvent &) {
		m_running = false;
		return true;
	});

	dispatcher.dispatch<Events::WindowResizeEvent>([this](Events::WindowResizeEvent &ev) {
		if (ev.get_width() > 0 && ev.get_height() > 0) {
			m_pending_resize = true;
		}
		return true;
	});
}

void Application::internal_on_secondary_window_event(RenderWindow &rw, Events::Event &event) {
	Rendering::FrameScheduler::get()->request_frame();

	Events::EventDispatcher dispatcher(event);
	dispatcher.dispatch<Events::WindowResizeEvent>([&](auto &) {
		rw.needs_resize = true; // swapchain + targets rebuilt in RenderOneSecondaryWindow
		return false;
	});

	if (rw.on_event) {
		rw.on_event(event);
	}
}

void Application::handle_resize() {
	const Uint32 width = m_window->get_width();
	const Uint32 height = m_window->get_height();
	if (width == 0 || height == 0) {
		return;
	}

	m_ctx->wait_idle();
	m_swapchain->resize(width, height);
	m_render_pipeline->resize(width, height);

	on_resize(width, height);
}

} // namespace Aquila::Application
