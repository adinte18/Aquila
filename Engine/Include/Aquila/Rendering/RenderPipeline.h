#pragma once
#include "Aquila/Foundation/Defines.h"
#include "Aquila/Foundation/PrimitiveTypes.h"
#include "Aquila/Foundation/SharedConstants.h"
#include "Aquila/Graphics/RenderGraph/RGGraph.h"
#include "Aquila/Rendering/Renderers/IRenderer.h"
#include "Aquila/Rendering/FrameContext.h"
#include "Aquila/GFX/GfxTexture.h"

namespace Aquila::GFX {
class GfxContext;
class GfxCommandList;
} // namespace Aquila::GFX

namespace Aquila::SceneManagement {
class Scene;
}

namespace Aquila::Rendering {

class RenderPipeline {
  public:
	RenderPipeline(GFX::GfxContext &ctx, Uint32 width, Uint32 height);
	~RenderPipeline();

	AQUILA_NONCOPYABLE(RenderPipeline);
	AQUILA_NONMOVEABLE(RenderPipeline);

	template <typename T, typename... Args> T &add(Args &&...args) {
		static_assert(std::is_base_of_v<IRenderer, T>, "T must derive from IRenderer");
		auto renderer = std::make_unique<T>(std::forward<Args>(args)...);
		T &ref = *renderer;
		renderer->on_init(m_ctx);
		m_renderers.push_back(std::move(renderer));
		return ref;
	}

	void render(GFX::GfxCommandList &cmd, SceneManagement::Scene &scene, F32 delta_time);
	void render(GFX::GfxCommandList &cmd, SceneManagement::Scene &scene, F32 delta_time, Uint32 width, Uint32 height);
	void resize(Uint32 width, Uint32 height);

	[[nodiscard]] GFX::GfxTexture &get_output() const { return *m_scene_color; }
	[[nodiscard]] Uint32 get_width() const { return m_width; }
	[[nodiscard]] Uint32 get_height() const { return m_height; }

  private:
	void build_frame_context(SceneManagement::Scene &scene, F32 delta_time, FrameContext &out);
	void rebuild_targets();

	GFX::GfxContext &m_ctx;
	Graphics::RG::RenderGraph m_graph;
	std::vector<Unique<IRenderer>> m_renderers;

	Ref<GFX::GfxTexture> m_scene_color;
	Ref<GFX::GfxTexture> m_depth_tex;

	Uint32 m_width = 0;
	Uint32 m_height = 0;

	// Rotates 0..MAX_FRAMES_IN_FLIGHT-1 each Render() call, matching swapchain fence rotation.
	Uint32 m_frame_slot = 0;
};

} // namespace Aquila::Rendering
