#pragma once
#include "Aquila/Rendering/Systems/RenderingSystemBase.h"
#include "Aquila/Rendering/Environment/HosekWilkieSky.h"
#include "Aquila/Foundation/SharedConstants.h"
#include "Aquila/Graphics/Shader/ReloadablePipeline.h"
#include "Aquila/GFX/GfxBuffer.h"
#include "Aquila/GFX/GfxDescriptorSet.h"

namespace Aquila::Rendering {

class SkySystem : public RenderingSystemBase {
  public:
	SkySystem() = default;
	~SkySystem() override = default;

	void on_init(GFX::GfxContext &ctx) override;
	void add_passes(Graphics::RG::RenderGraph &graph, FrameContext &ctx) override;

  private:
	Ref<Graphics::Shader::ReloadablePipeline> m_pipeline;
	Ref<GFX::GfxDescriptorSetLayout> m_sky_layout;
	std::array<Ref<GFX::GfxBuffer>, SharedConstants::MAX_FRAMES_IN_FLIGHT> m_sky_buffers;
	std::array<Ref<GFX::GfxDescriptorSet>, SharedConstants::MAX_FRAMES_IN_FLIGHT> m_sky_sets;
	GpuSkyData m_cached{};
};

} // namespace Aquila::Rendering
