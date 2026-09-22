#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest.h>

#include "Aquila/Scene/ComponentRegistry.h"
#include "Aquila/Scene/Components/CameraComponent.h"
#include "Aquila/Scene/Components/LightComponent.h"
#include "Aquila/Scene/Components/MeshComponent.h"
#include "Aquila/Scene/Components/TransformComponent.h"
#include "Aquila/Scene/EntityManager.h"
#include "Aquila/Scene/Scene.h"

using namespace Aquila;
using namespace Aquila::SceneManagement;
using namespace Aquila::SceneManagement::Components;

TEST_SUITE("ComponentRegistry") {
	TEST_CASE("built-in components are registered by name") {
		const auto &registry = ComponentRegistry::instance();
		for (const char *name : { "Transform", "Camera", "Light", "Sky Light", "Mesh", "Material" }) {
			CHECK_MESSAGE(registry.find(name) != nullptr, name);
		}
		CHECK(registry.find("Nope") == nullptr);
	}

	TEST_CASE("descriptors find components on entities") {
		Scene scene("Test");
		Entity entity = scene.get_entity_manager()->create_entity("Thing");
		const auto &registry = ComponentRegistry::instance();

		const ComponentDescriptor &camera = *registry.find("Camera");
		CHECK_FALSE(camera.has(entity));
		CHECK(camera.get(entity) == nullptr);

		entity.add_component<CameraComponent>();
		CHECK(camera.has(entity));
		CHECK(camera.get(entity) == &entity.get_component<CameraComponent>());
	}

	TEST_CASE("properties read and write the real component") {
		Scene scene("Test");
		Entity entity = scene.get_entity_manager()->create_entity("Thing");
		auto &camera_component = entity.add_component<CameraComponent>();

		const ComponentDescriptor &camera = *ComponentRegistry::instance().find("Camera");
		const Reflection::Property &fov = *camera.get_type().find("FOV");
		void *instance = camera.get(entity);

		camera_component.fov = 42.F;
		CHECK(std::get<F32>(fov.get(instance)) == doctest::Approx(42.F));

		fov.set(instance, Reflection::PropertyValue{ 70.F });
		CHECK(camera_component.fov == doctest::Approx(70.F));
	}

	TEST_CASE("getter/setter properties go through the component's own logic") {
		Scene scene("Test");
		Entity entity = scene.get_entity_manager()->create_entity("Thing");
		auto &transform = entity.get_component<TransformComponent>();

		const ComponentDescriptor &descriptor = *ComponentRegistry::instance().find("Transform");
		const Reflection::Property &position = *descriptor.get_type().find("Position");
		position.set(descriptor.get(entity), Reflection::PropertyValue{ Vec3(1.F, 2.F, 3.F) });

		CHECK(transform.get_local_position().y == doctest::Approx(2.F));
		CHECK(transform.is_world_matrix_dirty());
	}

	TEST_CASE("change signals are exposed only for components that have one") {
		Scene scene("Test");
		Entity entity = scene.get_entity_manager()->create_entity("Thing");
		entity.add_component<CameraComponent>();
		entity.add_component<MeshComponent>();
		const auto &registry = ComponentRegistry::instance();

		Signal<void()> *signal = registry.find("Camera")->get_changed_signal(entity);
		REQUIRE(signal != nullptr);
		CHECK(signal == &entity.get_component<CameraComponent>().on_changed);
		CHECK(registry.find("Mesh")->get_changed_signal(entity) == nullptr);
		CHECK(registry.find("Light")->get_changed_signal(entity) == nullptr);
	}

	TEST_CASE("light range is only visible for point lights") {
		Scene scene("Test");
		Entity entity = scene.get_entity_manager()->create_entity("Thing");
		auto &light = entity.add_component<LightComponent>();

		const ComponentDescriptor &descriptor = *ComponentRegistry::instance().find("Light");
		const Reflection::Property &range = *descriptor.get_type().find("Range");
		void *instance = descriptor.get(entity);

		light.set_type(LightComponent::Type::Directional);
		CHECK_FALSE(range.is_visible(instance));
		light.set_type(LightComponent::Type::Point);
		CHECK(range.is_visible(instance));
	}

	TEST_CASE("components can be removed through their descriptor, except the transform") {
		Scene scene("Test");
		Entity entity = scene.get_entity_manager()->create_entity("Thing");
		entity.add_component<CameraComponent>();
		const auto &registry = ComponentRegistry::instance();

		const ComponentDescriptor &camera = *registry.find("Camera");
		REQUIRE(camera.is_removable());
		camera.remove(entity);
		CHECK_FALSE(camera.has(entity));

		CHECK_FALSE(registry.find("Transform")->is_removable());
	}
}
