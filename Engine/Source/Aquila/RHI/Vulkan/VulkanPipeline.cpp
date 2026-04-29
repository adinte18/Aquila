#include "Aquila/RHI/Vulkan/VulkanPipeline.h"
#include "Aquila/RHI/Vulkan/VulkanDeletionQueue.h"

#include "Aquila/RHI/Vulkan/VulkanCommandList.h"
#include "Aquila/RHI/Vulkan/VulkanDevice.h"

namespace Aquila::RHI {

VulkanPipeline::VulkanPipeline(VulkanDevice &device, const std::vector<VkPipelineShaderStageCreateInfo> &stages,
							   const VulkanPipelineConfig &configInfo)
	: m_Device(device), m_Layout(configInfo.pipelineLayout) {
	CreatePipelineCache();
	CreatePipelineFromStages(stages, configInfo);
}

VulkanPipeline::~VulkanPipeline() {
	auto &dq = m_Device.GetDeletionQueue();
	if (m_GraphicsPipeline != VK_NULL_HANDLE) {
		dq.QueueDeletion(m_GraphicsPipeline);
		m_GraphicsPipeline = VK_NULL_HANDLE;
	}
	if (m_Layout != VK_NULL_HANDLE) {
		dq.QueueDeletion(m_Layout);
		m_Layout = VK_NULL_HANDLE;
	}
	if (m_PipelineCache != VK_NULL_HANDLE) {
		dq.QueueDeletion(m_PipelineCache);
		m_PipelineCache = VK_NULL_HANDLE;
	}
}

void VulkanPipeline::Bind(IRHICommandList &cmd) {
	vkCmdBindPipeline(static_cast<VulkanCommandList &>(cmd).GetHandle(), VK_PIPELINE_BIND_POINT_GRAPHICS,
					  m_GraphicsPipeline);
}

void VulkanPipeline::Bind(VulkanCommandList &cmd) const {
	vkCmdBindPipeline(cmd.GetHandle(), VK_PIPELINE_BIND_POINT_GRAPHICS, m_GraphicsPipeline);
}

void VulkanPipeline::CreatePipelineFromStages(const std::vector<VkPipelineShaderStageCreateInfo> &stages,
											  const VulkanPipelineConfig &configInfo) {
	AQUILA_ASSERT(configInfo.pipelineLayout != VK_NULL_HANDLE, "Pipeline layout must be set in VulkanPipelineConfig");
	AQUILA_ASSERT(!stages.empty(), "At least one shader stage is required");

	VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
	vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	vertexInputInfo.vertexBindingDescriptionCount = static_cast<uint32_t>(configInfo.bindingDescriptions.size());
	vertexInputInfo.pVertexBindingDescriptions = configInfo.bindingDescriptions.data();
	vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(configInfo.attributeDescriptions.size());
	vertexInputInfo.pVertexAttributeDescriptions = configInfo.attributeDescriptions.data();

	uint32_t colorAttachmentCount = configInfo.colorFormats.empty()
										? static_cast<uint32_t>(configInfo.colorBlendAttachments.size())
										: static_cast<uint32_t>(configInfo.colorFormats.size());

	VkPipelineColorBlendStateCreateInfo colorBlending{};
	colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	colorBlending.logicOpEnable = VK_FALSE;
	colorBlending.logicOp = VK_LOGIC_OP_COPY;
	colorBlending.attachmentCount = colorAttachmentCount;
	colorBlending.pAttachments =
		configInfo.colorBlendAttachments.empty() ? nullptr : configInfo.colorBlendAttachments.data();

	VkPipelineRenderingCreateInfo renderingCreateInfo{};
	renderingCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO;
	renderingCreateInfo.pNext = configInfo.pNext;
	renderingCreateInfo.colorAttachmentCount = static_cast<uint32_t>(configInfo.colorFormats.size());
	renderingCreateInfo.pColorAttachmentFormats =
		configInfo.colorFormats.empty() ? nullptr : configInfo.colorFormats.data();
	renderingCreateInfo.depthAttachmentFormat = configInfo.depthFormat;
	renderingCreateInfo.stencilAttachmentFormat = configInfo.stencilFormat;

	bool useDynamicRendering = !configInfo.colorFormats.empty() || configInfo.depthFormat != VK_FORMAT_UNDEFINED;

	VkGraphicsPipelineCreateInfo pipelineInfo{};
	pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	pipelineInfo.pNext = useDynamicRendering ? &renderingCreateInfo : configInfo.pNext;
	pipelineInfo.stageCount = static_cast<uint32_t>(stages.size());
	pipelineInfo.pStages = stages.data();
	pipelineInfo.pVertexInputState = &vertexInputInfo;
	pipelineInfo.pInputAssemblyState = &configInfo.inputAssemblyInfo;
	pipelineInfo.pViewportState = &configInfo.viewportInfo;
	pipelineInfo.pRasterizationState = &configInfo.rasterizationInfo;
	pipelineInfo.pMultisampleState = &configInfo.multisampleInfo;
	pipelineInfo.pDepthStencilState = &configInfo.depthStencilInfo;
	pipelineInfo.pColorBlendState = &colorBlending;
	pipelineInfo.pDynamicState = &configInfo.dynamicStateCreateInfo;
	pipelineInfo.layout = configInfo.pipelineLayout;
	pipelineInfo.renderPass = configInfo.renderPass;
	pipelineInfo.subpass = 0;
	pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
	pipelineInfo.basePipelineIndex = -1;

	AQUILA_VULKAN_CHECK(vkCreateGraphicsPipelines(m_Device.GetDevice(), m_PipelineCache, 1, &pipelineInfo, nullptr,
												  &m_GraphicsPipeline));
}

void VulkanPipeline::CreatePipelineCache() {
	VkPipelineCacheCreateInfo cacheInfo{};
	cacheInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;
	AQUILA_VULKAN_CHECK(vkCreatePipelineCache(m_Device.GetDevice(), &cacheInfo, nullptr, &m_PipelineCache));
}

void VulkanPipeline::DefaultPipelineConfig(VulkanPipelineConfig &configInfo) {
	configInfo.inputAssemblyInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	configInfo.inputAssemblyInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	configInfo.inputAssemblyInfo.primitiveRestartEnable = VK_FALSE;

	configInfo.viewportInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	configInfo.viewportInfo.viewportCount = 1;
	configInfo.viewportInfo.pViewports = nullptr;
	configInfo.viewportInfo.scissorCount = 1;
	configInfo.viewportInfo.pScissors = nullptr;

	configInfo.rasterizationInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	configInfo.rasterizationInfo.depthClampEnable = VK_FALSE;
	configInfo.rasterizationInfo.rasterizerDiscardEnable = VK_FALSE;
	configInfo.rasterizationInfo.polygonMode = VK_POLYGON_MODE_FILL;
	configInfo.rasterizationInfo.lineWidth = 1.0F;
	configInfo.rasterizationInfo.cullMode = VK_CULL_MODE_NONE;
	configInfo.rasterizationInfo.frontFace = VK_FRONT_FACE_CLOCKWISE;
	configInfo.rasterizationInfo.depthBiasEnable = VK_FALSE;

	configInfo.multisampleInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	configInfo.multisampleInfo.sampleShadingEnable = VK_FALSE;
	configInfo.multisampleInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
	configInfo.multisampleInfo.minSampleShading = 1.0F;
	configInfo.multisampleInfo.pSampleMask = nullptr;
	configInfo.multisampleInfo.alphaToCoverageEnable = VK_FALSE;
	configInfo.multisampleInfo.alphaToOneEnable = VK_FALSE;

	VkPipelineColorBlendAttachmentState defaultAttachment{};
	defaultAttachment.blendEnable = VK_TRUE;
	defaultAttachment.colorWriteMask =
		VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	defaultAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
	defaultAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
	defaultAttachment.colorBlendOp = VK_BLEND_OP_ADD;
	defaultAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
	defaultAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
	defaultAttachment.alphaBlendOp = VK_BLEND_OP_ADD;

	configInfo.colorBlendAttachments.clear();
	configInfo.colorBlendAttachments.push_back(defaultAttachment);

	configInfo.colorBlendInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	configInfo.colorBlendInfo.logicOpEnable = VK_FALSE;
	configInfo.colorBlendInfo.logicOp = VK_LOGIC_OP_COPY;
	configInfo.colorBlendInfo.attachmentCount = static_cast<uint32_t>(configInfo.colorBlendAttachments.size());
	configInfo.colorBlendInfo.pAttachments = configInfo.colorBlendAttachments.data();

	configInfo.depthStencilInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
	configInfo.depthStencilInfo.depthTestEnable = VK_TRUE;
	configInfo.depthStencilInfo.depthWriteEnable = VK_TRUE;
	configInfo.depthStencilInfo.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
	configInfo.depthStencilInfo.depthBoundsTestEnable = VK_FALSE;
	configInfo.depthStencilInfo.minDepthBounds = 0.0F;
	configInfo.depthStencilInfo.maxDepthBounds = 1.0F;
	configInfo.depthStencilInfo.stencilTestEnable = VK_FALSE;
	configInfo.depthStencilInfo.front = {};
	configInfo.depthStencilInfo.back = {};

	configInfo.dynamicStateEnables = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR,
									   VK_DYNAMIC_STATE_LINE_WIDTH };
	configInfo.dynamicStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	configInfo.dynamicStateCreateInfo.pDynamicStates = configInfo.dynamicStateEnables.data();
	configInfo.dynamicStateCreateInfo.dynamicStateCount = static_cast<uint32_t>(configInfo.dynamicStateEnables.size());
	configInfo.dynamicStateCreateInfo.flags = 0;

	if (configInfo.customVertexLayout.has_value()) {
		auto &layout = configInfo.customVertexLayout.value();
		configInfo.bindingDescriptions = { { 0, layout.stride, VK_VERTEX_INPUT_RATE_VERTEX } };
		configInfo.attributeDescriptions.clear();
		for (auto &a : layout.attributes) {
			configInfo.attributeDescriptions.push_back({ a.location, a.binding, ToVkFormat(a.format), a.offset });
		}
	} else {
		configInfo.bindingDescriptions = Vertex::GetBindingDescriptions();
		configInfo.attributeDescriptions = Vertex::GetAttributeDescriptions();
	}
}

} // namespace Aquila::RHI
