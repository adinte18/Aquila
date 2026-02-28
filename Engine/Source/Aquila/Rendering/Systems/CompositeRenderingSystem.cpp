#include "Aquila/Rendering/Systems/CompositeRenderingSystem.h"
#include "Aquila/Graphics/Pipeline/DynamicRenderingHelper.h"

namespace Aquila::Rendering::Systems {

using namespace Aquila::Graphics::Helpers;

CompositeRenderingSystem::CompositeRenderingSystem(Device &device, const std::vector<Ref<DescriptorSetLayout>> &layouts)
	: RenderingSystemBase(device) {
	m_Layouts = layouts;

	CreatePipelineLayout();

	// Create pipeline for final composite pass
	auto compositeFormats = PipelineRenderingFormats::SingleColor(VK_FORMAT_R8G8B8A8_UNORM);
	CreatePipeline(compositeFormats);

	// CreateFullscreenQuad();
}

void CompositeRenderingSystem::CreatePipelineLayout() {
	const std::vector<VkDescriptorSetLayout> layouts = {
		m_Layouts[0]->GetDescriptorSetLayout(),
	};

	VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
	pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutInfo.setLayoutCount = static_cast<uint32_t>(layouts.size());
	pipelineLayoutInfo.pSetLayouts = layouts.data();
	pipelineLayoutInfo.pushConstantRangeCount = 0;
	pipelineLayoutInfo.pPushConstantRanges = nullptr;

	AQUILA_VULKAN_CHECK(vkCreatePipelineLayout(device.GetDevice(), &pipelineLayoutInfo, nullptr, &m_PipelineLayout));
}

void CompositeRenderingSystem::CreatePipeline(const PipelineRenderingFormats &renderingFormats) {
	PipelineConfigInfo config{};
	Pipeline::DefaultPipelineConfig(config);

	config.pipelineLayout = m_PipelineLayout;
	config.depthStencilInfo.depthTestEnable = VK_FALSE;
	config.colorBlendAttachments[0].blendEnable = VK_FALSE;
	config.rasterizationInfo.cullMode = VK_CULL_MODE_NONE;

	config.bindingDescriptions.clear();
	config.attributeDescriptions.clear();

	config.colorFormats = renderingFormats.colorFormats;
	config.depthFormat = renderingFormats.depthFormat;
	config.stencilFormat = VK_FORMAT_UNDEFINED;

	VkPipelineRenderingCreateInfo renderingCreateInfo{};
	renderingCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO;
	renderingCreateInfo.colorAttachmentCount = static_cast<uint32>(config.colorFormats.size());
	renderingCreateInfo.pColorAttachmentFormats = config.colorFormats.data();
	renderingCreateInfo.depthAttachmentFormat = config.depthFormat;
	renderingCreateInfo.stencilAttachmentFormat = config.stencilFormat;

	config.pNext = &renderingCreateInfo;

	m_Pipeline = CreateUnique<Pipeline>(device, std::string(IMMUTABLE_SHADERS_PATH) + "Composite.slang", config);
}

void CompositeRenderingSystem::CreateFullscreenQuad() {
	const std::vector<glm::vec2> verts = { { -1, -1 }, { 1, -1 }, { 1, 1 }, { -1, 1 } };
	const std::vector<uint32_t> indices = { 0, 1, 2, 2, 3, 0 };

	m_QuadVertexBuffer = CreateUnique<Buffer>(
		device, "Composite_QuadVB", sizeof(glm::vec2), verts.size(), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
	m_QuadVertexBuffer->Map();
	m_QuadVertexBuffer->Write(verts.data());

	m_QuadIndexBuffer = CreateUnique<Buffer>(
		device, "Composite_QuadIB", sizeof(uint32_t), indices.size(), VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
	m_QuadIndexBuffer->Map();
	m_QuadIndexBuffer->Write(indices.data());
}

void CompositeRenderingSystem::OnUpdate(const FrameSpec &frameSpec) {}

void CompositeRenderingSystem::OnEvent(Events::Event &event) {}

void CompositeRenderingSystem::OnRender(const FrameSpec &frameSpec) {
	if (frameSpec.scene == nullptr) {
		AQUILA_LOG_WARNING("No scene provided to CompositeRenderingSystem");
		return;
	}

	VkDescriptorSet sceneDescriptorSet = GetSceneDescriptorSet(frameSpec.scene, 0, frameSpec.frameIndex);

	if (sceneDescriptorSet == VK_NULL_HANDLE) {
		AQUILA_LOG_ERROR("No descriptor set for scene '{}' frame {}", frameSpec.scene->GetSceneName(),
						 frameSpec.frameIndex);
		return;
	}

	m_Pipeline->Bind(frameSpec.commandBuffer);

	vkCmdBindDescriptorSets(frameSpec.commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_PipelineLayout, 0, 1,
							&sceneDescriptorSet, 0, nullptr);

	vkCmdDraw(frameSpec.commandBuffer, 3, 1, 0, 0);
}

} // namespace Aquila::Rendering::Systems
