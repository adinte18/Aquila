#pragma once
#include "Aquila/Rendering/Renderers/IRenderer.h"
#include "Aquila/Rendering/Systems/Base/IRenderingSystem.h"

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
	void BlitToSwapchain(Graphics::RG::RenderGraph &graph, FrameContext &ctx) override;
	void OnResize(uint32 width, uint32 height) override;

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

	GFX::GfxSwapchain *m_Swapchain = nullptr;
	uint32 m_SwapchainImageIndex = 0;
};

} // namespace Aquila::Rendering
