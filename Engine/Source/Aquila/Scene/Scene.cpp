#include "Aquila/Scene/Scene.h"

#include "Aquila/Assets/AssetManager.h"
#include <algorithm>
#include "Aquila/Foundation/PrimitiveTypes.h"
#include "Aquila/Foundation/Macros.h"
#include "Aquila/Scene/Components/LightComponent.h"
#include "Aquila/Scene/Components/MaterialComponent.h"
#include "Aquila/Scene/Components/MeshComponent.h"
#include "Aquila/Scene/Components/MetadataComponent.h"
#include "Aquila/Scene/Components/SceneNodeComponent.h"
#include "Aquila/Scene/Components/SkyLightComponent.h"
#include "Aquila/Scene/Components/TransformComponent.h"
#include "Aquila/Scene/EntityManager.h"
#include "Aquila/Platform/Filesystem/VirtualFileSystem.h"

namespace Aquila::SceneManagement {
Scene::Scene() {
	m_scene_id = Foundation::UUID::generate();
	on_start();
}

Scene::Scene(const std::string &name) : m_scene_name(name) {
	m_scene_id = Foundation::UUID::generate();
	on_start();
}

Scene::~Scene() = default;

/**
 * @brief Retrieves the entt registry associated with the scene.
 *
 * @return entt::registry& The enthe scene starts, setting up the necessary
 * components for managing entities and their relationships within the scene.
 */
void Scene::on_start() {
	m_entity_manager = std::make_unique<EntityManager>(this);
	// Wire dirty callback on every TransformComponent that gets created.
	m_entity_manager->get_registry()
		.on_construct<Components::TransformComponent>()
		.connect<&Scene::on_transform_construct>(this);
	m_entity_manager->construct_scene_graph();
}

void Scene::on_transform_construct(entt::registry &registry, entt::entity e) {
	auto &t = registry.get<Components::TransformComponent>(e);
	t.set_dirty_callback([this, e]() { mark_transform_dirty(e); });
	mark_transform_dirty(e); // bootstrap: new entity needs its first world matrix compute
}

void Scene::mark_transform_dirty(entt::entity entity) {
	m_dirty_transforms.mark_dirty(entity);
}

bool Scene::has_dirty_ancestor(entt::entity e) const {
	Entity entity(e, const_cast<Scene *>(this));
	auto *node = entity.try_get_component<Components::SceneNodeComponent>();
	while (node && !node->parent.is_null()) {
		if (m_dirty_transforms.is_dirty(node->parent.get_handle())) {
			return true;
		}
		node = node->parent.try_get_component<Components::SceneNodeComponent>();
	}
	return false;
}

int Scene::get_entity_depth(entt::entity e) const {
	int depth = 0;
	Entity entity(e, const_cast<Scene *>(this));
	auto *node = entity.try_get_component<Components::SceneNodeComponent>();
	while (node && !node->parent.is_null()) {
		depth++;
		node = node->parent.try_get_component<Components::SceneNodeComponent>();
	}
	return depth;
}

/**
 * @brief Retrieves the entt registry associated with the scene.
 *
 * @return entt::registry& The entt registry for the scene, which contains all
 * entities and their components.
 */
entt::registry &Scene::get_registry() const {
	return m_entity_manager->get_registry();
}

/**
 * @brief Retrieves the EntityManager associated with the scene.
 *
 * @return EntityManager* Pointer to the EntityManager that manages entities in
 * the scene.
 */
EntityManager *Scene::get_entity_manager() const {
	return m_entity_manager.get();
}

/**
 * @brief Retrieves the name of the scene.
 *
 * @return const std::string& The name of the scene.
 */
const std::string &Scene::get_scene_name() const {
	return m_scene_name;
}

Foundation::UUID Scene::get_handle() const {
	return m_scene_id;
}

void Scene::clear() {
	m_entity_manager->clear();
	m_active_camera_entity = entt::null;
	m_dirty_transforms.clear();
}

void Scene::update_transform_hierarchy() {
	if (m_dirty_transforms.is_empty()) {
		return;
	}

	// Sort dirty entities parent-first so ancestors are always processed before descendants.
	std::vector<entt::entity> sorted = m_dirty_transforms.get_ordered();
	std::ranges::stable_sort(
		sorted, [this](entt::entity a, entt::entity b) { return get_entity_depth(a) < get_entity_depth(b); });

	for (entt::entity e : sorted) {
		// If a dirty ancestor is also in the set, it was (or will be) processed first and its
		// UpdateTransformRecursive call already covers this entity — skip it.
		if (has_dirty_ancestor(e)) {
			continue;
		}

		Entity entity(e, this);
		glm::mat4 parent_world(1.0f);
		auto *node = entity.try_get_component<Components::SceneNodeComponent>();
		if (node && !node->parent.is_null()) {
			auto *parent_transform = node->parent.try_get_component<Components::TransformComponent>();
			if (parent_transform) {
				parent_world = parent_transform->get_world_matrix_lazy();
			}
		}
		update_transform_recursive(entity, parent_world);
	}

	m_dirty_transforms.clear();
}

void Scene::update_transform_recursive(Entity entity, const glm::mat4 &parent_world) {
	if (!entity.is_valid()) {
		return;
	}

	auto *transform = entity.try_get_component<Components::TransformComponent>();
	if (!transform) {
		return;
	}

	// Update this entity's world matrix
	transform->update_world_matrix(parent_world);

	// Recursively update all children
	if (auto *node = entity.try_get_component<Components::SceneNodeComponent>()) {
		for (auto child : node->children) {
			if (child.is_valid()) {
				update_transform_recursive(child, transform->get_world_matrix());
			}
		}
	}
}

void Scene::set_active_camera(Entity camera_entity) {
	if (camera_entity.is_valid() && camera_entity.has_component<Components::CameraComponent>()) {
		// Store the raw entt::entity handle
		m_active_camera_entity = camera_entity.get_handle();
	}
}

Entity Scene::get_active_camera_entity() const {
	// Wrap the entt::entity in an Entity object
	if (m_active_camera_entity != entt::null) {
		return Entity(m_active_camera_entity, const_cast<Scene *>(this));
	}
	return Entity(); // Return invalid entity
}

bool Scene::has_active_camera() const {
	if (m_active_camera_entity == entt::null) {
		return false;
	}

	// Check if entity still exists and has camera component
	return get_registry().valid(m_active_camera_entity) &&
		get_registry().all_of<Components::CameraComponent>(m_active_camera_entity);
}

Entity Scene::find_primary_camera() const {
	// Iterate through all entities with CameraComponent
	auto view = get_registry().view<Components::CameraComponent>();

	for (auto entity : view) {
		const auto &cam = view.get<Components::CameraComponent>(entity);
		if (cam.primary) {
			return Entity(entity, const_cast<Scene *>(this));
		}
	}
	return Entity(); // Return invalid entity if no primary camera found
}

bool Scene::serialize(const std::string &filepath) {
	nlohmann::ordered_json scene_json;

	scene_json["SceneName"] = m_scene_name;

	auto &registry = get_registry();
	scene_json["Entities"] = nlohmann::ordered_json::object();

	auto view = registry.view<Components::MetadataComponent>();

	for (auto entity_handle : view) {
		Entity entity(entity_handle, this);
		nlohmann::ordered_json entity_json;

		// Serialize MetadataComponent
		if (entity.has_component<Components::MetadataComponent>()) {
			auto &meta = entity.get_component<Components::MetadataComponent>();
			entity_json["MetadataComponent"] = { { "Name", meta.get_name() },
												 { "UUID", meta.get_id().to_string() },
												 { "Enabled", meta.is_visible() },
												 { "Selected", meta.is_selected() } };
		}

		// Serialize TransformComponent
		if (entity.has_component<Components::TransformComponent>()) {
			auto &transform = entity.get_component<Components::TransformComponent>();
			entity_json["TransformComponent"] = {
				{ "Position",
				  { transform.get_local_position().x, transform.get_local_position().y,
					transform.get_local_position().z } },
				{ "Rotation",
				  { transform.get_local_rotation().x, transform.get_local_rotation().y,
					transform.get_local_rotation().z } },
				{ "Scale",
				  { transform.get_local_scale().x, transform.get_local_scale().y, transform.get_local_scale().z } }
			};
		}

		// Serialize SceneNodeComponent
		if (entity.has_component<Components::SceneNodeComponent>()) {
			auto &node = entity.get_component<Components::SceneNodeComponent>();
			entity_json["SceneNodeComponent"] = {
				{ "Parent",
				  node.parent.is_null()
					  ? "null"
					  : node.parent.get_component<Components::MetadataComponent>().get_id().to_string() },
				{ "Children", nlohmann::ordered_json::array() }
			};

			for (auto &child : node.children) {
				if (!child.is_null() && child.has_component<Components::MetadataComponent>()) {
					entity_json["SceneNodeComponent"]["Children"].push_back(
						child.get_component<Components::MetadataComponent>().get_id().to_string());
				}
			}
		}

		// Serialize MeshComponent
		if (entity.has_component<Components::MeshComponent>()) {
			auto &mesh = entity.get_component<Components::MeshComponent>();
			entity_json["MeshComponent"] = { { "Path", mesh.data->get_path() },
											 { "DebugName", mesh.data->get_debug_name() },
											 { "CastShadows", mesh.cast_shadows } };
		}

		// Serialize LightComponent with shadow settings
		if (entity.has_component<Components::LightComponent>()) {
			auto &light = entity.get_component<Components::LightComponent>();

			entity_json["LightComponent"] = { { "Type", static_cast<int>(light.m_type) },
											  { "Color", { light.m_color.r, light.m_color.g, light.m_color.b } },
											  { "Intensity", light.m_intensity },
											  { "Range", light.m_range },
											  { "InnerConeAngle", light.m_inner_cone_angle },
											  { "OuterConeAngle", light.m_outer_cone_angle },
											  { "Direction",
												{ light.m_direction.x, light.m_direction.y, light.m_direction.z } },
											  { "IsActive", light.m_is_active } };

			// Serialize shadow settings for directional lights
			if (light.m_type == Components::LightComponent::Type::Directional) {
				const auto &shadow_settings = light.m_shadow_settings;
				entity_json["LightComponent"]["ShadowSettings"] = {
					{ "LightSize", shadow_settings.light_size },
					{ "ShadowBias", shadow_settings.shadow_bias },
					{ "NormalBias", shadow_settings.normal_bias },
					{ "PCFSamples", shadow_settings.pcf_samples },
					{ "CascadeSplitLambda", shadow_settings.cascade_split_lambda },
					{ "BlockerSearchSamples", shadow_settings.blocker_search_samples }
				};
			}
		}

		// Serialize CameraComponent
		if (entity.has_component<Components::CameraComponent>()) {
			auto &cam = entity.get_component<Components::CameraComponent>();
			entity_json["CameraComponent"] = { { "Primary", cam.primary },
											   { "IsOrthographic", cam.is_orthographic },
											   { "FOV", cam.fov },
											   { "AspectRatio", cam.aspect_ratio },
											   { "NearPlane", cam.near_plane },
											   { "FarPlane", cam.far_plane },
											   { "OrthoLeft", cam.ortho_left },
											   { "OrthoRight", cam.ortho_right },
											   { "OrthoTop", cam.ortho_top },
											   { "OrthoBottom", cam.ortho_bottom } };
		}

		// Serialize SkyLightComponent
		// if (entity.HasComponent<Components::SkyLightComponent>()) {
		// 	auto &skyLight = entity.GetComponent<Components::SkyLightComponent>();
		// 	entityJson["SkyLightComponent"] = {
		// 		{ "Active", skyLight.IsActive() },
		// 		{ "Intensity", skyLight.GetIntensity() },
		// 		{ "Tint", { skyLight.GetTint().r, skyLight.GetTint().g, skyLight.GetTint().b } },
		// 		{ "RenderSkybox", skyLight.ShouldRenderSkybox() },
		// 		{ "HDRTexturePath", skyLight.GetHDRTexture() ? skyLight.GetHDRTexture()->GetRHI(). : "" }
		// 	};
		// }

		// Serialize MaterialComponent
		if (entity.has_component<Components::MaterialComponent>()) {
			auto &mat_comp = entity.get_component<Components::MaterialComponent>();
			entity_json["MaterialComponent"] = {
				{ "Type", static_cast<int>(mat_comp.type) },
			};
		}

		scene_json["Entities"][std::to_string(static_cast<int>(entity_handle))] = entity_json;
	}

	const auto vfs_file = Aquila::Platform::Filesystem::VirtualFileSystem::get()->open_file(filepath, AccessMode::Write,
																							OpenMode::Binary);
	if (!vfs_file->is_valid()) {
		return false;
	}

	vfs_file->write(scene_json.dump(4).data(), scene_json.dump(4).size());
	vfs_file->close();

	return true;
}

bool Scene::deserialize(const std::string &filepath, Assets::AssetManager &asset_manager) {
	AQUILA_ASSERT(m_entity_manager != nullptr, "EntityManager is nullptr");

	auto vfs_file =
		Aquila::Platform::Filesystem::VirtualFileSystem::get()->open_file(filepath, AccessMode::Read, OpenMode::Binary);
	if (!vfs_file->is_valid()) {
		return false;
	}

	std::vector<char> buffer(vfs_file->size());
	vfs_file->read(buffer.data(), buffer.size());
	vfs_file->close();

	nlohmann::ordered_json scene_json;
	scene_json = nlohmann::ordered_json::parse(buffer.begin(), buffer.end());

	m_scene_name = scene_json.value("SceneName", "Untitled Scene");

	auto &registry = get_registry();
	registry.clear();

	if (!scene_json.contains("Entities")) {
		return false;
	}

	const auto &entities_json = scene_json["Entities"];

	std::unordered_map<std::string, Entity> uuid_to_entity;

	// First pass: Create all entities with metadata
	for (auto &[idStr, entityData] : entities_json.items()) {
		if (entityData.contains("MetadataComponent")) {
			const auto &meta = entityData["MetadataComponent"];
			Components::MetadataComponent metadata;
			metadata.set_name(meta.value("Name", ""));
			metadata.set_id(Foundation::UUID::from_string(meta.value("UUID", "")));
			metadata.set_visible(meta.value("Enabled", true));
			metadata.set_selected(meta.value("Selected", false));

			Entity entity = m_entity_manager->create_entity(metadata.get_name());
			uuid_to_entity[metadata.get_id().to_string()] = entity;
		}
	}

	// Second pass: Add all components
	for (auto &[idStr, entityData] : entities_json.items()) {
		const auto &meta = entityData["MetadataComponent"];
		std::string uuid_str = meta.value("UUID", "");
		Entity entity = uuid_to_entity.at(uuid_str);

		// Deserialize TransformComponent
		if (entityData.contains("TransformComponent")) {
			const auto &transform_json = entityData["TransformComponent"];
			Vec3 position =
				Vec3(transform_json["Position"][0], transform_json["Position"][1], transform_json["Position"][2]);
			Vec3 rotation =
				Vec3(transform_json["Rotation"][0], transform_json["Rotation"][1], transform_json["Rotation"][2]);
			Vec3 scale = Vec3(transform_json["Scale"][0], transform_json["Scale"][1], transform_json["Scale"][2]);

			Components::TransformComponent transform;
			transform.set_local_position(position);
			transform.set_local_rotation(rotation);
			transform.set_local_scale(scale);
			transform.update_world_matrix();

			entity.add_or_replace_component<Components::TransformComponent>(transform);
		}

		// Deserialize SceneNodeComponent
		if (entityData.contains("SceneNodeComponent")) {
			const auto &node_json = entityData["SceneNodeComponent"];
			Components::SceneNodeComponent node;
			node.ent = entity;

			if (std::string parent_uuid = node_json.value("Parent", "null"); parent_uuid == "null") {
				node.parent = Entity::null();
			} else {
				node.parent = uuid_to_entity.contains(parent_uuid) ? uuid_to_entity.at(parent_uuid) : Entity::null();
			}

			if (node_json.contains("Children")) {
				for (const auto &child_uuid_json : node_json["Children"]) {
					if (auto child_uuid = child_uuid_json.get<std::string>(); uuid_to_entity.contains(child_uuid)) {
						node.children.push_back(uuid_to_entity.at(child_uuid));
					}
				}
			}

			entity.add_or_replace_component<Components::SceneNodeComponent>(node);
		}

		// Deserialize MeshComponent
		if (entityData.contains("MeshComponent")) {
			const auto &mesh_json = entityData["MeshComponent"];
			auto &mesh_comp = entity.get_or_emplace<Components::MeshComponent>();

			if (std::string mesh_path = mesh_json.value("Path", ""); !mesh_path.empty()) {
				if (mesh_path.starts_with("procedural://")) {
					std::string type = mesh_path.substr(13);
					// meshComp.data = assetManager.CreateProceduralMesh(type);
				} else {
					// meshComp.data = assetManager.LoadMesh(meshPath);
				}
			}

			mesh_comp.cast_shadows = mesh_json.value("CastShadows", true);
		}

		// Deserialize LightComponent with shadow settings
		if (entityData.contains("LightComponent")) {
			const auto &light_json = entityData["LightComponent"];
			Components::LightComponent light;

			if (light_json.contains("Type")) {
				light.m_type = static_cast<Components::LightComponent::Type>(light_json["Type"].get<int>());
			}

			if (light_json.contains("Color")) {
				light.m_color = Vec3(light_json["Color"][0].get<F32>(), light_json["Color"][1].get<F32>(),
									 light_json["Color"][2].get<F32>());
			}

			if (light_json.contains("Intensity")) {
				light.m_intensity = light_json["Intensity"].get<F32>();
			}
			if (light_json.contains("Range")) {
				light.m_range = light_json["Range"].get<F32>();
			}
			if (light_json.contains("InnerConeAngle")) {
				light.m_inner_cone_angle = light_json["InnerConeAngle"].get<F32>();
			}
			if (light_json.contains("OuterConeAngle")) {
				light.m_outer_cone_angle = light_json["OuterConeAngle"].get<F32>();
			}

			if (light_json.contains("Direction")) {
				light.m_direction = Vec3(light_json["Direction"][0].get<F32>(), light_json["Direction"][1].get<F32>(),
										 light_json["Direction"][2].get<F32>());
			}

			if (light_json.contains("IsActive")) {
				light.m_is_active = light_json["IsActive"].get<bool>();
			}

			// Deserialize shadow settings for directional lights
			if (light.m_type == Components::LightComponent::Type::Directional &&
				light_json.contains("ShadowSettings")) {
				const auto &shadow_json = light_json["ShadowSettings"];

				light.m_shadow_settings.light_size = shadow_json.value("LightSize", 8.0f);
				light.m_shadow_settings.shadow_bias = shadow_json.value("ShadowBias", 0.0005f);
				light.m_shadow_settings.normal_bias = shadow_json.value("NormalBias", 1.0f);
				light.m_shadow_settings.pcf_samples = shadow_json.value("PCFSamples", 32);
				light.m_shadow_settings.cascade_split_lambda = shadow_json.value("CascadeSplitLambda", 0.95f);
				light.m_shadow_settings.blocker_search_samples = shadow_json.value("BlockerSearchSamples", 16);
			}

			entity.add_or_replace_component<Components::LightComponent>(light);
		}

		// Deserialize CameraComponent
		if (entityData.contains("CameraComponent")) {
			const auto &cam_json = entityData["CameraComponent"];
			Components::CameraComponent cam;

			cam.primary = cam_json.value("Primary", false);
			cam.is_orthographic = cam_json.value("IsOrthographic", false);
			cam.fov = cam_json.value("FOV", 45.0f);
			cam.aspect_ratio = cam_json.value("AspectRatio", 16.0f / 9.0f);
			cam.near_plane = cam_json.value("NearPlane", 0.1f);
			cam.far_plane = cam_json.value("FarPlane", 1000.0f);
			cam.ortho_left = cam_json.value("OrthoLeft", -10.0f);
			cam.ortho_right = cam_json.value("OrthoRight", 10.0f);
			cam.ortho_top = cam_json.value("OrthoTop", 10.0f);
			cam.ortho_bottom = cam_json.value("OrthoBottom", -10.0f);

			entity.add_or_replace_component<Components::CameraComponent>(cam);
		}

		// Deserialize SkyLightComponent
		if (entityData.contains("SkyLightComponent")) {
			const auto &sky_light_json = entityData["SkyLightComponent"];
			auto &sky_light = entity.get_or_emplace<Components::SkyLightComponent>();

			sky_light.set_active(sky_light_json.value("Active", true));
			sky_light.set_intensity(sky_light_json.value("Intensity", 1.0f));

			if (sky_light_json.contains("Tint")) {
				Vec3 tint = Vec3(sky_light_json["Tint"][0].get<F32>(), sky_light_json["Tint"][1].get<F32>(),
								 sky_light_json["Tint"][2].get<F32>());
				sky_light.set_tint(tint);
			}

			sky_light.set_render_skybox(sky_light_json.value("RenderSkybox", true));

			if (std::string hdr_path = sky_light_json.value("HDRTexturePath", ""); !hdr_path.empty()) {
				// if (auto hdrTexture = assetManager.LoadHDRTexture(hdrPath)) {
				// 	skyLight.SetHDRTexture(hdrTexture);
				// }
			}
		}

		// Deserialize MaterialComponent
		if (entityData.contains("MaterialComponent")) {
			const auto &mat_json = entityData["MaterialComponent"];
			auto &mat_comp = entity.get_or_emplace<Components::MaterialComponent>();
			mat_comp.type = static_cast<Graphics::MaterialType>(mat_json.value("Type", 0));
		}
	}

	return true;
}
} // namespace Aquila::SceneManagement
