#include "Aquila/Rendering/SceneFrameData.h"
#include "Aquila/GFX/GfxContext.h"
#include "Aquila/Scene/Scene.h"
#include "Aquila/Scene/Entity.h"
#include "Aquila/Scene/Components/CameraComponent.h"
#include "Aquila/Scene/Components/TransformComponent.h"
#include "Aquila/Foundation/Macros.h"

namespace Aquila::Rendering {

using namespace SceneManagement::Components;

SceneFrameData::SceneFrameData(GFX::GfxContext &ctx, uint32 width, uint32 height)
	: m_Ctx(ctx), m_Width(width), m_Height(height) {
	m_Layout = ctx.CreateDescriptorSetLayout({
		.bindings = { {
			.binding = 0,
			.type = RHI::DescriptorType::UniformBuffer,
			.stages = RHI::ShaderStageFlags::Vertex | RHI::ShaderStageFlags::Fragment | RHI::ShaderStageFlags::Compute,
			.count = 1,
		} },
	});

	for (uint32 i = 0; i < SharedConstants::MAX_FRAMES_IN_FLIGHT; ++i) {
		m_Buffers[i] = ctx.CreateBuffer({
			.size = sizeof(GpuFrameData),
			.usage = RHI::BufferUsage::UniformBuffer,
			.domain = RHI::MemoryDomain::CPU_TO_GPU,
			.debugName = "FrameData_" + std::to_string(i),
		});

		m_Sets[i] = ctx.AllocateDescriptorSet(*m_Layout);
		m_Sets[i]->SetBuffer(0, *m_Buffers[i]).Flush();
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
	data.prevViewProjection = data.viewProjection; // for TAA

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

void SceneFrameData::Update(SceneManagement::Scene &scene, float deltaTime, uint32 frameSlot) {
	m_Time += deltaTime;

	GpuFrameData gpuData{};
	gpuData.time = m_Time;
	gpuData.deltaTime = deltaTime;
	gpuData.frameIndex = m_FrameIndex++;
	gpuData.screenResolution = vec2(static_cast<float>(m_Width), static_cast<float>(m_Height));

	// Main (active) camera
	if (scene.HasActiveCamera()) {
		auto camEntity = scene.GetActiveCameraEntity();
		if (camEntity.HasAllComponents<CameraComponent, TransformComponent>()) {
			auto &cam = camEntity.GetComponent<CameraComponent>();
			auto &transform = camEntity.GetComponent<TransformComponent>();
			gpuData.mainCamera = BuildCameraData(cam, transform, 0, m_Width, m_Height);
		}
	}

	// All cameras in scene order (up to MAX_CAMERAS)
	auto &registry = scene.GetRegistry();
	auto view = registry.view<CameraComponent, TransformComponent>();
	uint32 cameraCount = 0;
	for (auto entity : view) {
		if (cameraCount >= SharedConstants::MAX_CAMERAS) {
			break;
		}
		auto &cam = view.get<CameraComponent>(entity);
		auto &transform = view.get<TransformComponent>(entity);
		gpuData.cameras[cameraCount] = BuildCameraData(cam, transform, cameraCount, m_Width, m_Height);
		++cameraCount;
	}
	gpuData.cameraCount = cameraCount;

	m_Buffers[frameSlot]->Write(&gpuData, sizeof(GpuFrameData));
}

void SceneFrameData::OnResize(uint32 width, uint32 height) {
	m_Width = width;
	m_Height = height;
}

GFX::GfxDescriptorSet &SceneFrameData::GetDescriptorSet(uint32 frameSlot) const {
	return *m_Sets[frameSlot];
}

} // namespace Aquila::Rendering
