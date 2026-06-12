#pragma once

#include "Aquila/Rendering/Systems/Base/IRenderingSystem.h"
#include "Aquila/Graphics/Core/QuadBatcher.h"
#include "Aquila/GFX/GfxRenderpass.h"
#include "Aquila/GFX/GfxTexture.h"

namespace Aquila::UI::Rendering {

class ViewRenderingSystem final : public Aquila::Rendering::IRenderingSystem {
  public:
	ViewRenderingSystem() = default;

	void OnInit(GFX::GfxContext &ctx) override;
	void AddPasses(Graphics::RG::RenderGraph &graph, Aquila::Rendering::FrameContext &ctx) override;
	void BlitToSwapchain(Graphics::RG::RenderGraph &graph, Aquila::Rendering::FrameContext &ctx) override;
	void OnResize(uint32 width, uint32 height) override;
	void OnShutdown() override {}

  private:
	void RebuildOverlayResources(uint32 w, uint32 h);

	GFX::GfxContext *m_Ctx = nullptr;
	Unique<Graphics::QuadBatcher> m_R2D;
	Ref<GFX::GfxTexture> m_MSAAColor;
	Ref<GFX::GfxRenderPass> m_OverlayPass;
	uint32 m_Width = 0;
	uint32 m_Height = 0;
};

} // namespace Aquila::UI::Rendering
