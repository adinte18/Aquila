#pragma once
#include "Aquila/Rendering/Systems/RenderingSystemBase.h"

namespace Aquila::Rendering {

class GeometrySystem : public RenderingSystemBase {
  public:
	GeometrySystem() = default;
	~GeometrySystem() override = default;

	void OnInit(GFX::GfxContext &ctx) override;
	void AddPasses(Graphics::RG::RenderGraph &graph, FrameContext &ctx) override;
};

} // namespace Aquila::Rendering
