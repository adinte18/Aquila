#include "Aquila/Rendering/SceneFrameData.h"
#include "Aquila/GFX/GfxContext.h"
#include "Aquila/Scene/Scene.h"
#include "Aquila/Scene/Entity.h"
#include "Aquila/Scene/Components/CameraComponent.h"
#include "Aquila/Scene/Components/TransformComponent.h"
#include "Aquila/Scene/Components/LightComponent.h"
#include "Aquila/Scene/Components/SkyLightComponent.h"
#include "Aquila/Scene/Components/MaterialComponent.h"
#include "Aquila/Rendering/FrameData.h"
#include "Aquila/Rendering/LightData.h"
#include "Aquila/Graphics/SurfaceData.h"

namespace Aquila::Rendering {

using namespace SceneManagement::Components;

SceneFrameData::SceneFrameData(GFX::GfxContext &ctx, Uint32 width, Uint32 height)
	: m_ctx(ctx), m_width(width), m_height(height) {
	m_layout = m_ctx.create_descriptor_set_layout({
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

	for (Uint32 i = 0; i < SharedConstants::MAX_FRAMES_IN_FLIGHT; ++i) {
		m_frame_buffers[i] = ctx.create_buffer({
			.size = sizeof(GpuFrameData),
			.usage = RHI::BufferUsage::UniformBuffer,
			.domain = RHI::MemoryDomain::CpuToGpu,
			.debug_name = "FrameData_" + std::to_string(i),
		});

		m_light_buffers[i] = ctx.create_buffer({
			.size = sizeof(GpuLightData) * SharedConstants::MAX_LIGHTS,
			.usage = RHI::BufferUsage::StorageBuffer,
			.domain = RHI::MemoryDomain::CpuToGpu,
			.debug_name = "LightData_" + std::to_string(i),
		});

		m_env_buffers[i] = ctx.create_buffer({
			.size = sizeof(GpuEnvironmentData),
			.usage = RHI::BufferUsage::UniformBuffer,
			.domain = RHI::MemoryDomain::CpuToGpu,
			.debug_name = "EnvironmentData_" + std::to_string(i),
		});

		m_material_buffers[i] = ctx.create_buffer({
			.size = sizeof(Graphics::GpuSurfaceData) * SharedConstants::MAX_MATERIALS,
			.usage = RHI::BufferUsage::StorageBuffer,
			.domain = RHI::MemoryDomain::CpuToGpu,
			.debug_name = "MaterialData_" + std::to_string(i),
		});

		m_sets[i] = ctx.allocate_descriptor_set(*m_layout);
	}

	m_light_index_list_buffer = ctx.create_buffer({
		.size = sizeof(Uint32) * SharedConstants::CLUSTER_COUNT * SharedConstants::MAX_LIGHTS_PER_CLUSTER,
		.usage = RHI::BufferUsage::StorageBuffer,
		.domain = RHI::MemoryDomain::GpuOnly,
		.debug_name = "LightIndexList",
	});
	m_cluster_light_info_buffer = ctx.create_buffer({
		.size = 2 * sizeof(Uint32) * SharedConstants::CLUSTER_COUNT,
		.usage = RHI::BufferUsage::StorageBuffer,
		.domain = RHI::MemoryDomain::GpuOnly,
		.debug_name = "ClusterLightInfo",
	});

	for (Uint32 i = 0; i < SharedConstants::MAX_FRAMES_IN_FLIGHT; ++i) {
		m_sets[i]
			->set_buffer(0, *m_frame_buffers[i])
			.set_buffer(1, *m_light_buffers[i])
			.set_buffer(2, *m_env_buffers[i])
			.set_buffer(3, *m_material_buffers[i])
			.set_buffer(4, *m_light_index_list_buffer)
			.set_buffer(5, *m_cluster_light_info_buffer)
			.flush();
	}
}

static GpuCameraData build_camera_data(const CameraComponent &cam, const TransformComponent &transform, Uint32 index,
									   Uint32 width, Uint32 height) {
	GpuCameraData data{};

	const Vec3 pos = transform.get_world_position();
	const quat rot = transform.get_local_rotation();

	data.view = cam.get_view_matrix(pos, rot);
	data.projection = cam.get_projection_matrix();
	data.view_projection = data.projection * data.view;
	data.inverse_view = glm::inverse(data.view);
	data.inverse_projection = glm::inverse(data.projection);
	data.inverse_view_projection = glm::inverse(data.view_projection);
	data.prev_view_projection = data.view_projection;

	data.position = Vec4(pos, 0.F);
	data.forward = Vec4(cam.get_forward_direction(rot), 0.F);
	data.up = Vec4(cam.get_up_direction(rot), 0.F);
	data.right = Vec4(cam.get_right_direction(rot), 0.F);
	data.near_plane = cam.near_plane;
	data.far_plane = cam.far_plane;
	data.fov = cam.fov;
	data.aspect_ratio = cam.aspect_ratio;
	data.resolution = Vec2(static_cast<float>(width), static_cast<float>(height));
	data.jitter = Vec2(0.F);
	data.is_orthographic = cam.is_orthographic ? 1u : 0u;
	data.camera_index = index;

	return data;
}

static GpuLightData build_light_data(const LightComponent &light, const TransformComponent &transform) {
	GpuLightData data{};

	data.m_position_and_range = Vec4(Vec3(transform.get_local_position()), light.m_range);
	data.m_color_and_intensity = Vec4(light.m_color, light.m_intensity);
	data.m_direction_and_type = Vec4(light.m_direction, static_cast<float>(light.m_type));
	data.m_shadow_index = -1;
	data.m_flags = 0;

	switch (light.m_type) {
	case LightComponent::Type::Spot: {
		data.m_cos_inner_angle = glm::cos(glm::radians(light.m_inner_cone_angle));
		data.m_cos_outer_angle = glm::cos(glm::radians(light.m_outer_cone_angle));
		break;
	}
	case LightComponent::Type::Area: {
		const Mat3 rot = glm::mat3_cast(transform.get_local_rotation());
		const Vec3 right = rot[0];
		const Vec3 up = rot[1];
		data.m_right_and_width = Vec4(right, light.m_area_size.x);
		data.m_up_and_height = Vec4(up, light.m_area_size.y);
		data.m_cos_inner_angle = 1.0f;
		data.m_cos_outer_angle = 1.0f;
		break;
	}
	default:
		data.m_cos_inner_angle = 1.0f;
		data.m_cos_outer_angle = 1.0f;
		break;
	}

	return data;
}

void SceneFrameData::update(SceneManagement::Scene &scene, float delta_time, Uint32 frame_slot) {
	m_time += delta_time;

	GpuFrameData gpu_frame{};
	gpu_frame.time = m_time;
	gpu_frame.delta_time = delta_time;
	gpu_frame.frame_index = m_frame_index++;
	gpu_frame.screen_resolution = Vec2(static_cast<float>(m_width), static_cast<float>(m_height));

	if (scene.has_active_camera()) {
		auto cam_entity = scene.get_active_camera_entity();
		if (cam_entity.has_all_components<CameraComponent, TransformComponent>()) {
			auto &cam = cam_entity.get_component<CameraComponent>();
			auto &transform = cam_entity.get_component<TransformComponent>();
			gpu_frame.main_camera = build_camera_data(cam, transform, 0, m_width, m_height);
		}
	}

	auto &registry = scene.get_registry();
	{
		auto view = registry.view<CameraComponent, TransformComponent>();
		Uint32 count = 0;
		for (auto entity : view) {
			if (count >= SharedConstants::MAX_CAMERAS) {
				break;
			}
			gpu_frame.cameras[count] = build_camera_data(
				view.get<CameraComponent>(entity), view.get<TransformComponent>(entity), count, m_width, m_height);
			++count;
		}
		gpu_frame.camera_count = count;
	}

	GpuLightData lights[SharedConstants::MAX_LIGHTS];
	Uint32 light_count = 0;
	{
		auto view = registry.view<LightComponent, TransformComponent>();
		for (auto entity : view) {
			if (light_count >= SharedConstants::MAX_LIGHTS) {
				break;
			}
			const auto &light = view.get<LightComponent>(entity);
			const auto &transform = view.get<TransformComponent>(entity);
			if (!light.m_is_active) {
				continue;
			}
			lights[light_count++] = build_light_data(light, transform);
		}
	}
	gpu_frame.light_count = light_count;

	m_frame_buffers[frame_slot]->write(&gpu_frame, sizeof(GpuFrameData));
	if (light_count > 0) {
		m_light_buffers[frame_slot]->write(lights, sizeof(GpuLightData) * light_count);
	}

	GpuEnvironmentData env_data{};
	{
		auto view = registry.view<SkyLightComponent>();
		for (auto entity : view) {
			const auto &sky = view.get<SkyLightComponent>(entity);
			if (!sky.is_active()) {
				continue;
			}
			const auto &sh = sky.get_irradiance();
			for (int i = 0; i < 9; ++i) {
				env_data.m_sh_coeffs[i] = Vec4(sh.coeffs[i], 0.F);
			}
			env_data.m_tint_and_intensity = Vec4(sky.get_tint(), sky.get_intensity());
			env_data.m_enabled = 1;
			break;
		}
	}
	m_env_buffers[frame_slot]->write(&env_data, sizeof(GpuEnvironmentData));

	{
		Graphics::GpuSurfaceData materials[SharedConstants::MAX_MATERIALS];
		Uint32 material_count = 0;
		auto mat_view = registry.view<MaterialComponent>();
		for (auto entity : mat_view) {
			if (material_count >= SharedConstants::MAX_MATERIALS) {
				break;
			}
			auto &comp = mat_view.get<MaterialComponent>(entity);
			materials[material_count] = comp.surface_properties;
			comp.material_index = material_count++;
		}
		if (material_count > 0) {
			m_material_buffers[frame_slot]->write(materials, sizeof(Graphics::GpuSurfaceData) * material_count);
		}
	}
}

void SceneFrameData::on_resize(Uint32 width, Uint32 height) {
	m_width = width;
	m_height = height;
}

GFX::GfxDescriptorSet &SceneFrameData::get_descriptor_set(Uint32 frame_slot) const {
	return *m_sets[frame_slot];
}

} // namespace Aquila::Rendering
