#pragma once
#include "Aquila/Rendering/Systems/RenderingSystemBase.h"

namespace Aquila::Rendering {

class GridSystem : public RenderingSystemBase {
  public:
	GridSystem() = default;
	~GridSystem() override = default;

	void on_init(GFX::GfxContext &ctx) override;
	void add_passes(Graphics::RG::RenderGraph &graph, FrameContext &ctx) override;
};

} // namespace Aquila::Rendering
