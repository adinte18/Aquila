#ifndef AQUILA_APPLICATION_H
#define AQUILA_APPLICATION_H

#include "Aquila/Foundation/Defines.h"
#include "Aquila/Foundation/SharedConstants.h"
#include "Aquila/Foundation/Timer.h"
#include "Aquila/Application/Window.h"
#include "Aquila/GFX/GfxContext.h"
#include "Aquila/GFX/GfxSwapchain.h"
#include "Aquila/GFX/GfxTexture.h"
#include "Aquila/Scene/Scene.h"
#include "Aquila/Rendering/RenderPipeline.h"
#include "Aquila/Rendering/Renderers/Renderer.h"
#include "Aquila/Rendering/Renderers/Renderer2D.h"

#include <array>
#include <vector>

struct ApplicationSpec {
	std::string name = "Aquila";
	Uint32 width = 1920;
	Uint32 height = 1080;
	bool start_hidden = false;
};

namespace Aquila::Graphics {
class QuadBatcher;
}

namespace Aquila::Application {

struct RenderWindow {
	Unique<Window> window;
	Ref<GFX::GfxSwapchain> swapchain;

	Ref<GFX::GfxTexture> msaa_color;
	Ref<GFX::GfxRenderPass> render_pass;
	bool needs_resize = false;

	Delegate<void(F32)> on_update;
	Delegate<void(Graphics::QuadBatcher &, GFX::GfxCommandList &)> on_render;
	Delegate<void(Events::Event &)> on_event;
	Delegate<void()> on_close;
};

class Application {
  public:
	explicit Application(const ApplicationSpec &spec);
	virtual ~Application();

	AQUILA_NONCOPYABLE(Application);
	AQUILA_NONMOVEABLE(Application);

	void run();
	void close();

	Window &get_window() { return *m_window; }

  protected:
	virtual void on_init() {}
	virtual void on_shutdown() {}
	virtual void on_pre_render(F32 delta_time) {}
	virtual void on_event(Events::Event &event) {}
	virtual void on_resize(Uint32 width, Uint32 height) {}
	virtual void on_render_resize(Uint32 width, Uint32 height) {}

	// Request the 3D scene render target to change size, applied before the next frame.
	// Independent of the window/swapchain size so an editor can render at its viewport size.
	void request_render_resize(Uint32 width, Uint32 height);

	GFX::GfxContext &get_context() { return *m_ctx; }
	GFX::GfxTexture &get_render_output() { return m_render_pipeline->get_output(); }
	SceneManagement::Scene &get_scene() { return *m_scene; }
	Rendering::RenderPipeline &get_render_pipeline() { return *m_render_pipeline; }
	Rendering::Renderer &get_renderer() { return *m_renderer; }
	Rendering::Renderer2D &get_renderer2_d() { return *m_renderer2_d; }

	RenderWindow &create_secondary_window(Uint32 width, Uint32 height, const std::string &title);

  private:
	void route_window_event(Events::Event &event);
	void internal_update(F32 delta_time);
	void internal_on_main_window_event(Events::Event &event);
	void internal_on_secondary_window_event(RenderWindow &rw, Events::Event &event);
	void handle_resize();
	void init_rendering(Uint32 width, Uint32 height);

	void render_secondary_windows();
	void render_one_secondary_window(RenderWindow &rw);
	void ensure_window_targets(RenderWindow &rw, Uint32 width, Uint32 height);

	ApplicationSpec m_spec;
	Unique<Window> m_window;
	Unique<Foundation::Stopwatch> m_timer;
	bool m_running = true;
	bool m_pending_resize = false;
	bool m_frame_in_progress = false;

	Uint32 m_render_width = 0;
	Uint32 m_render_height = 0;
	bool m_render_resize_pending = false;
	Uint32 m_next_render_width = 0;
	Uint32 m_next_render_height = 0;

	Unique<GFX::GfxContext> m_ctx;
	Ref<GFX::GfxSwapchain> m_swapchain;
	Unique<SceneManagement::Scene> m_scene;
	Unique<Rendering::RenderPipeline> m_render_pipeline;
	Rendering::Renderer *m_renderer = nullptr;
	Rendering::Renderer2D *m_renderer2_d = nullptr;

	Unique<Graphics::QuadBatcher> m_secondary_batcher;
	std::vector<Unique<RenderWindow>> m_secondary_windows;
};

} // namespace Aquila::Application

#endif
