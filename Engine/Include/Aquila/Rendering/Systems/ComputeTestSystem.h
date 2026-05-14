#pragma once
#include "Aquila/Rendering/Systems/RenderingSystemBase.h"
#include "Aquila/GFX/GfxBuffer.h"
#include "Aquila/GFX/GfxPipeline.h"
#include "Aquila/GFX/GfxDescriptorSet.h"

namespace Aquila::Rendering {

class ClusterComputeSystem : public RenderingSystemBase {
	struct GridData {
		ivec3 grid;
	};

	struct AABB {
		vec3 min;
		vec3 max;
	};

  public:
	ClusterComputeSystem() = default;
	~ClusterComputeSystem() override = default;

	void OnInit(GFX::GfxContext &ctx) override;
	void AddPasses(Graphics::RG::RenderGraph &graph, FrameContext &ctx) override;

  private:
	Ref<GFX::GfxPipeline> m_Pipeline;
	Ref<GFX::GfxDescriptorSetLayout> m_StorageLayout;
	Ref<GFX::GfxBuffer> m_OutputBuffer;
	Ref<GFX::GfxBuffer> m_GridBuffer;
	Ref<GFX::GfxDescriptorSet> m_StorageSet;
	bool m_Verified = false;
	GridData m_GridData{};
};

} // namespace Aquila::Rendering
