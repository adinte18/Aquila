#include "Aquila/Rendering/Systems/CascadedShadowMapsRenderingSystem.h"

#include "Aquila/Core/Defines.h"
#include "Aquila/Math/Math.h"
#include "Aquila/Scene/Components/MetadataComponent.h"
#include "Aquila/Graphics/Core/Swapchain.h"
#include "Aquila/Graphics/Pipeline/DynamicRenderingHelper.h"
#include "Aquila/Utilities/Profiler.h"

#include "Aquila/Scene/EntityManager.h"
namespace Aquila::Rendering::Systems {

using namespace Aquila::Graphics::Helpers;

ShadowRenderSystem::ShadowRenderSystem(Device &device, const std::vector<Ref<DescriptorSetLayout>> &layouts)
	: RenderingSystemBase(device) {
	m_Layouts = layouts;

	// Create uniform buffers
	m_UniformBuffers.resize(SharedConstants::MAX_FRAMES_IN_FLIGHT);
	for (uint32 i = 0; i < SharedConstants::MAX_FRAMES_IN_FLIGHT; ++i) {
		m_UniformBuffers[i] =
			CreateUnique<Buffer>(device, "Shadow_UniformBuffer_" + std::to_string(i), sizeof(ShadowUniformData), 1,
								 VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
		m_UniformBuffers[i]->Map();
	}

	m_CascadeRenderTargets.resize(CASCADE_COUNT);
	for (uint32 i = 0; i < CASCADE_COUNT; ++i) {
		m_CascadeRenderTargets[i] =
			RenderingPipeline::RenderTarget::Builder(device, "ShadowCascade_" + std::to_string(i))
				.WithSize(SHADOW_MAP_SIZE, SHADOW_MAP_SIZE)
				.WithType(RenderingPipeline::RenderTarget::Type::DEPTH_ONLY)
				.WithUsage(RenderingPipeline::RenderTarget::Usage::SHADOW_MAP)
				.WithDepthFormat(VK_FORMAT_D32_SFLOAT)
				.WithDepthUsage(VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT)
				.Build();
	}

	m_CascadeSplits.resize(CASCADE_COUNT);
	m_CascadePipelines.resize(CASCADE_COUNT);

	CreatePipelineLayout();

	// Create pipeline for depth-only rendering
	auto renderingFormats = PipelineRenderingFormats::DepthOnly(VK_FORMAT_D32_SFLOAT);

	CreatePipeline(renderingFormats);
	for (uint32 i = 0; i < CASCADE_COUNT; ++i) {
		m_CascadePipelines[i] = m_Pipeline.get();
	}
}

void ShadowRenderSystem::CreatePipelineLayout() {
	auto *setLayout = m_Layouts[0]->GetDescriptorSetLayout();

	VkPushConstantRange pushConstantRange{};
	pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
	pushConstantRange.offset = 0;
	pushConstantRange.size = sizeof(ShadowPushConstants);

	VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
	pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutInfo.setLayoutCount = 1;
	pipelineLayoutInfo.pSetLayouts = &setLayout;
	pipelineLayoutInfo.pushConstantRangeCount = 1;
	pipelineLayoutInfo.pPushConstantRanges = &pushConstantRange;

	AQUILA_VULKAN_CHECK(vkCreatePipelineLayout(device.GetDevice(), &pipelineLayoutInfo, nullptr, &m_PipelineLayout));
}

void ShadowRenderSystem::CreatePipeline(const PipelineRenderingFormats &renderingFormats) {
	PipelineConfigInfo pipelineConfig{};
	Pipeline::DefaultPipelineConfig(pipelineConfig);

	pipelineConfig.pipelineLayout = m_PipelineLayout;

	pipelineConfig.bindingDescriptions.clear();
	pipelineConfig.attributeDescriptions.clear();

	VkVertexInputBindingDescription bindingDesc{};
	bindingDesc.binding = 0;
	bindingDesc.stride = sizeof(Vertex);
	bindingDesc.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

	VkVertexInputAttributeDescription posAttr{};
	posAttr.binding = 0;
	posAttr.location = 0;
	posAttr.format = VK_FORMAT_R32G32B32_SFLOAT;

	pipelineConfig.bindingDescriptions.push_back(bindingDesc);
	pipelineConfig.attributeDescriptions.push_back(posAttr);

	pipelineConfig.colorBlendAttachments.clear();
	pipelineConfig.depthStencilInfo.depthTestEnable = VK_TRUE;
	pipelineConfig.depthStencilInfo.depthWriteEnable = VK_TRUE;
	pipelineConfig.depthStencilInfo.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;

	pipelineConfig.rasterizationInfo.depthBiasEnable = VK_TRUE;
	pipelineConfig.rasterizationInfo.depthBiasConstantFactor = 1.25F;
	pipelineConfig.rasterizationInfo.depthBiasSlopeFactor = 1.75F;

	pipelineConfig.rasterizationInfo.cullMode = VK_CULL_MODE_FRONT_BIT;

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

	m_Pipeline = CreateUnique<Pipeline>(device, std::string(IMMUTABLE_SHADERS_PATH) + "Shadows.slang", pipelineConfig);
}

void ShadowRenderSystem::OnEvent(Events::Event &event) {}

void ShadowRenderSystem::OnUpdate(const FrameSpec &frameSpec) {
	auto *scene = frameSpec.scene;
	if (scene == nullptr) {
		return;
	}

	const vec3 lightDirection = GetPrimaryDirectionalLightDirection(scene);
	const f32 nearPlane = frameSpec.GetNearPlane();
	const f32 farPlane = frameSpec.GetFarPlane();

	SceneManagement::Components::ShadowQualitySettings activeShadowSettings = m_QualitySettings;

	scene->GetEntityManager()->ForEach<SceneManagement::Components::LightComponent>(
		[&](SceneManagement::Components::LightComponent &light) {
			if (light.IsActive() && light.GetType() == SceneManagement::Components::LightComponent::Type::Directional) {
				activeShadowSettings = light.GetShadowSettings();
			}
		});

	m_QualitySettings = activeShadowSettings;
	CalculateCascadeSplits(nearPlane, farPlane);

	mat4 cameraView = frameSpec.GetViewMatrix();

	for (uint32 i = 0; i < CASCADE_COUNT; ++i) {
		const f32 splitDist = m_CascadeSplits[i];
		const f32 cascadeNear = (i == 0) ? nearPlane : nearPlane + (m_CascadeSplits[i - 1] * (farPlane - nearPlane));
		const f32 cascadeFar = nearPlane + (splitDist * (farPlane - nearPlane));

		//! TODO : the hardcoded standaloneCamera thingy should be eradicated from here, this is temporary and is EXTREMELY bug prone
		m_ShadowData.cascadeViewProj[i] =
			GetLightSpaceMatrix(cascadeNear, cascadeFar, cameraView, lightDirection,
								frameSpec.standaloneCamera->GetFOV(), frameSpec.standaloneCamera->GetAspectRatio());
		m_ShadowData.cascadeSplits[i].x = cascadeFar;
	}
	m_ShadowData.lightDirection = lightDirection;
	m_ShadowData.lightSize = activeShadowSettings.lightSize;
	m_ShadowData.shadowBias = activeShadowSettings.shadowBias;
	m_ShadowData.normalBias = activeShadowSettings.normalBias;
	m_ShadowData.pcfSamples = activeShadowSettings.pcfSamples;

	auto &uniformBuffer = m_UniformBuffers[frameSpec.frameIndex];
	uniformBuffer->Write(&m_ShadowData);
	AQUILA_VULKAN_CHECK(uniformBuffer->Flush());

	auto uniformInfo = uniformBuffer->DescriptorInfo();
	BindUniformBuffer(scene, 0, 0, &uniformInfo);

	UpdateDescriptorsForScene(frameSpec);
}

void ShadowRenderSystem::OnRender(const FrameSpec &frameSpec) {
	auto *scene = frameSpec.scene;
	if (scene == nullptr) {
		return;
	}

	auto &cache = m_SceneCaches[scene];

	uint32 currentCount = scene->GetEntityManager()->Count<SceneManagement::Components::MeshComponent>();
	if (currentCount != cache.lastEntityCount) {
		cache.lastEntityCount = currentCount;
		cache.entitiesDirty = true;
	}

	bool hasDirectionalLight = false;
	scene->GetEntityManager()->ForEach<SceneManagement::Components::LightComponent>(
		[&](const SceneManagement::Components::LightComponent &light) {
			if (light.IsActive() && light.GetType() == SceneManagement::Components::LightComponent::Type::Directional) {
				hasDirectionalLight = true;
			}
		});

	if (!hasDirectionalLight) {
		return;
	}

	if (cache.entitiesDirty) {
		cache.shadowCasters.clear();

		scene->GetEntityManager()->ForEach<SceneManagement::Components::MeshComponent>(
			[&](SceneManagement::Entity entity, SceneManagement::Components::MeshComponent &meshComp) {
				UUID entityID = entity.GetUUID();

				UpdateEntityState(scene, entityID, meshComp);

				if (meshComp.IsValid() && meshComp.castShadows) {
					cache.shadowCasters.push_back(entity);
				}
			});

		AQUILA_LOG_DEBUG("Rebuilt shadow casters cache: {} entities", cache.shadowCasters.size());
		cache.entitiesDirty = false;
	}

	if (cache.shadowCasters.empty()) {
		return;
	}

	VkDescriptorSet sceneDescriptorSet = GetSceneDescriptorSet(scene, 0, frameSpec.frameIndex);
	if (sceneDescriptorSet == VK_NULL_HANDLE) {
		AQUILA_LOG_ERROR("No descriptor set for scene");
		return;
	}

	for (uint32 cascadeIdx = 0; cascadeIdx < CASCADE_COUNT; ++cascadeIdx) {
		auto *depthView = m_CascadeRenderTargets[cascadeIdx]->GetDepthImageView();

		DynamicRendering::BeginDepthOnly(frameSpec.commandBuffer, depthView, SHADOW_MAP_SIZE, true);

		m_CascadePipelines[cascadeIdx]->Bind(frameSpec.commandBuffer);
		vkCmdBindDescriptorSets(frameSpec.commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_PipelineLayout, 0, 1,
								&sceneDescriptorSet, 0, nullptr);

		for (auto &entity : cache.shadowCasters) {
			auto &meshComp = entity.GetComponent<SceneManagement::Components::MeshComponent>();
			auto &transformComp = entity.GetComponent<SceneManagement::Components::TransformComponent>();

			ShadowPushConstants push{};
			push.modelMatrix = transformComp.GetWorldMatrix();
			push.cascadeIndex = cascadeIdx;

			vkCmdPushConstants(frameSpec.commandBuffer, m_PipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0,
							   sizeof(ShadowPushConstants), &push);

			meshComp.data->Bind(frameSpec.commandBuffer);
			meshComp.data->Draw(frameSpec.commandBuffer);
		}

		DynamicRendering::End(frameSpec.commandBuffer);
	}
}

std::array<vec4, 8> ShadowRenderSystem::GetFrustumCornersWorldSpace(const mat4 &projection, const mat4 &view) {
	const mat4 inv = glm::inverse(projection * view);

	std::array<vec4, 8> corners{};
	uint32 idx = 0;
	for (uint32 x = 0; x < 2; ++x) {
		for (uint32 y = 0; y < 2; ++y) {
			for (uint32 z = 0; z < 2; ++z) {
				const vec4 pt = inv * vec4(2.0f * x - 1.0f, 2.0f * y - 1.0f, z == 0 ? 0.0f : 1.0f, 1.0f);
				corners[idx++] = pt / pt.w;
			}
		}
	}
	return corners;
}

vec3 ShadowRenderSystem::GetPrimaryDirectionalLightDirection(SceneManagement::Scene *scene) {
	auto lightDirection = vec3(0.0f, -1.0f, 0.0f);
	bool foundLight = false;

	scene->GetEntityManager()->ForEach<SceneManagement::Components::LightComponent>(
		[&](const SceneManagement::Components::LightComponent &light) {
			if (!foundLight && light.IsActive() &&
				light.GetType() == SceneManagement::Components::LightComponent::Type::Directional) {
				lightDirection = glm::normalize(light.GetDirection());
				foundLight = true;
			}
		});

	return lightDirection;
}

void ShadowRenderSystem::CalculateCascadeSplits(f32 nearPlane, f32 farPlane) {
	const f32 clipRange = farPlane - nearPlane;
	const f32 minZ = nearPlane;
	const f32 maxZ = farPlane;

	const f32 lambda = m_QualitySettings.cascadeSplitLambda;

	for (uint32 i = 0; i < CASCADE_COUNT; ++i) {
		const f32 p = static_cast<f32>(i + 1) / static_cast<f32>(CASCADE_COUNT);
		const f32 logSplit = minZ * std::pow(maxZ / minZ, p);
		const f32 uniformSplit = minZ + (clipRange * p);
		const f32 splitDist = (lambda * logSplit) + ((1.0f - lambda) * uniformSplit);

		m_CascadeSplits[i] = (splitDist - nearPlane) / clipRange;
	}
}

mat4 ShadowRenderSystem::GetLightSpaceMatrix(const f32 nearPlane, const f32 farPlane, const mat4 &view,
											 const vec3 &lightDirection, const f32 fov, const f32 aspectRatio) {
	const mat4 proj = Math::PerspectiveVulkan(Math::Radians(fov), aspectRatio, nearPlane, farPlane);
	const auto corners = GetFrustumCornersWorldSpace(proj, view);

	vec3 center(0.0f);
	for (const auto &corner : corners) {
		center += vec3(corner);
	}
	center /= static_cast<f32>(corners.size());

	const vec3 dir = Math::Normalize(lightDirection);
	mat4 lightView;

	if (std::abs(dir.y) > 0.99f) {
		const vec3 u_axis = vec3(1.0f, 0.0f, 0.0f); // right = X
		const vec3 v_axis = vec3(0.0f, 0.0f, 1.0f); // up = Z
		const vec3 w_axis = dir;					// forward = light dir

		lightView = mat4(1.0f);
		lightView[0][0] = u_axis.x;
		lightView[1][0] = u_axis.y;
		lightView[2][0] = u_axis.z;
		lightView[0][1] = v_axis.x;
		lightView[1][1] = v_axis.y;
		lightView[2][1] = v_axis.z;
		lightView[0][2] = w_axis.x;
		lightView[1][2] = w_axis.y;
		lightView[2][2] = w_axis.z;
		lightView[3][0] = -Math::Dot(u_axis, center);
		lightView[3][1] = -Math::Dot(v_axis, center);
		lightView[3][2] = -Math::Dot(w_axis, center);
	} else {
		const vec3 u_axis = Math::Normalize(Math::Cross(dir, vec3(0.0f, 1.0f, 0.0f)));
		const vec3 v_axis = Math::Normalize(Math::Cross(dir, u_axis));
		const vec3 w_axis = dir;

		lightView = mat4(1.0f);
		lightView[0][0] = u_axis.x;
		lightView[1][0] = u_axis.y;
		lightView[2][0] = u_axis.z;
		lightView[0][1] = v_axis.x;
		lightView[1][1] = v_axis.y;
		lightView[2][1] = v_axis.z;
		lightView[0][2] = w_axis.x;
		lightView[1][2] = w_axis.y;
		lightView[2][2] = w_axis.z;
		lightView[3][0] = -Math::Dot(u_axis, center);
		lightView[3][1] = -Math::Dot(v_axis, center);
		lightView[3][2] = -Math::Dot(w_axis, center);
	}

	float minX = std::numeric_limits<float>::max();
	float maxX = std::numeric_limits<float>::lowest();
	float minY = std::numeric_limits<float>::max();
	float maxY = std::numeric_limits<float>::lowest();
	float minZ = std::numeric_limits<float>::max();
	float maxZ = std::numeric_limits<float>::lowest();

	for (const auto &v : corners) {
		const vec4 trf = lightView * v;
		minX = std::min(minX, trf.x);
		maxX = std::max(maxX, trf.x);
		minY = std::min(minY, trf.y);
		maxY = std::max(maxY, trf.y);
		minZ = std::min(minZ, trf.z);
		maxZ = std::max(maxZ, trf.z);
	}

	minZ -= 500.0f;

	const mat4 lightProj = Math::OrthoVulkan(minX, maxX, minY, maxY, minZ, maxZ);

	return lightProj * lightView;
}
} // namespace Aquila::Rendering::Systems
