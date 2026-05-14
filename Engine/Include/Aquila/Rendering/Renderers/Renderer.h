#pragma once
#include "Aquila/Rendering/Renderers/IRenderer.h"
#include "Aquila/Rendering/Systems/Base/IRenderingSystem.h"
#include "Aquila/Foundation/SharedConstants.h"
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
	void AddFinalPasses(Graphics::RG::RenderGraph &graph, FrameContext &ctx) override;
	void SetSwapchainTarget(GFX::GfxSwapchain &swapchain, uint32 imageIndex);

	template <typename T, typename... Args> T &AddSystem(Args &&...args) {
		static_assert(std::is_base_of_v<IRenderingSystem, T>);
		auto sys = CreateUnique<T>(std::forward<Args>(args)...);
		T &ref = *sys;
		sys->OnInit(*m_Ctx);
		m_Systems.push_back(std::move(sys));
		return ref;
	}

  private:
	GFX::GfxContext *m_Ctx = nullptr;
	std::vector<Unique<IRenderingSystem>> m_Systems;

	// Swapchain blit resources
	Ref<GFX::GfxDescriptorSetLayout> m_BlitLayout;
	Ref<GFX::GfxPipeline> m_BlitPipeline;
	// One descriptor set per frame-in-flight so the GPU can read set[N-1] while the CPU updates set[N].
	std::array<Ref<GFX::GfxDescriptorSet>, SharedConstants::MAX_FRAMES_IN_FLIGHT> m_BlitSets;
	Ref<GFX::GfxRenderPass> m_SwapchainPass;

	GFX::GfxSwapchain *m_Swapchain = nullptr;
	uint32 m_SwapchainImageIndex = 0;
	// Rotates 0..MAX_FRAMES_IN_FLIGHT-1; advanced in SetSwapchainTarget (once per frame).
	uint32 m_FrameSlot = SharedConstants::MAX_FRAMES_IN_FLIGHT - 1;
};

} // namespace Aquila::Rendering
