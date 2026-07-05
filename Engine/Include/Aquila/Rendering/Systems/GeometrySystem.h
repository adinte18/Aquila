#pragma once
#include "Aquila/Rendering/Systems/RenderingSystemBase.h"

namespace Aquila::Rendering {

class GeometrySystem : public RenderingSystemBase {
  public:
	GeometrySystem() = default;
	~GeometrySystem() override = default;

	void on_init(GFX::GfxContext &ctx) override;
	void add_passes(Graphics::RG::RenderGraph &graph, FrameContext &ctx) override;
};

} // namespace Aquila::Rendering
