#ifndef CompositeRenderingSystem_H
#define CompositeRenderingSystem_H
#include "Aquila/Graphics/Pipeline/DynamicRenderingHelper.h"
#include "RenderingSystemBase.h"

namespace Aquila::Rendering::Systems {

using namespace Aquila::Graphics;
using namespace Aquila::Graphics::RenderingPipeline;
using namespace Aquila::Graphics::Resources;

class CompositeRenderingSystem final : public RenderingSystemBase {
  public:
	CompositeRenderingSystem(Device &device, const std::vector<Ref<DescriptorSetLayout>> &layouts);

	~CompositeRenderingSystem() override {
		if (m_Pipeline) {
			m_Pipeline.reset();
		}

		if (m_PipelineLayout != VK_NULL_HANDLE) {
			vkDestroyPipelineLayout(device.GetDevice(), m_PipelineLayout, nullptr);
			m_PipelineLayout = VK_NULL_HANDLE;
		}
	}
	void OnUpdate(const FrameSpec &frameSpec) override;
	void OnRender(const FrameSpec &frameSpec) override;
	void OnEvent(Events::Event &event) override;

  private:
	void CreatePipeline(const PipelineRenderingFormats &renderingFormats) override;
	void CreatePipelineLayout() override;
	void CreateFullscreenQuad();

	Unique<Buffer> m_QuadVertexBuffer;
	Unique<Buffer> m_QuadIndexBuffer;
	Unique<Pipeline> m_Pipeline;
	VkPipelineLayout m_PipelineLayout = VK_NULL_HANDLE;
	Ref<DescriptorSetLayout> m_GBufferLayout;
	std::vector<VkDescriptorSet> m_GBufferDescriptorSets;
};
} // namespace Aquila::Rendering::Systems

#endif // CompositeRenderingSystem_H
