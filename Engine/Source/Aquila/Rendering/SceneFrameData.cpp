#include "Aquila/Rendering/SceneFrameData.h"
#include "Aquila/GFX/GfxContext.h"
#include "Aquila/Scene/Scene.h"
#include "Aquila/Scene/Entity.h"
#include "Aquila/Scene/Components/CameraComponent.h"
#include "Aquila/Scene/Components/TransformComponent.h"
#include "Aquila/Scene/Components/LightComponent.h"
#include "Aquila/Scene/Components/SkyLightComponent.h"
#include "Aquila/Scene/Components/MaterialComponent.h"
#include "Aquila/Foundation/Macros.h"

#include <glm/gtc/constants.hpp>

namespace Aquila::Rendering {

using namespace SceneManagement::Components;

SceneFrameData::SceneFrameData(GFX::GfxContext &ctx, uint32 width, uint32 height)
	: m_Ctx(ctx), m_Width(width), m_Height(height) {
	m_Layout = ctx.CreateDescriptorSetLayout({
        .bindings = {
            {
                .binding = 0,
                .type    = RHI::DescriptorType::UniformBuffer,
                .stages  = RHI::ShaderStageFlags::Vertex | RHI::ShaderStageFlags::Fragment | RHI::ShaderStageFlags::Compute,
                .count   = 1,
            },
            {
                .binding = 1,
                .type    = RHI::DescriptorType::StorageBuffer,
                .stages  = RHI::ShaderStageFlags::Fragment | RHI::ShaderStageFlags::Compute,
                .count   = 1,
            },
            {
                .binding = 2,
                .type    = RHI::DescriptorType::UniformBuffer,
                .stages  = RHI::ShaderStageFlags::Fragment | RHI::ShaderStageFlags::Compute,
                .count   = 1,
            },
            {
                .binding = 3,
                .type    = RHI::DescriptorType::StorageBuffer,
                .stages  = RHI::ShaderStageFlags::Vertex | RHI::ShaderStageFlags::Fragment | RHI::ShaderStageFlags::Compute,
                .count   = 1,
            },
            {
                .binding = 4,
                .type    = RHI::DescriptorType::StorageBuffer,
                .stages  = RHI::ShaderStageFlags::Fragment | RHI::ShaderStageFlags::Compute,
                .count   = 1,
            },
            {
                .binding = 5,
                .type    = RHI::DescriptorType::StorageBuffer,
                .stages  = RHI::ShaderStageFlags::Fragment | RHI::ShaderStageFlags::Compute,
                .count   = 1,
            },
        },
    });

	for (uint32 i = 0; i < SharedConstants::MAX_FRAMES_IN_FLIGHT; ++i) {
		m_FrameBuffers[i] = ctx.CreateBuffer({
			.size = sizeof(GpuFrameData),
			.usage = RHI::BufferUsage::UniformBuffer,
			.domain = RHI::MemoryDomain::CPU_TO_GPU,
			.debugName = "FrameData_" + std::to_string(i),
		});

		m_LightBuffers[i] = ctx.CreateBuffer({
			.size = sizeof(GpuLightData) * SharedConstants::MAX_LIGHTS,
			.usage = RHI::BufferUsage::StorageBuffer,
			.domain = RHI::MemoryDomain::CPU_TO_GPU,
			.debugName = "LightData_" + std::to_string(i),
		});

		m_EnvBuffers[i] = ctx.CreateBuffer({
			.size = sizeof(GpuEnvironmentData),
			.usage = RHI::BufferUsage::UniformBuffer,
			.domain = RHI::MemoryDomain::CPU_TO_GPU,
			.debugName = "EnvironmentData_" + std::to_string(i),
		});

		m_MaterialBuffers[i] = ctx.CreateBuffer({
			.size = sizeof(Graphics::GpuSurfaceData) * SharedConstants::MAX_MATERIALS,
			.usage = RHI::BufferUsage::StorageBuffer,
			.domain = RHI::MemoryDomain::CPU_TO_GPU,
			.debugName = "MaterialData_" + std::to_string(i),
		});

		m_Sets[i] = ctx.AllocateDescriptorSet(*m_Layout);
	}

	m_LightIndexListBuffer = ctx.CreateBuffer({
		.size = sizeof(uint32) * SharedConstants::CLUSTER_COUNT * SharedConstants::MAX_LIGHTS_PER_CLUSTER,
		.usage = RHI::BufferUsage::StorageBuffer,
		.domain = RHI::MemoryDomain::GPU_ONLY,
		.debugName = "LightIndexList",
	});
	m_ClusterLightInfoBuffer = ctx.CreateBuffer({
		.size = 2 * sizeof(uint32) * SharedConstants::CLUSTER_COUNT,
		.usage = RHI::BufferUsage::StorageBuffer,
		.domain = RHI::MemoryDomain::GPU_ONLY,
		.debugName = "ClusterLightInfo",
	});

	for (uint32 i = 0; i < SharedConstants::MAX_FRAMES_IN_FLIGHT; ++i) {
		m_Sets[i]
			->SetBuffer(0, *m_FrameBuffers[i])
			.SetBuffer(1, *m_LightBuffers[i])
			.SetBuffer(2, *m_EnvBuffers[i])
			.SetBuffer(3, *m_MaterialBuffers[i])
			.SetBuffer(4, *m_LightIndexListBuffer)
			.SetBuffer(5, *m_ClusterLightInfoBuffer)
			.Flush();
	}
}

static GpuCameraData BuildCameraData(const CameraComponent &cam, const TransformComponent &transform, uint32 index,
									 uint32 width, uint32 height) {
	GpuCameraData data{};

	const vec3 pos = transform.GetWorldPosition();
	const quat rot = transform.GetLocalRotation();

	data.view = cam.GetViewMatrix(pos, rot);
	data.projection = cam.GetProjectionMatrix();
	data.viewProjection = data.projection * data.view;
	data.inverseView = glm::inverse(data.view);
	data.inverseProjection = glm::inverse(data.projection);
	data.inverseViewProjection = glm::inverse(data.viewProjection);
	data.prevViewProjection = data.viewProjection;

	data.position = vec4(pos, 0.f);
	data.forward = vec4(cam.GetForwardDirection(rot), 0.f);
	data.up = vec4(cam.GetUpDirection(rot), 0.f);
	data.right = vec4(cam.GetRightDirection(rot), 0.f);
	data.nearPlane = cam.nearPlane;
	data.farPlane = cam.farPlane;
	data.fov = cam.fov;
	data.aspectRatio = cam.aspectRatio;
	data.resolution = vec2(static_cast<float>(width), static_cast<float>(height));
	data.jitter = vec2(0.f);
	data.isOrthographic = cam.isOrthographic ? 1u : 0u;
	data.cameraIndex = index;

	return data;
}

static GpuLightData BuildLightData(const LightComponent &light, const TransformComponent &transform) {
	GpuLightData data{};

	data.positionAndRange = vec4(vec3(transform.GetLocalPosition()), light.m_Range);
	data.colorAndIntensity = vec4(light.m_Color, light.m_Intensity);
	data.directionAndType = vec4(light.m_Direction, static_cast<float>(light.m_Type));
	data.shadowIndex = -1;
	data.flags = 0;

	switch (light.m_Type) {
	case LightComponent::Type::Spot: {
		data.cosInnerAngle = glm::cos(glm::radians(light.m_InnerConeAngle));
		data.cosOuterAngle = glm::cos(glm::radians(light.m_OuterConeAngle));
		break;
	}
	case LightComponent::Type::Area: {
		const mat3 rot = glm::mat3_cast(transform.GetLocalRotation());
		const vec3 right = rot[0];
		const vec3 up = rot[1];
		data.rightAndWidth = vec4(right, light.m_AreaSize.x);
		data.upAndHeight = vec4(up, light.m_AreaSize.y);
		data.cosInnerAngle = 1.0f;
		data.cosOuterAngle = 1.0f;
		break;
	}
	default:
		data.cosInnerAngle = 1.0f;
		data.cosOuterAngle = 1.0f;
		break;
	}

	return data;
}

void SceneFrameData::Update(SceneManagement::Scene &scene, float deltaTime, uint32 frameSlot) {
	m_Time += deltaTime;

	GpuFrameData gpuFrame{};
	gpuFrame.time = m_Time;
	gpuFrame.deltaTime = deltaTime;
	gpuFrame.frameIndex = m_FrameIndex++;
	gpuFrame.screenResolution = vec2(static_cast<float>(m_Width), static_cast<float>(m_Height));

	if (scene.HasActiveCamera()) {
		auto camEntity = scene.GetActiveCameraEntity();
		if (camEntity.HasAllComponents<CameraComponent, TransformComponent>()) {
			auto &cam = camEntity.GetComponent<CameraComponent>();
			auto &transform = camEntity.GetComponent<TransformComponent>();
			gpuFrame.mainCamera = BuildCameraData(cam, transform, 0, m_Width, m_Height);
		}
	}

	auto &registry = scene.GetRegistry();
	{
		auto view = registry.view<CameraComponent, TransformComponent>();
		uint32 count = 0;
		for (auto entity : view) {
			if (count >= SharedConstants::MAX_CAMERAS) {
				break;
			}
			gpuFrame.cameras[count] = BuildCameraData(view.get<CameraComponent>(entity),
													  view.get<TransformComponent>(entity), count, m_Width, m_Height);
			++count;
		}
		gpuFrame.cameraCount = count;
	}

	GpuLightData lights[SharedConstants::MAX_LIGHTS];
	uint32 lightCount = 0;
	{
		auto view = registry.view<LightComponent, TransformComponent>();
		for (auto entity : view) {
			if (lightCount >= SharedConstants::MAX_LIGHTS) {
				break;
			}
			const auto &light = view.get<LightComponent>(entity);
			const auto &transform = view.get<TransformComponent>(entity);
			if (!light.m_IsActive) {
				continue;
			}
			lights[lightCount++] = BuildLightData(light, transform);
		}
	}
	gpuFrame.lightCount = lightCount;

	m_FrameBuffers[frameSlot]->Write(&gpuFrame, sizeof(GpuFrameData));
	if (lightCount > 0) {
		m_LightBuffers[frameSlot]->Write(lights, sizeof(GpuLightData) * lightCount);
	}

	GpuEnvironmentData envData{};
	{
		auto view = registry.view<SkyLightComponent>();
		for (auto entity : view) {
			const auto &sky = view.get<SkyLightComponent>(entity);
			if (!sky.IsActive()) {
				continue;
			}
			const auto &sh = sky.GetIrradiance();
			for (int i = 0; i < 9; ++i) {
				envData.shCoeffs[i] = vec4(sh.coeffs[i], 0.f);
			}
			envData.tintAndIntensity = vec4(sky.GetTint(), sky.GetIntensity());
			envData.enabled = 1;
			break;
		}
	}
	m_EnvBuffers[frameSlot]->Write(&envData, sizeof(GpuEnvironmentData));

	{
		Graphics::GpuSurfaceData materials[SharedConstants::MAX_MATERIALS];
		uint32 materialCount = 0;
		auto matView = registry.view<MaterialComponent>();
		for (auto entity : matView) {
			if (materialCount >= SharedConstants::MAX_MATERIALS) {
				break;
			}
			auto &comp = matView.get<MaterialComponent>(entity);
			materials[materialCount] = comp.surfaceProperties;
			comp.materialIndex = materialCount++;
		}
		if (materialCount > 0) {
			m_MaterialBuffers[frameSlot]->Write(materials, sizeof(Graphics::GpuSurfaceData) * materialCount);
		}
	}
}

void SceneFrameData::OnResize(uint32 width, uint32 height) {
	m_Width = width;
	m_Height = height;
}

GFX::GfxDescriptorSet &SceneFrameData::GetDescriptorSet(uint32 frameSlot) const {
	return *m_Sets[frameSlot];
}

} // namespace Aquila::Rendering
