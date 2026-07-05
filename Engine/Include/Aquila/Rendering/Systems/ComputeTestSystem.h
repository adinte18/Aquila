#pragma once
#include "Aquila/Rendering/Systems/RenderingSystemBase.h"
#include "Aquila/GFX/GfxBuffer.h"
#include "Aquila/GFX/GfxPipeline.h"
#include "Aquila/GFX/GfxDescriptorSet.h"

namespace Aquila::Rendering {

class ClusterComputeSystem : public RenderingSystemBase {
	struct GridData {
		Ivec3 grid;
	};

	struct AABB {
		Vec3 min;
		Vec3 max;
	};

  public:
	ClusterComputeSystem() = default;
	~ClusterComputeSystem() override = default;

	void on_init(GFX::GfxContext &ctx) override;
	void add_passes(Graphics::RG::RenderGraph &graph, FrameContext &ctx) override;

  private:
	Ref<GFX::GfxPipeline> m_pipeline;
	Ref<GFX::GfxDescriptorSetLayout> m_storage_layout;
	Ref<GFX::GfxBuffer> m_output_buffer;
	Ref<GFX::GfxBuffer> m_grid_buffer;
	Ref<GFX::GfxDescriptorSet> m_storage_set;
	bool m_verified = false;
	GridData m_grid_data{};
};

} // namespace Aquila::Rendering
