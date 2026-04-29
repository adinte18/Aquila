#pragma once
#include "Aquila/Rendering/FrameContext.h"
#include "Aquila/Rendering/Systems/RenderingSystemBase.h"
#include "Aquila/GFX/GfxPipeline.h"

namespace Aquila::Rendering {

class DepthPrepassSystem : public RenderingSystemBase {
  public:
	DepthPrepassSystem() = default;
	~DepthPrepassSystem() override = default;

	void OnInit(GFX::GfxContext &ctx) override;
	void AddPasses(Graphics::RG::RenderGraph &graph, FrameContext &ctx) override;

  private:
	Ref<GFX::GfxPipeline> m_Pipeline;
};

} // namespace Aquila::Rendering
