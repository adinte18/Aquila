// LightingRenderingSystem.cpp - Updated for Dynamic Rendering
#include "Aquila/Rendering/Systems/LightingRenderingSystem.h"
#include "Aquila/Core/Defines.h"
#include "Aquila/Events/Event.h"
#include "Aquila/Graphics/Core/Swapchain.h"
#include "Aquila/Graphics/Pipeline/DynamicRenderingHelper.h"
#include "Aquila/Graphics/Pipeline/RenderSettings.h"
#include "Aquila/Math/EnvBaker.h"
#include "Aquila/Rendering/Camera.h"
#include "Aquila/Scene/Components/LightComponent.h"
#include "Aquila/Scene/Components/SkyLightComponent.h"
#include "Aquila/Scene/Components/TransformComponent.h"
#include "Aquila/Scene/EntityManager.h"
#include "Aquila/Scene/Scene.h"

#include "Aquila/Foundation/Profiler.h"

namespace Aquila::Rendering::Systems {

using namespace Aquila::Graphics::Helpers;

LightingRenderSystem::LightingRenderSystem(Device &device, const std::vector<Ref<DescriptorSetLayout>> &layouts)
	: RenderingSystemBase(device) {
	m_Layouts = layouts;

	m_LightsBuffers.resize(SharedConstants::MAX_FRAMES_IN_FLIGHT);
	for (uint32 i = 0; i < SharedConstants::MAX_FRAMES_IN_FLIGHT; ++i) {
		m_LightsBuffers[i] =
			CreateUnique<Buffer>(device, "Lighting_LightsBuffer" + std::to_string(i), sizeof(LightsUniformData), 1,
								 VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
		m_LightsBuffers[i]->Map();
	}

	m_EnvironmentBuffers.resize(SharedConstants::MAX_FRAMES_IN_FLIGHT);
	for (uint32 i = 0; i < SharedConstants::MAX_FRAMES_IN_FLIGHT; ++i) {
		m_EnvironmentBuffers[i] = CreateUnique<Buffer>(
			device, "Lighting_EnvironmentBuffer" + std::to_string(i), sizeof(EnvironmentUniformData), 1,
			VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
		m_EnvironmentBuffers[i]->Map();
	}

	CreateFullscreenQuad();
	CreatePipelineLayout();

	// Create pipeline for lighting pass (single color output)
	auto lightingFormats = PipelineRenderingFormats::SingleColor(VK_FORMAT_R8G8B8A8_UNORM);
	CreatePipeline(lightingFormats);
}

void LightingRenderSystem::CreatePipelineLayout() {
	std::vector<VkDescriptorSetLayout> layouts;
	layouts.reserve(m_Layouts.size());
	for (const auto &layout : m_Layouts) {
		layouts.push_back(layout->GetDescriptorSetLayout());
	}

	VkPushConstantRange pushConstantRange{};
	pushConstantRange.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
	pushConstantRange.offset = 0;
	pushConstantRange.size = sizeof(PushConstants);

	VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
	pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutInfo.pushConstantRangeCount = 1;
	pipelineLayoutInfo.pPushConstantRanges = &pushConstantRange;
	pipelineLayoutInfo.setLayoutCount = static_cast<uint32>(layouts.size());
	pipelineLayoutInfo.pSetLayouts = layouts.data();

	AQUILA_VULKAN_CHECK(vkCreatePipelineLayout(device.GetDevice(), &pipelineLayoutInfo, nullptr, &m_PipelineLayout));
}

void LightingRenderSystem::CreatePipeline(const PipelineRenderingFormats &renderingFormats) {
	PipelineConfigInfo pipelineConfig{};
	Pipeline::DefaultPipelineConfig(pipelineConfig);

	pipelineConfig.pipelineLayout = m_PipelineLayout;
	pipelineConfig.depthStencilInfo.depthTestEnable = VK_FALSE;
	pipelineConfig.depthStencilInfo.depthWriteEnable = VK_FALSE;
	pipelineConfig.colorBlendAttachments[0].blendEnable = VK_FALSE;
	pipelineConfig.rasterizationInfo.cullMode = VK_CULL_MODE_NONE;
	pipelineConfig.bindingDescriptions.clear();
	pipelineConfig.attributeDescriptions.clear();

	VkVertexInputBindingDescription bindingDesc{};
	bindingDesc.binding = 0;
	bindingDesc.stride = sizeof(vec2);
	bindingDesc.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

	VkVertexInputAttributeDescription attrDesc{};
	attrDesc.binding = 0;
	attrDesc.location = 0;
	attrDesc.format = VK_FORMAT_R32G32_SFLOAT;
	attrDesc.offset = 0;

	pipelineConfig.bindingDescriptions.push_back(bindingDesc);
	pipelineConfig.attributeDescriptions.push_back(attrDesc);

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
	pipelineConfig.pNext = &renderingCreateInfo;

	m_Pipeline = CreateUnique<Pipeline>(device, std::string(IMMUTABLE_SHADERS_PATH) + "PBR.slang", pipelineConfig);
}

void LightingRenderSystem::CreateFullscreenQuad() {
	std::vector<vec2> vertices = { { -1.0f, -1.0f }, { 1.0f, -1.0f }, { 1.0f, 1.0f }, { -1.0f, 1.0f } };
	std::vector<uint32> indices = { 0, 1, 2, 2, 3, 0 };

	m_QuadVertexBuffer = CreateUnique<Buffer>(
		device, "Fullscreen_QuadVertices", sizeof(vec2), vertices.size(), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
	m_QuadVertexBuffer->Map();
	m_QuadVertexBuffer->Write(vertices.data());

	m_QuadIndexBuffer = CreateUnique<Buffer>(
		device, "Fullscreen_QuadIndices", sizeof(uint32), indices.size(), VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
	m_QuadIndexBuffer->Map();
	m_QuadIndexBuffer->Write(indices.data());
}

void LightingRenderSystem::SetEnvironment(const SceneManagement::Components::SHCoefficients &sh, float intensity,
										  const vec3 &tint) {
	m_CurrentSH = sh;
	m_EnvironmentIntensity = intensity;
	m_EnvironmentTint = tint;
	m_EnvironmentDirty = true;
}

void LightingRenderSystem::OnEvent(Events::Event &event) {}

void LightingRenderSystem::OnUpdate(const FrameSpec &frameSpec) {
	if (frameSpec.scene == nullptr) {
		return;
	}

	auto &cache = m_SceneCaches[frameSpec.scene];
	{
		uint32 currentLightCount =
			frameSpec.scene->GetEntityManager()->Count<SceneManagement::Components::LightComponent>();

		if (currentLightCount != cache.lastLightCount) {
			cache.lastLightCount = currentLightCount;
			cache.lightsDirtyFramesRemaining = SharedConstants::MAX_FRAMES_IN_FLIGHT;
		}
	}

	auto &lightBuffer = m_LightsBuffers[frameSpec.frameIndex];
	auto &envBuffer = m_EnvironmentBuffers[frameSpec.frameIndex];

	if (cache.lightsDirtyFramesRemaining > 0) {
		m_LightCount = 0;

		LightsUniformData lightsData{};
		// for now only directional lights
		frameSpec.scene->GetEntityManager()
			->ForEach<SceneManagement::Components::LightComponent, SceneManagement::Components::TransformComponent>(
				[&](SceneManagement::Components::LightComponent &light,
					SceneManagement::Components::TransformComponent &transform) {
					if (!light.m_IsActive || m_LightCount >= Defaults::MaxDirectionalLights) {
						return;
					}

					LightData ld{};
					ld.color = light.m_Color;
					ld.intensity = light.m_Intensity;
					ld.direction = light.m_Direction;
					ld.range = light.m_Range;
					ld.position = vec3(transform.GetLocalPosition());
					ld.type = static_cast<int>(light.m_Type);
					ld.innerCone = light.m_InnerConeAngle;
					ld.outerCone = light.m_OuterConeAngle;
					ld.isActive = 1;

					lightsData.lights[m_LightCount] = ld;
					m_LightCount++;
				});

		lightBuffer->Write(&lightsData, sizeof(LightsUniformData));
		if (lightBuffer->Flush() != VK_SUCCESS) {
			AQUILA_LOG_CRITICAL("Buffer flush issue!");
		}

		--cache.lightsDirtyFramesRemaining;
	}

	{
		bool foundSkyLight = false;
		frameSpec.scene->GetEntityManager()->FindFirst<SceneManagement::Components::SkyLightComponent>(
			[&](SceneManagement::Entity entity, SceneManagement::Components::SkyLightComponent &skyLight) {
				if (!skyLight.IsActive()) {
					return false;
				}

				if (skyLight.IsDirty() && skyLight.GetHDRTexture()) {
					skyLight.SetIrradiance(EnvironmentBaker::BakeToSH(skyLight.GetHDRTexture()));
				}

				EnvironmentUniformData envData{};
				for (int i = 0; i < 9; ++i) {
					envData.shCoeffs[i] = vec4(skyLight.GetIrradiance().coeffs[i], 0.0f);
				}

				envData.tint = skyLight.GetTint();
				envData.intensity = skyLight.GetIntensity();
				envData.useEnvironment = 1;

				for (uint32 i = 0; i < SharedConstants::MAX_FRAMES_IN_FLIGHT; ++i) {
					m_EnvironmentBuffers[i]->Write(&envData, sizeof(EnvironmentUniformData));
					AQUILA_VULKAN_CHECK(m_EnvironmentBuffers[i]->Flush());
				}
				foundSkyLight = true;
				return true;
			});
		if (!foundSkyLight) {
			m_EnvironmentEnabled = false;

			m_CurrentSH = SceneManagement::Components::SHCoefficients{};
			m_EnvironmentIntensity = 0.0F;
			m_EnvironmentTint = vec3(1.0F);
		}
	}

	auto lightsInfo = lightBuffer->DescriptorInfo();
	auto envInfo = envBuffer->DescriptorInfo();

	BindUniformBuffer(frameSpec.scene, 1, 0, &lightsInfo);
	BindUniformBuffer(frameSpec.scene, 1, 1, &envInfo);

	UpdateDescriptorsForScene(frameSpec);
}

void LightingRenderSystem::OnRender(const FrameSpec &frameSpec) {
	if (frameSpec.scene == nullptr) {
		return;
	}

	m_Pipeline->Bind(frameSpec.commandBuffer);

	VkDescriptorSet set1 = GetSceneDescriptorSet(frameSpec.scene, 1, frameSpec.frameIndex);
	VkDescriptorSet set2 = GetSceneDescriptorSet(frameSpec.scene, 2, frameSpec.frameIndex);
	VkDescriptorSet set3 = GetSceneDescriptorSet(frameSpec.scene, 3, frameSpec.frameIndex);

	if (set1 == VK_NULL_HANDLE || set2 == VK_NULL_HANDLE || set3 == VK_NULL_HANDLE) {
		AQUILA_LOG_ERROR("Missing required descriptor sets!");
		return;
	}

	std::array<VkDescriptorSet, 4> descriptorSets = {
		frameSpec.cameraDescriptorSet, // set 0
		set1, // set 1 - lights + env
		set2, // set 2 - gbuffer
		set3 // set 3 - shadows
	};

	vkCmdBindDescriptorSets(frameSpec.commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_PipelineLayout, 0,
							static_cast<uint32>(descriptorSets.size()), descriptorSets.data(), 0, nullptr);

	VkBuffer vertexBuffers[] = { m_QuadVertexBuffer->GetBuffer() };
	VkDeviceSize offsets[] = { 0 };

	vkCmdBindVertexBuffers(frameSpec.commandBuffer, 0, 1, vertexBuffers, offsets);
	vkCmdBindIndexBuffer(frameSpec.commandBuffer, m_QuadIndexBuffer->GetBuffer(), 0, VK_INDEX_TYPE_UINT32);

	PushConstants push{};
	push.lightCount = m_LightCount;

	vkCmdPushConstants(frameSpec.commandBuffer, m_PipelineLayout, VK_SHADER_STAGE_FRAGMENT_BIT, 0,
					   sizeof(PushConstants), &push);

	vkCmdDrawIndexed(frameSpec.commandBuffer, 6, 1, 0, 0, 0);
}

} // namespace Aquila::Rendering::Systems
