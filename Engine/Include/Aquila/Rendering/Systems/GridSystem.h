#pragma once
#include "Aquila/Rendering/Systems/RenderingSystemBase.h"
#include "Aquila/Graphics/Shader/ReloadablePipeline.h"

namespace Aquila::Rendering {

class GridSystem : public RenderingSystemBase {
  public:
	GridSystem() = default;
	~GridSystem() override = default;

	void on_init(GFX::GfxContext &ctx) override;
	void add_passes(Graphics::RG::RenderGraph &graph, FrameContext &ctx) override;

  private:
	Ref<Graphics::Shader::ReloadablePipeline> m_pipeline;
};

} // namespace Aquila::Rendering
