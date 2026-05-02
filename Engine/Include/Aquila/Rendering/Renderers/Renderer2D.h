#pragma once
#include "Aquila/Rendering/Renderers/IRenderer.h"
#include "Aquila/Rendering/Systems/Base/I2DRenderingSystem.h"
#include "Aquila/Graphics/Core/QuadBatcher.h"
#include "Aquila/GFX/GfxRenderpass.h"

namespace Aquila::GFX {
class GfxSwapchain;
}

namespace Aquila::Rendering {

class Renderer2D : public IRenderer {
  public:
	Renderer2D() = default;
	~Renderer2D() override = default;

	AQUILA_NONCOPYABLE(Renderer2D);
	AQUILA_NONMOVEABLE(Renderer2D);

	void OnInit(GFX::GfxContext &ctx) override;
	void OnShutdown() override;
	void AddPasses(Graphics::RG::RenderGraph &graph, FrameContext &ctx) override;
	void AddFinalPasses(Graphics::RG::RenderGraph &graph, FrameContext &ctx) override;

	void SetSwapchainTarget(GFX::GfxSwapchain &swapchain, uint32 imageIndex);

	template <typename T, typename... Args> T &AddSystem(Args &&...args) {
		static_assert(std::is_base_of_v<I2DRenderingSystem, T>);
		auto sys = CreateUnique<T>(std::forward<Args>(args)...);
		T &ref = *sys;
		sys->OnInit(*m_Ctx);
		m_Systems.push_back(std::move(sys));
		return ref;
	}

  private:
	GFX::GfxContext *m_Ctx = nullptr;
	Unique<Graphics::QuadBatcher> m_R2D;
	std::vector<Unique<I2DRenderingSystem>> m_Systems;

	Ref<GFX::GfxRenderPass> m_SwapchainPass;
	GFX::GfxSwapchain *m_Swapchain = nullptr;
	uint32 m_SwapchainImageIndex = 0;
};

} // namespace Aquila::Rendering
