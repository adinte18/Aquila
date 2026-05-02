#pragma once

#include "Aquila/Rendering/Systems/Base/I2DRenderingSystem.h"

namespace Aquila::UI::Rendering {

class ViewRenderingSystem final : public Aquila::Rendering::I2DRenderingSystem {
  public:
	ViewRenderingSystem() = default;

	void OnInit(GFX::GfxContext &ctx) override;
	void AddPasses(Graphics::RG::RenderGraph &graph, Aquila::Rendering::FrameContext &ctx,
				   Graphics::QuadBatcher &r2d) override;
	void Render(Graphics::QuadBatcher &r2d, GFX::GfxCommandList &cmd) override;
	void OnResize(uint32 width, uint32 height) override;
	void OnShutdown() override {}

  private:
	uint32 m_Width = 0;
	uint32 m_Height = 0;
};

} // namespace Aquila::UI::Rendering
