#ifndef SKYBOX_RENDER_SYSTEM_H
#define SKYBOX_RENDER_SYSTEM_H

#include "Aquila/Graphics/Pipeline/DynamicRenderingHelper.h"
#include "Aquila/Rendering/Systems/RenderingSystemBase.h"
#include "Aquila/Graphics/Resources/Texture2D.h"

namespace Aquila::Rendering::Systems {

struct SkyboxPushConstants {
	f32 intensity = 1.0f;
	f32 lod = 0;
};

class SkyboxRenderSystem : public RenderingSystemBase {
  public:
	SkyboxRenderSystem(Device &device, const std::vector<Ref<DescriptorSetLayout>> &layouts);

	void SetSkyboxTexture(const Ref<Graphics::Resources::Texture2D> &hdrTexture);

	void OnUpdate(const FrameSpec &frameSpec) override;
	void OnRender(const FrameSpec &frameSpec) override;
	void OnEvent(Events::Event &event) override;

  private:
	void CreatePipelineLayout() override;
	void CreatePipeline(const PipelineRenderingFormats &renderingFormats) override;
	void CreateCubeGeometry();

	Unique<Buffer> m_CubeVertexBuffer;
	Unique<Buffer> m_CubeIndexBuffer;
	uint32 m_IndexCount = 0;
	Ref<Graphics::Resources::Texture2D> m_SkyboxTexture;
};

} // namespace Aquila::Rendering::Systems

#endif
