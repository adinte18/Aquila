#pragma once
#include "Aquila/Rendering/Systems/RenderingSystemBase.h"
#include "Aquila/GFX/GfxBuffer.h"
#include "Aquila/GFX/GfxPipeline.h"
#include "Aquila/GFX/GfxDescriptorSet.h"

namespace Aquila::Rendering {

class LightCullingSystem : public RenderingSystemBase {
  public:
	LightCullingSystem() = default;
	~LightCullingSystem() override = default;

	void OnInit(GFX::GfxContext &ctx) override;
	void AddPasses(Graphics::RG::RenderGraph &graph, FrameContext &ctx) override;

  private:
	Ref<GFX::GfxPipeline> m_Pipeline;
	Ref<GFX::GfxDescriptorSetLayout> m_StorageLayout;
	Ref<GFX::GfxBuffer> m_GlobalIndexCounter; // atomic counter reset each frame before dispatch
	Ref<GFX::GfxDescriptorSet> m_StorageSet;
	bool m_AABBBufferBound = false;
};

} // namespace Aquila::Rendering
