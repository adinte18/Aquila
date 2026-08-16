#pragma once
#include "Aquila/Rendering/FrameContext.h"
#include "Aquila/Rendering/Systems/RenderingSystemBase.h"
#include "Aquila/Graphics/Shader/ReloadablePipeline.h"

namespace Aquila::Rendering {

class DepthPrepassSystem : public RenderingSystemBase {
  public:
	DepthPrepassSystem() = default;
	~DepthPrepassSystem() override = default;

	void on_init(GFX::GfxContext &ctx) override;
	void add_passes(Graphics::RG::RenderGraph &graph, FrameContext &ctx) override;

  private:
	Ref<Graphics::Shader::ReloadablePipeline> m_pipeline;
};

} // namespace Aquila::Rendering
