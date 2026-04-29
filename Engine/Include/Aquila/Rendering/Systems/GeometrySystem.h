#pragma once
#include "Aquila/Rendering/Systems/RenderingSystemBase.h"
#include "Aquila/GFX/GfxPipeline.h"

namespace Aquila::Rendering {

class GeometrySystem : public RenderingSystemBase {
  public:
	GeometrySystem() = default;
	~GeometrySystem() override = default;

	void OnInit(GFX::GfxContext &ctx) override;
	void AddPasses(Graphics::RG::RenderGraph &graph, FrameContext &ctx) override;

  private:
	Ref<GFX::GfxPipeline> m_Pipeline;
};

} // namespace Aquila::Rendering
