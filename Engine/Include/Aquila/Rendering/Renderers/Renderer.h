#pragma once
#include "Aquila/Rendering/Renderers/IRenderer.h"
#include "Aquila/Rendering/Systems/Base/IRenderingSystem.h"
#include "Aquila/Foundation/SharedConstants.h"
#include "Aquila/Graphics/Shader/ReloadablePipeline.h"
#include "Aquila/GFX/GfxDescriptorSet.h"
#include "Aquila/GFX/GfxRenderpass.h"

namespace Aquila::GFX {
class GfxSwapchain;
}

namespace Aquila::Rendering {

class Renderer : public IRenderer {
  public:
	Renderer() = default;
	~Renderer() override = default;

	AQUILA_NONCOPYABLE(Renderer);
	AQUILA_NONMOVEABLE(Renderer);

	void on_init(GFX::GfxContext &ctx) override;
	void on_shutdown() override;
	void add_passes(Graphics::RG::RenderGraph &graph, FrameContext &ctx) override;
	void blit_to_swapchain(Graphics::RG::RenderGraph &graph, FrameContext &ctx) override;
	void on_resize(Uint32 width, Uint32 height) override;
	void set_swapchain_target(GFX::GfxSwapchain &swapchain, Uint32 image_index);

	template <typename T, typename... Args> T &add_system(Args &&...args) {
		static_assert(std::is_base_of_v<IRenderingSystem, T>);
		auto sys = std::make_unique<T>(std::forward<Args>(args)...);
		T &ref = *sys;
		sys->on_init(*m_ctx);
		m_systems.push_back(std::move(sys));
		return ref;
	}

  private:
	GFX::GfxContext *m_ctx = nullptr;
	std::vector<Unique<IRenderingSystem>> m_systems;

	// Swapchain blit resources
	Ref<GFX::GfxDescriptorSetLayout> m_blit_layout;
	Ref<Graphics::Shader::ReloadablePipeline> m_blit_pipeline;
	// One descriptor set per frame-in-flight so the GPU can read set[N-1] while the CPU updates set[N].
	std::array<Ref<GFX::GfxDescriptorSet>, SharedConstants::MAX_FRAMES_IN_FLIGHT> m_blit_sets;
	Ref<GFX::GfxRenderPass> m_swapchain_pass;

	GFX::GfxSwapchain *m_swapchain = nullptr;
	Uint32 m_swapchain_image_index = 0;
	// Rotates 0..MAX_FRAMES_IN_FLIGHT-1; advanced in SetSwapchainTarget (once per frame).
	Uint32 m_frame_slot = SharedConstants::MAX_FRAMES_IN_FLIGHT - 1;
};

} // namespace Aquila::Rendering
