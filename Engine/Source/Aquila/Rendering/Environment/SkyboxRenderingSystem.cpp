#include "Aquila/Rendering/Environment/SkyboxRenderingSystem.h"
#include "Aquila/Core/Defines.h"
#include "Aquila/Graphics/Core/Swapchain.h"
#include "Aquila/Rendering/Camera.h"
#include "Aquila/Scene/Components/SkyLightComponent.h"
#include "Aquila/Scene/EntityManager.h"

namespace Aquila::Rendering::Systems {

SkyboxRenderSystem::SkyboxRenderSystem(Device &device, const std::vector<Ref<DescriptorSetLayout>> &layouts)
	: RenderingSystemBase(device) {
	m_Layouts = layouts; // Set 0: Camera, Set 1: Skybox Texture

	CreateCubeGeometry();
	CreatePipelineLayout();

	auto renderingFormats = PipelineRenderingFormats::SkyBox();

	CreatePipeline(renderingFormats);
}

void SkyboxRenderSystem::CreatePipelineLayout() {
	std::vector<VkDescriptorSetLayout> layouts = {
		m_Layouts[0]->GetDescriptorSetLayout(), // Set 0: Camera
		m_Layouts[1]->GetDescriptorSetLayout() // Set 1: Skybox texture
	};

	VkPushConstantRange pushConstantRange{};
	pushConstantRange.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
	pushConstantRange.offset = 0;
	pushConstantRange.size = sizeof(SkyboxPushConstants);

	VkPipelineLayoutCreateInfo layoutInfo{};
	layoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	layoutInfo.setLayoutCount = static_cast<uint32_t>(layouts.size());
	layoutInfo.pSetLayouts = layouts.data();
	layoutInfo.pushConstantRangeCount = 1;
	layoutInfo.pPushConstantRanges = &pushConstantRange;

	AQUILA_VULKAN_CHECK(vkCreatePipelineLayout(device.GetDevice(), &layoutInfo, nullptr, &m_PipelineLayout));
}

void SkyboxRenderSystem::CreatePipeline(const PipelineRenderingFormats &renderingFormats) {
	PipelineConfigInfo pipelineConfig{};
	Pipeline::DefaultPipelineConfig(pipelineConfig);

	pipelineConfig.colorFormats = renderingFormats.colorFormats;
	pipelineConfig.depthFormat = renderingFormats.depthFormat;
	pipelineConfig.stencilFormat = VK_FORMAT_UNDEFINED;

	VkPipelineRenderingCreateInfo renderingCreateInfo{};
	renderingCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO;
	renderingCreateInfo.colorAttachmentCount = static_cast<uint32>(pipelineConfig.colorFormats.size());
	renderingCreateInfo.pColorAttachmentFormats =
		pipelineConfig.colorFormats.empty() ? nullptr : pipelineConfig.colorFormats.data();
	renderingCreateInfo.depthAttachmentFormat = pipelineConfig.depthFormat;
	renderingCreateInfo.stencilAttachmentFormat = pipelineConfig.stencilFormat;

	pipelineConfig.pipelineLayout = m_PipelineLayout;
	pipelineConfig.pNext = &renderingCreateInfo;

	// Skybox-specific settings
	pipelineConfig.depthStencilInfo.depthTestEnable = VK_TRUE;
	pipelineConfig.depthStencilInfo.depthWriteEnable = VK_FALSE;
	pipelineConfig.depthStencilInfo.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
	pipelineConfig.rasterizationInfo.cullMode = VK_CULL_MODE_NONE;

	// Vertex input: only position (vec3)
	pipelineConfig.bindingDescriptions.clear();
	pipelineConfig.attributeDescriptions.clear();

	VkVertexInputBindingDescription bindingDesc{};
	bindingDesc.binding = 0;
	bindingDesc.stride = sizeof(vec3);
	bindingDesc.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

	VkVertexInputAttributeDescription attrDesc{};
	attrDesc.binding = 0;
	attrDesc.location = 0;
	attrDesc.format = VK_FORMAT_R32G32B32_SFLOAT;
	attrDesc.offset = 0;

	pipelineConfig.bindingDescriptions.push_back(bindingDesc);
	pipelineConfig.attributeDescriptions.push_back(attrDesc);

	m_Pipeline = CreateUnique<Pipeline>(device, IMMUTABLE_SHADERS_PATH "Skybox.slang", pipelineConfig);
}

void SkyboxRenderSystem::CreateCubeGeometry() {
	std::vector<vec3> vertices = { { -1.0F, -1.0F, 1.0F },
								   { 1.0F, -1.0F, 1.0F },
								   { 1.0F, 1.0F, 1.0F },
								   { -1.0F, 1.0F, 1.0F },

								   { -1.0F, -1.0F, -1.0F },
								   { -1.0F, 1.0F, -1.0F },
								   { 1.0F, 1.0F, -1.0F },
								   { 1.0F, -1.0F, -1.0F },

								   { -1.0F, 1.0F, -1.0F },
								   { -1.0F, 1.0F, 1.0F },
								   { 1.0F, 1.0F, 1.0F },
								   { 1.0F, 1.0F, -1.0F },

								   { -1.0F, -1.0F, -1.0F },
								   { 1.0F, -1.0F, -1.0F },
								   { 1.0F, -1.0F, 1.0F },
								   { -1.0F, -1.0F, 1.0F },

								   { 1.0F, -1.0F, -1.0F },
								   { 1.0F, 1.0F, -1.0F },
								   { 1.0F, 1.0F, 1.0F },
								   { 1.0F, -1.0F, 1.0F },
								   // LeFt Face
								   { -1.0F, -1.0F, -1.0F },
								   { -1.0F, -1.0F, 1.0F },
								   { -1.0F, 1.0F, 1.0F },
								   { -1.0F, 1.0F, -1.0F } };

	std::vector<uint32_t> indices = {
		0,	1,	2,	2,	3,	0, // Front
		4,	5,	6,	6,	7,	4, // Back
		8,	9,	10, 10, 11, 8, // Top
		12, 13, 14, 14, 15, 12, // Bottom
		16, 17, 18, 18, 19, 16, // Right
		20, 21, 22, 22, 23, 20 // Left
	};

	m_IndexCount = static_cast<uint32>(indices.size());

	m_CubeVertexBuffer = CreateUnique<Buffer>(
		device, "Skybox_Vertices", sizeof(vec3), vertices.size(), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
	m_CubeVertexBuffer->Map();
	m_CubeVertexBuffer->Write(vertices.data());

	m_CubeIndexBuffer = CreateUnique<Buffer>(
		device, "Skybox_Indices", sizeof(uint32_t), indices.size(), VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
	m_CubeIndexBuffer->Map();
	m_CubeIndexBuffer->Write(indices.data());
}

// !! TODO : should be an event
void SkyboxRenderSystem::SetSkyboxTexture(const Ref<Graphics::Resources::Texture2D> &hdrTexture) {
	m_SkyboxTexture = hdrTexture;
}

void SkyboxRenderSystem::OnUpdate(const FrameSpec &frameSpec) {
	if ((frameSpec.scene == nullptr) || !m_SkyboxTexture) {
		return;
	}

	auto textureInfo = m_SkyboxTexture->GetDescriptorImageInfo();
	BindTexture(frameSpec.scene, 1, 0, &textureInfo);

	UpdateDescriptorsForScene(frameSpec);
}

void SkyboxRenderSystem::OnEvent(Events::Event &event) {}

void SkyboxRenderSystem::OnRender(const FrameSpec &frameSpec) {
	if ((frameSpec.scene == nullptr) || !m_SkyboxTexture) {
		return;
	}
	SceneManagement::Entity skyLightEntity =
		frameSpec.scene->GetEntityManager()->FindFirst<SceneManagement::Components::SkyLightComponent>(
			[](SceneManagement::Entity, SceneManagement::Components::SkyLightComponent &comp) {
				return comp.IsActive();
			});

	auto &skyLightComponent = skyLightEntity.GetComponent<SceneManagement::Components::SkyLightComponent>();

	if (!skyLightComponent.ShouldRenderSkybox()) {
		return;
	}

	m_Pipeline->Bind(frameSpec.commandBuffer);

	VkDescriptorSet textureSet = GetSceneDescriptorSet(frameSpec.scene, 1, frameSpec.frameIndex);

	if (frameSpec.cameraDescriptorSet == VK_NULL_HANDLE || textureSet == VK_NULL_HANDLE) {
		AQUILA_LOG_ERROR("Missing skybox descriptor sets!");
		return;
	}

	std::array<VkDescriptorSet, 2> sets = {
		frameSpec.cameraDescriptorSet, // set 0: global camera
		textureSet // set 1: skybox texture
	};

	vkCmdBindDescriptorSets(frameSpec.commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_PipelineLayout, 0, 2,
							sets.data(), 0, nullptr);

	SkyboxPushConstants push{};
	push.intensity = skyLightComponent.GetIntensity();
	push.lod = skyLightComponent.GetSkyboxLOD();

	vkCmdPushConstants(frameSpec.commandBuffer, m_PipelineLayout, VK_SHADER_STAGE_FRAGMENT_BIT, 0,
					   sizeof(SkyboxPushConstants), &push);

	VkBuffer vertexBuffers[] = { m_CubeVertexBuffer->GetBuffer() };
	VkDeviceSize offsets[] = { 0 };
	vkCmdBindVertexBuffers(frameSpec.commandBuffer, 0, 1, vertexBuffers, offsets);
	vkCmdBindIndexBuffer(frameSpec.commandBuffer, m_CubeIndexBuffer->GetBuffer(), 0, VK_INDEX_TYPE_UINT32);

	vkCmdDrawIndexed(frameSpec.commandBuffer, m_IndexCount, 1, 0, 0, 0);
}

} // namespace Aquila::Rendering::Systems
