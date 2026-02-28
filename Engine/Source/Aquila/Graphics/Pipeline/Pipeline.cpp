#include "Aquila/Graphics/Pipeline/Pipeline.h"
#include "Aquila/Graphics/Resources/Vertex.h"

namespace Aquila::Graphics::RenderingPipeline {

// Constructors

Pipeline::Pipeline(Device &device, const Shader::ShaderProgram &shader, const PipelineConfigInfo &configInfo)
	: m_Device(device) {
	CreatePipelineCache();
	CreateFromShaderProgram(shader, configInfo);

	m_Device.SetObjectDebugName(VK_OBJECT_TYPE_PIPELINE, reinterpret_cast<uint64>(m_GraphicsPipeline),
								shader.m_Name.c_str());
}

Pipeline::Pipeline(Device &device, const Ref<Material::Material> &material, const PipelineConfigInfo &configInfo)
	: m_Device(device) {
	CreatePipelineCache();
	CreateFromMaterial(material, configInfo);

	m_Device.SetObjectDebugName(VK_OBJECT_TYPE_PIPELINE, reinterpret_cast<uint64>(m_GraphicsPipeline),
								material->name.c_str());
}

Pipeline::Pipeline(Device &device, const std::string &vertSpvPath, const std::string &fragSpvPath,
				   const PipelineConfigInfo &configInfo)
	: m_Device(device) {
	CreatePipelineCache();
	CreateFromSpvPair(vertSpvPath, fragSpvPath, configInfo);

	// Use the vertex shader filename as the debug label
	std::string debugName = vertSpvPath.substr(vertSpvPath.find_last_of("/\\") + 1);
	m_Device.SetObjectDebugName(VK_OBJECT_TYPE_PIPELINE, reinterpret_cast<uint64>(m_GraphicsPipeline),
								debugName.c_str());
}

Pipeline::Pipeline(Device &device, const std::string &slangFilePath, const PipelineConfigInfo &configInfo)
	: m_Device(device) {
	CreatePipelineCache();
	CreateFromSlang(slangFilePath, configInfo);

	std::string debugName = slangFilePath.substr(slangFilePath.find_last_of("/\\") + 1);
	m_Device.SetObjectDebugName(VK_OBJECT_TYPE_PIPELINE, reinterpret_cast<uint64>(m_GraphicsPipeline),
								debugName.c_str());
}

// Destructor

Pipeline::~Pipeline() {
	if (m_Device.GetDevice() != nullptr) {
		if (m_VertexShaderModule != VK_NULL_HANDLE) {
			m_Device.GetDeletionManager().QueueDeletion(m_VertexShaderModule);
			m_VertexShaderModule = VK_NULL_HANDLE;
		}
		if (m_FragmentShaderModule != VK_NULL_HANDLE) {
			m_Device.GetDeletionManager().QueueDeletion(m_FragmentShaderModule);
			m_FragmentShaderModule = VK_NULL_HANDLE;
		}
		if (m_GraphicsPipeline != VK_NULL_HANDLE) {
			m_Device.GetDeletionManager().QueueDeletion(m_GraphicsPipeline);
			m_GraphicsPipeline = VK_NULL_HANDLE;
		}
		if (m_PipelineCache != VK_NULL_HANDLE) {
			m_Device.GetDeletionManager().QueueDeletion(m_PipelineCache);
			m_PipelineCache = VK_NULL_HANDLE;
		}
	} else {
		// Fallback: immediate deletion (shutdown path only, should never be called mid-frame)
		if (m_VertexShaderModule != VK_NULL_HANDLE) {
			vkDestroyShaderModule(m_Device.GetDevice(), m_VertexShaderModule, nullptr);
			m_VertexShaderModule = VK_NULL_HANDLE;
		}
		if (m_FragmentShaderModule != VK_NULL_HANDLE) {
			vkDestroyShaderModule(m_Device.GetDevice(), m_FragmentShaderModule, nullptr);
			m_FragmentShaderModule = VK_NULL_HANDLE;
		}
		if (m_GraphicsPipeline != VK_NULL_HANDLE) {
			// Note(A): vkDeviceWaitIdle is only safe here because this is the shutdown path.
			m_Device.Wait();
			vkDestroyPipeline(m_Device.GetDevice(), m_GraphicsPipeline, nullptr);
			m_GraphicsPipeline = VK_NULL_HANDLE;
		}
		if (m_PipelineCache != VK_NULL_HANDLE) {
			vkDestroyPipelineCache(m_Device.GetDevice(), m_PipelineCache, nullptr);
			m_PipelineCache = VK_NULL_HANDLE;
		}
	}
}

// Public interface

VkPipeline Pipeline::GetPipeline() const {
	return m_GraphicsPipeline;
}

void Pipeline::Bind(VkCommandBuffer commandBuffer) const {
	vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_GraphicsPipeline);
}

// Core shared creation — every constructor ends up here

void Pipeline::CreatePipelineFromStages(const std::vector<VkPipelineShaderStageCreateInfo> &stages,
										const PipelineConfigInfo &configInfo) {
	AQUILA_ASSERT(configInfo.pipelineLayout != VK_NULL_HANDLE, "Pipeline layout must be set in PipelineConfigInfo");
	AQUILA_ASSERT(!stages.empty(), "At least one shader stage is required");

	// --- Vertex input ---
	auto bindingDescriptions = configInfo.bindingDescriptions;
	auto attributeDescriptions = configInfo.attributeDescriptions;

	VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
	vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	vertexInputInfo.vertexBindingDescriptionCount = static_cast<uint32_t>(bindingDescriptions.size());
	vertexInputInfo.pVertexBindingDescriptions = bindingDescriptions.data();
	vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
	vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();

	// --- Color blend — honour the attachment list from configInfo ---
	uint32_t colorAttachmentCount = configInfo.colorFormats.empty()
										? static_cast<uint32_t>(configInfo.colorBlendAttachments.size())
										: static_cast<uint32_t>(configInfo.colorFormats.size());

	// For depth-only passes colorBlendAttachments may be empty; that is valid.
	VkPipelineColorBlendStateCreateInfo colorBlending{};
	colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	colorBlending.logicOpEnable = VK_FALSE;
	colorBlending.logicOp = VK_LOGIC_OP_COPY;
	colorBlending.attachmentCount = colorAttachmentCount;
	colorBlending.pAttachments =
		configInfo.colorBlendAttachments.empty() ? nullptr : configInfo.colorBlendAttachments.data();

	VkGraphicsPipelineCreateInfo pipelineInfo{};
	pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	pipelineInfo.pNext = configInfo.pNext; // VkPipelineRenderingCreateInfo for dynamic rendering
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
	pipelineInfo.renderPass = configInfo.renderPass; // VK_NULL_HANDLE for dynamic rendering
	pipelineInfo.subpass = 0;
	pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
	pipelineInfo.basePipelineIndex = -1;

	AQUILA_VULKAN_CHECK(vkCreateGraphicsPipelines(m_Device.GetDevice(), m_PipelineCache, 1, &pipelineInfo, nullptr,
												  &m_GraphicsPipeline));
}

// Per-constructor helpers

void Pipeline::CreateFromShaderProgram(const Shader::ShaderProgram &shader, const PipelineConfigInfo &configInfo) {
	if (!shader.IsValid()) {
		AQUILA_LOG_ERROR("Pipeline: ShaderProgram '{}' is invalid (no stages)", shader.m_Name);
		return;
	}

	// Modules are already compiled and owned by ShaderProgram — just reference them.
	std::vector<VkPipelineShaderStageCreateInfo> stages;
	stages.reserve(shader.m_Stages.size());
	for (const auto &s : shader.m_Stages) {
		VkPipelineShaderStageCreateInfo info{};
		info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		info.stage = s.stage;
		info.module = s.module;
		info.pName = "main";
		stages.push_back(info);
	}

	CreatePipelineFromStages(stages, configInfo);
}

void Pipeline::CreateFromSpvPair(const std::string &vertPath, const std::string &fragPath,
								 const PipelineConfigInfo &configInfo) {
	m_VertexShaderModule = LoadSpvModule(vertPath, vertPath);
	m_FragmentShaderModule = LoadSpvModule(fragPath, fragPath);

	VkPipelineShaderStageCreateInfo vertStage{};
	vertStage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	vertStage.stage = VK_SHADER_STAGE_VERTEX_BIT;
	vertStage.module = m_VertexShaderModule;
	vertStage.pName = "main";

	VkPipelineShaderStageCreateInfo fragStage{};
	fragStage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	fragStage.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	fragStage.module = m_FragmentShaderModule;
	fragStage.pName = "main";

	CreatePipelineFromStages({ vertStage, fragStage }, configInfo);
}

void Pipeline::CreateFromSlang(const std::string &slangPath, const PipelineConfigInfo &configInfo) {
	// Use the project-wide ShaderCompiler to compile all [shader("...")] entry points
	// found in the file to SPIR-V in one shot.
	std::string errorLog;
	std::vector<Shader::CompiledStage> compiled;

	if (!Shader::ShaderCompiler::CompileFile(slangPath, compiled, errorLog)) {
		AQUILA_LOG_ERROR("Pipeline: Slang compilation failed for '{}': {}", slangPath, errorLog);
		return;
	}

	// Create a temporary VkShaderModule per compiled stage.
	// We only keep the vertex and fragment modules alive as members so the destructor
	// can clean them up.  Extra stages (geometry, etc.) are destroyed here after the
	// pipeline is created — which is safe because Vulkan copies the SPIR-V internally.
	struct TempModule {
		VkShaderStageFlagBits stage;
		VkShaderModule module;
		std::string entryPoint;
	};
	std::vector<TempModule> tempModules;
	tempModules.reserve(compiled.size());

	for (auto &cs : compiled) {
		VkShaderModule mod =
			Shader::Shader::CreateShaderModule(cs.spirv, m_Device, slangPath + "::" + cs.entryPointName);
		tempModules.push_back({ cs.stage, mod, cs.entryPointName });
	}

	// Build stage create-infos
	std::vector<VkPipelineShaderStageCreateInfo> stages;
	stages.reserve(tempModules.size());
	for (const auto &tm : tempModules) {
		VkPipelineShaderStageCreateInfo info{};
		info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		info.stage = tm.stage;
		info.module = tm.module;
		info.pName = "main";
		stages.push_back(info);
	}

	CreatePipelineFromStages(stages, configInfo);

	// Transfer ownership of vertex/fragment modules to the Pipeline members so
	// the destructor handles them.  Any other stage modules are destroyed now.
	for (auto &tm : tempModules) {
		if (tm.stage == VK_SHADER_STAGE_VERTEX_BIT) {
			m_VertexShaderModule = tm.module;
		} else if (tm.stage == VK_SHADER_STAGE_FRAGMENT_BIT) {
			m_FragmentShaderModule = tm.module;
		} else {
			// Geometry, tessellation, etc. — safe to release immediately
			vkDestroyShaderModule(m_Device.GetDevice(), tm.module, nullptr);
		}
	}

	AQUILA_LOG_INFO("Pipeline: created from Slang '{}' ({} stage(s))", slangPath, compiled.size());
}

void Pipeline::CreateFromMaterial(const Ref<Material::Material> &material, const PipelineConfigInfo &configInfo) {
	const auto &shader = material->GetTemplate()->shader;
	if (!shader || !shader->IsValid()) {
		AQUILA_LOG_ERROR("Pipeline: material '{}' has an invalid shader", material->name);
		return;
	}

	// Destroy the old pipeline that the material may be holding
	if (material->GetPipeline() != VK_NULL_HANDLE) {
		vkDestroyPipeline(m_Device.GetDevice(), material->GetPipeline(), nullptr);
	}

	// --- Shader stages (owned by ShaderProgram, not by us) ---
	std::vector<VkPipelineShaderStageCreateInfo> shaderStages;
	shaderStages.reserve(shader->m_Stages.size());
	for (const auto &s : shader->m_Stages) {
		VkPipelineShaderStageCreateInfo info{};
		info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		info.stage = s.stage;
		info.module = s.module;
		info.pName = "main";
		shaderStages.push_back(info);
	}

	// --- Override rasterization from material render state ---
	const auto &renderState = material->GetRenderState();

	// Take a mutable copy of the config so we can patch it per-material without
	// touching the caller's struct.
	//
	// NOTE: PipelineConfigInfo is non-copyable, so we patch the Vulkan structs
	// directly by building the pipeline create-info manually here instead of
	// calling CreatePipelineFromStages (which uses configInfo's rasterization).

	VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
	vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	auto bindingDesc = configInfo.bindingDescriptions;
	auto attrDesc = configInfo.attributeDescriptions;
	vertexInputInfo.vertexBindingDescriptionCount = static_cast<uint32_t>(bindingDesc.size());
	vertexInputInfo.pVertexBindingDescriptions = bindingDesc.data();
	vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attrDesc.size());
	vertexInputInfo.pVertexAttributeDescriptions = attrDesc.data();

	VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
	inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	inputAssembly.primitiveRestartEnable = VK_FALSE;

	VkPipelineViewportStateCreateInfo viewportState{};
	viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	viewportState.viewportCount = 1;
	viewportState.scissorCount = 1;

	VkPipelineRasterizationStateCreateInfo rasterizer{};
	rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	rasterizer.depthClampEnable = VK_FALSE;
	rasterizer.rasterizerDiscardEnable = VK_FALSE;
	rasterizer.polygonMode = renderState.m_Wireframe ? VK_POLYGON_MODE_LINE : VK_POLYGON_MODE_FILL;
	rasterizer.lineWidth = renderState.m_LineWidth;
	rasterizer.cullMode = GetVkCullMode(renderState.m_CullMode);
	rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
	rasterizer.depthBiasEnable = VK_FALSE;

	VkPipelineMultisampleStateCreateInfo multisampling{};
	multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	multisampling.sampleShadingEnable = VK_FALSE;
	multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

	VkPipelineDepthStencilStateCreateInfo depthStencil{};
	depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
	depthStencil.depthTestEnable = renderState.m_DepthTest ? VK_TRUE : VK_FALSE;
	depthStencil.depthWriteEnable = renderState.m_DepthWrite ? VK_TRUE : VK_FALSE;
	depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;
	depthStencil.depthBoundsTestEnable = VK_FALSE;
	depthStencil.stencilTestEnable = VK_FALSE;

	// --- Blend state from material blend mode ---
	VkPipelineColorBlendAttachmentState blendAttachment{};
	blendAttachment.colorWriteMask =
		VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;

	switch (renderState.m_BlendMode) {
	case Material::BlendMode::Opaque:
		blendAttachment.blendEnable = VK_FALSE;
		break;
	case Material::BlendMode::AlphaBlend:
		blendAttachment.blendEnable = VK_TRUE;
		blendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
		blendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
		blendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
		blendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
		blendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
		blendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;
		break;
	case Material::BlendMode::Additive:
		blendAttachment.blendEnable = VK_TRUE;
		blendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
		blendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE;
		blendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
		blendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
		blendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
		blendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;
		break;
	case Material::BlendMode::Multiply:
		blendAttachment.blendEnable = VK_TRUE;
		blendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_DST_COLOR;
		blendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
		blendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
		blendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
		blendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
		blendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;
		break;
	}

	uint32_t colorAttachmentCount = configInfo.colorFormats.empty()
										? static_cast<uint32_t>(configInfo.colorBlendAttachments.size())
										: static_cast<uint32_t>(configInfo.colorFormats.size());

	std::vector<VkPipelineColorBlendAttachmentState> blendAttachments(colorAttachmentCount, blendAttachment);

	VkPipelineColorBlendStateCreateInfo colorBlending{};
	colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	colorBlending.logicOpEnable = VK_FALSE;
	colorBlending.attachmentCount = colorAttachmentCount;
	colorBlending.pAttachments = blendAttachments.data();

	std::vector<VkDynamicState> dynamicStates = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
	VkPipelineDynamicStateCreateInfo dynamicState{};
	dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
	dynamicState.pDynamicStates = dynamicStates.data();

	VkGraphicsPipelineCreateInfo pipelineInfo{};
	pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	pipelineInfo.pNext = configInfo.pNext;
	pipelineInfo.stageCount = static_cast<uint32_t>(shaderStages.size());
	pipelineInfo.pStages = shaderStages.data();
	pipelineInfo.pVertexInputState = &vertexInputInfo;
	pipelineInfo.pInputAssemblyState = &inputAssembly;
	pipelineInfo.pViewportState = &viewportState;
	pipelineInfo.pRasterizationState = &rasterizer;
	pipelineInfo.pMultisampleState = &multisampling;
	pipelineInfo.pDepthStencilState = &depthStencil;
	pipelineInfo.pColorBlendState = &colorBlending;
	pipelineInfo.pDynamicState = &dynamicState;
	pipelineInfo.layout = shader->m_Layout;
	pipelineInfo.renderPass = configInfo.renderPass; // VK_NULL_HANDLE for dynamic rendering
	pipelineInfo.subpass = 0;
	pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
	pipelineInfo.basePipelineIndex = -1;

	AQUILA_VULKAN_CHECK(vkCreateGraphicsPipelines(m_Device.GetDevice(), m_PipelineCache, 1, &pipelineInfo, nullptr,
												  &m_GraphicsPipeline));

	AQUILA_LOG_INFO("Pipeline: created for material '{}'", material->name);
}

// Utility helpers

VkShaderModule Pipeline::LoadSpvModule(const std::string &spvPath, const std::string &debugName) {
	auto code = Shader::Shader::ReadFile(spvPath);
	return Shader::Shader::CreateShaderModule(code, m_Device, debugName);
}

void Pipeline::CreatePipelineCache() {
	VkPipelineCacheCreateInfo cacheInfo{};
	cacheInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;
	AQUILA_VULKAN_CHECK(vkCreatePipelineCache(m_Device.GetDevice(), &cacheInfo, nullptr, &m_PipelineCache));
}

VkCullModeFlags Pipeline::GetVkCullMode(Material::CullMode mode) {
	switch (mode) {
	case Material::CullMode::None:
		return VK_CULL_MODE_NONE;
	case Material::CullMode::Front:
		return VK_CULL_MODE_FRONT_BIT;
	case Material::CullMode::Back:
		return VK_CULL_MODE_BACK_BIT;
	default:
		return VK_CULL_MODE_BACK_BIT;
	}
}

// DefaultPipelineConfig

void Pipeline::DefaultPipelineConfig(PipelineConfigInfo &configInfo) {
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
	configInfo.rasterizationInfo.depthBiasEnable = VK_FALSE; // disabled by default; shadow pass enables it explicitly
	configInfo.rasterizationInfo.depthBiasConstantFactor = 0.0F;
	configInfo.rasterizationInfo.depthBiasClamp = 0.0F;
	configInfo.rasterizationInfo.depthBiasSlopeFactor = 0.0F;

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
	configInfo.colorBlendInfo.blendConstants[0] = 0.0F;
	configInfo.colorBlendInfo.blendConstants[1] = 0.0F;
	configInfo.colorBlendInfo.blendConstants[2] = 0.0F;
	configInfo.colorBlendInfo.blendConstants[3] = 0.0F;

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

	configInfo.bindingDescriptions = Resources::Vertex::GetBindingDescriptions();
	configInfo.attributeDescriptions = Resources::Vertex::GetAttributeDescriptions();
}

} // namespace Aquila::Graphics::RenderingPipeline
