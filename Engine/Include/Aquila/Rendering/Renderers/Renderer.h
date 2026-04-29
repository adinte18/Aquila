#pragma once
#include "Aquila/Rendering/Renderers/IRenderer.h"
#include "Aquila/Rendering/Systems/Base/IRenderingSystem.h"
#include "Aquila/GFX/GfxDescriptorSet.h"
#include "Aquila/GFX/GfxPipeline.h"
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

	void OnInit(GFX::GfxContext &ctx) override;
	void OnShutdown() override;
	void AddPasses(Graphics::RG::RenderGraph &graph, FrameContext &ctx) override;

	template <typename T, typename... Args> T &AddSystem(Args &&...args) {
		static_assert(std::is_base_of_v<IRenderingSystem, T>);
		auto sys = CreateUnique<T>(std::forward<Args>(args)...);
		T &ref = *sys;
		sys->OnInit(*m_Ctx);
		m_Systems.push_back(std::move(sys));
		return ref;
	}

	// Call each frame before RenderPipeline::Render() to blit hSceneColor into
	// the given swapchain image as the last render graph pass.
	void SetSwapchainTarget(GFX::GfxSwapchain &swapchain, uint32 imageIndex);

  private:
	GFX::GfxContext *m_Ctx = nullptr;
	std::vector<Unique<IRenderingSystem>> m_Systems;

	// Swapchain blit resources
	Ref<GFX::GfxDescriptorSetLayout> m_BlitLayout;
	Ref<GFX::GfxPipeline> m_BlitPipeline;
	Ref<GFX::GfxDescriptorSet> m_BlitSet;
	Ref<GFX::GfxRenderPass> m_SwapchainPass;

	GFX::GfxSwapchain *m_Swapchain = nullptr;
	uint32 m_SwapchainImageIndex = 0;
};

} // namespace Aquila::Rendering
