#include "Aquila/Scene/ComponentRegistry.h"

#include "Aquila/Graphics/Material/Material.h"
#include "Aquila/Scene/Components/CameraComponent.h"
#include "Aquila/Scene/Components/LightComponent.h"
#include "Aquila/Scene/Components/MaterialComponent.h"
#include "Aquila/Scene/Components/MeshComponent.h"
#include "Aquila/Scene/Components/SkyLightComponent.h"
#include "Aquila/Scene/Components/TransformComponent.h"

namespace Aquila::SceneManagement {

using namespace Aquila::SceneManagement::Components;
using Reflection::PropertyHints;

namespace {

void register_builtin_components(ComponentRegistry &registry) {
	registry.register_component<TransformComponent>("Transform", false)
		.property("Position", &TransformComponent::get_local_position, &TransformComponent::set_local_position,
				  PropertyHints{ .speed = 0.1F })
		.property("Scale", &TransformComponent::get_local_scale, &TransformComponent::set_local_scale,
				  PropertyHints{ .speed = 0.1F })
		.property("Rotation", &TransformComponent::get_local_rotation_euler,
				  &TransformComponent::set_local_rotation_euler, PropertyHints{ .speed = 0.5F });

	registry.register_component<CameraComponent>("Camera")
		.property("FOV", &CameraComponent::fov, PropertyHints{ .min = 1.F, .max = 179.F, .speed = 0.5F, .precision = 1 })
		.property("Near", &CameraComponent::near_plane,
				  PropertyHints{ .min = 0.001F, .max = 100.F, .speed = 0.01F, .precision = 3 })
		.property("Far", &CameraComponent::far_plane,
				  PropertyHints{ .min = 0.1F, .max = 10000.F, .speed = 0.01F, .precision = 1 })
		.property("Primary", &CameraComponent::primary, PropertyHints{ .toggle = true })
		.property("Orthographic", &CameraComponent::is_orthographic);

	registry.register_component<LightComponent>("Light")
		.property("Color", &LightComponent::get_color, &LightComponent::set_color, PropertyHints{ .color = true })
		.property("Intensity", &LightComponent::get_intensity, &LightComponent::set_intensity,
				  PropertyHints{ .min = 0.F, .max = 100.F, .speed = 0.5F })
		.property("Range", &LightComponent::get_range, &LightComponent::set_range,
				  PropertyHints{ .min = 0.F, .max = 200.F, .speed = 0.5F })
		.visible_if([](const LightComponent &light) { return light.get_type() == LightComponent::Type::Point; })
		.property("Active", &LightComponent::is_active, &LightComponent::set_active, PropertyHints{ .toggle = true });

	registry.register_component<SkyLightComponent>("Sky Light")
		.property("Source", &SkyLightComponent::get_source, &SkyLightComponent::set_source)
		.options({ { "Procedural", static_cast<Int32>(SkySource::Procedural) },
				   { "HDR Image", static_cast<Int32>(SkySource::HdrImage) } })
		.property("Sun Elevation", &SkyLightComponent::get_sun_elevation, &SkyLightComponent::set_sun_elevation,
				  PropertyHints{ .min = -10.F, .max = 90.F, .speed = 0.5F, .precision = 1 })
		.property("Sun Azimuth", &SkyLightComponent::get_sun_azimuth, &SkyLightComponent::set_sun_azimuth,
				  PropertyHints{ .min = 0.F, .max = 360.F, .speed = 1.F, .precision = 1 })
		.property("Turbidity", &SkyLightComponent::get_turbidity, &SkyLightComponent::set_turbidity,
				  PropertyHints{ .min = 1.F, .max = 10.F, .speed = 0.05F, .precision = 2 })
		.property("Ground Albedo", &SkyLightComponent::get_ground_albedo, &SkyLightComponent::set_ground_albedo,
				  PropertyHints{ .color = true })
		.property("Intensity", &SkyLightComponent::get_intensity, &SkyLightComponent::set_intensity,
				  PropertyHints{ .min = 0.F, .max = 20.F, .speed = 0.05F, .precision = 3 })
		.property("Tint", &SkyLightComponent::get_tint, &SkyLightComponent::set_tint, PropertyHints{ .color = true })
		.property("Active", &SkyLightComponent::is_active, &SkyLightComponent::set_active,
				  PropertyHints{ .toggle = true })
		.property(
			"Render Skybox", [](const SkyLightComponent &sky) { return sky.m_render_skybox; },
			&SkyLightComponent::set_render_skybox, PropertyHints{ .toggle = true });

	registry.register_component<MeshComponent>("Mesh")
		.property("Cast Shadows", &MeshComponent::cast_shadows)
		.property("Receive Shadows", &MeshComponent::receive_shadows);

	registry.register_component<MaterialComponent>("Material")
		.property("Type", &MaterialComponent::type)
		.options({ { "PBR", static_cast<Int32>(Graphics::MaterialType::PBR) },
				   { "Lit", static_cast<Int32>(Graphics::MaterialType::Lit) },
				   { "Unlit", static_cast<Int32>(Graphics::MaterialType::Unlit) },
				   { "Custom", static_cast<Int32>(Graphics::MaterialType::Custom) } })
		.property("Albedo", [](MaterialComponent &m) -> Vec4 & { return m.surface_properties.albedo; },
				  PropertyHints{ .color = true })
		.property("Metallic", [](MaterialComponent &m) -> F32 & { return m.surface_properties.metallic; },
				  PropertyHints{ .min = 0.F, .max = 1.F, .speed = 0.01F, .precision = 3 })
		.property("Roughness", [](MaterialComponent &m) -> F32 & { return m.surface_properties.roughness; },
				  PropertyHints{ .min = 0.F, .max = 1.F, .speed = 0.01F, .precision = 3 });
}

}

ComponentRegistry &ComponentRegistry::instance() {
	static ComponentRegistry registry;
	return registry;
}

ComponentRegistry::ComponentRegistry() {
	register_builtin_components(*this);
}

const ComponentDescriptor *ComponentRegistry::find(std::string_view name) const {
	for (const auto &descriptor : m_descriptors) {
		if (descriptor->get_name() == name) {
			return descriptor.get();
		}
	}
	return nullptr;
}

}
