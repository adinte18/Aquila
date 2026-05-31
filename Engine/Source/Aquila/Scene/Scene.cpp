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
	m_SceneID = Utils::UUID::Generate();
	OnStart();
}

Scene::Scene(const std::string &name) : m_SceneName(name) {
	m_SceneID = Utils::UUID::Generate();
	OnStart();
}

Scene::~Scene() = default;

/**
 * @brief Retrieves the entt registry associated with the scene.
 *
 * @return entt::registry& The enthe scene starts, setting up the necessary
 * components for managing entities and their relationships within the scene.
 */
void Scene::OnStart() {
	m_EntityManager = CreateUnique<EntityManager>(this);
	// Wire dirty callback on every TransformComponent that gets created.
	m_EntityManager->GetRegistry().on_construct<Components::TransformComponent>().connect<&Scene::OnTransformConstruct>(
		this);
	m_EntityManager->ConstructSceneGraph();
}

void Scene::OnTransformConstruct(entt::registry &registry, entt::entity e) {
	auto &t = registry.get<Components::TransformComponent>(e);
	t.SetDirtyCallback([this, e]() { MarkTransformDirty(e); });
	MarkTransformDirty(e); // bootstrap: new entity needs its first world matrix compute
}

void Scene::MarkTransformDirty(entt::entity entity) {
	m_DirtyTransforms.MarkDirty(entity);
}

bool Scene::HasDirtyAncestor(entt::entity e) const {
	Entity entity(e, const_cast<Scene *>(this));
	auto *node = entity.TryGetComponent<Components::SceneNodeComponent>();
	while (node && !node->Parent.IsNull()) {
		if (m_DirtyTransforms.IsDirty(node->Parent.GetHandle())) {
			return true;
		}
		node = node->Parent.TryGetComponent<Components::SceneNodeComponent>();
	}
	return false;
}

int Scene::GetEntityDepth(entt::entity e) const {
	int depth = 0;
	Entity entity(e, const_cast<Scene *>(this));
	auto *node = entity.TryGetComponent<Components::SceneNodeComponent>();
	while (node && !node->Parent.IsNull()) {
		depth++;
		node = node->Parent.TryGetComponent<Components::SceneNodeComponent>();
	}
	return depth;
}

/**
 * @brief Retrieves the entt registry associated with the scene.
 *
 * @return entt::registry& The entt registry for the scene, which contains all
 * entities and their components.
 */
entt::registry &Scene::GetRegistry() const {
	return m_EntityManager->GetRegistry();
}

/**
 * @brief Retrieves the EntityManager associated with the scene.
 *
 * @return EntityManager* Pointer to the EntityManager that manages entities in
 * the scene.
 */
EntityManager *Scene::GetEntityManager() const {
	return m_EntityManager.get();
}

/**
 * @brief Retrieves the name of the scene.
 *
 * @return const std::string& The name of the scene.
 */
const std::string &Scene::GetSceneName() const {
	return m_SceneName;
}

const Utils::UUID Scene::GetHandle() const {
	return m_SceneID;
}

void Scene::UpdateTransformHierarchy() {
	if (m_DirtyTransforms.IsEmpty()) {
		return;
	}

	// Sort dirty entities parent-first so ancestors are always processed before descendants.
	std::vector<entt::entity> sorted = m_DirtyTransforms.GetOrdered();
	std::stable_sort(sorted.begin(), sorted.end(),
					 [this](entt::entity a, entt::entity b) { return GetEntityDepth(a) < GetEntityDepth(b); });

	for (entt::entity e : sorted) {
		// If a dirty ancestor is also in the set, it was (or will be) processed first and its
		// UpdateTransformRecursive call already covers this entity — skip it.
		if (HasDirtyAncestor(e)) {
			continue;
		}

		Entity entity(e, this);
		glm::mat4 parentWorld(1.0f);
		auto *node = entity.TryGetComponent<Components::SceneNodeComponent>();
		if (node && !node->Parent.IsNull()) {
			auto *parentTransform = node->Parent.TryGetComponent<Components::TransformComponent>();
			if (parentTransform) {
				parentWorld = parentTransform->GetWorldMatrixLazy();
			}
		}
		UpdateTransformRecursive(entity, parentWorld);
	}

	m_DirtyTransforms.Clear();
}

void Scene::UpdateTransformRecursive(Entity entity, const glm::mat4 &parentWorld) {
	if (!entity.IsValid()) {
		return;
	}

	auto *transform = entity.TryGetComponent<Components::TransformComponent>();
	if (!transform) {
		return;
	}

	// Update this entity's world matrix
	transform->UpdateWorldMatrix(parentWorld);

	// Recursively update all children
	if (auto *node = entity.TryGetComponent<Components::SceneNodeComponent>()) {
		for (auto child : node->Children) {
			if (child.IsValid()) {
				UpdateTransformRecursive(child, transform->GetWorldMatrix());
			}
		}
	}
}

void Scene::SetActiveCamera(Entity cameraEntity) {
	if (cameraEntity.IsValid() && cameraEntity.HasComponent<Components::CameraComponent>()) {
		// Store the raw entt::entity handle
		m_ActiveCameraEntity = cameraEntity.GetHandle();
	}
}

Entity Scene::GetActiveCameraEntity() const {
	// Wrap the entt::entity in an Entity object
	if (m_ActiveCameraEntity != entt::null) {
		return Entity(m_ActiveCameraEntity, const_cast<Scene *>(this));
	}
	return Entity(); // Return invalid entity
}

bool Scene::HasActiveCamera() const {
	if (m_ActiveCameraEntity == entt::null) {
		return false;
	}

	// Check if entity still exists and has camera component
	return GetRegistry().valid(m_ActiveCameraEntity) &&
		GetRegistry().all_of<Components::CameraComponent>(m_ActiveCameraEntity);
}

Entity Scene::FindPrimaryCamera() const {
	// Iterate through all entities with CameraComponent
	auto view = GetRegistry().view<Components::CameraComponent>();

	for (auto entity : view) {
		const auto &cam = view.get<Components::CameraComponent>(entity);
		if (cam.primary) {
			return Entity(entity, const_cast<Scene *>(this));
		}
	}
	return Entity(); // Return invalid entity if no primary camera found
}

bool Scene::Serialize(const std::string &filepath) {
	nlohmann::ordered_json sceneJson;

	sceneJson["SceneName"] = m_SceneName;

	auto &registry = GetRegistry();
	sceneJson["Entities"] = nlohmann::ordered_json::object();

	auto view = registry.view<Components::MetadataComponent>();

	for (auto entityHandle : view) {
		Entity entity(entityHandle, this);
		nlohmann::ordered_json entityJson;

		// Serialize MetadataComponent
		if (entity.HasComponent<Components::MetadataComponent>()) {
			auto &meta = entity.GetComponent<Components::MetadataComponent>();
			entityJson["MetadataComponent"] = { { "Name", meta.GetName() },
												{ "UUID", meta.GetId().ToString() },
												{ "Enabled", meta.IsVisible() },
												{ "Selected", meta.IsSelected() } };
		}

		// Serialize TransformComponent
		if (entity.HasComponent<Components::TransformComponent>()) {
			auto &transform = entity.GetComponent<Components::TransformComponent>();
			entityJson["TransformComponent"] = {
				{ "Position",
				  { transform.GetLocalPosition().x, transform.GetLocalPosition().y, transform.GetLocalPosition().z } },
				{ "Rotation",
				  { transform.GetLocalRotation().x, transform.GetLocalRotation().y, transform.GetLocalRotation().z } },
				{ "Scale", { transform.GetLocalScale().x, transform.GetLocalScale().y, transform.GetLocalScale().z } }
			};
		}

		// Serialize SceneNodeComponent
		if (entity.HasComponent<Components::SceneNodeComponent>()) {
			auto &node = entity.GetComponent<Components::SceneNodeComponent>();
			entityJson["SceneNodeComponent"] = {
				{ "Parent",
				  node.Parent.IsNull() ? "null"
									   : node.Parent.GetComponent<Components::MetadataComponent>().GetId().ToString() },
				{ "Children", nlohmann::ordered_json::array() }
			};

			for (auto &child : node.Children) {
				if (!child.IsNull() && child.HasComponent<Components::MetadataComponent>()) {
					entityJson["SceneNodeComponent"]["Children"].push_back(
						child.GetComponent<Components::MetadataComponent>().GetId().ToString());
				}
			}
		}

		// Serialize MeshComponent
		if (entity.HasComponent<Components::MeshComponent>()) {
			auto &mesh = entity.GetComponent<Components::MeshComponent>();
			entityJson["MeshComponent"] = { { "Path", mesh.data->GetPath() },
											{ "DebugName", mesh.data->GetDebugName() },
											{ "CastShadows", mesh.castShadows } };
		}

		// Serialize LightComponent with shadow settings
		if (entity.HasComponent<Components::LightComponent>()) {
			auto &light = entity.GetComponent<Components::LightComponent>();

			entityJson["LightComponent"] = { { "Type", static_cast<int>(light.m_Type) },
											 { "Color", { light.m_Color.r, light.m_Color.g, light.m_Color.b } },
											 { "Intensity", light.m_Intensity },
											 { "Range", light.m_Range },
											 { "InnerConeAngle", light.m_InnerConeAngle },
											 { "OuterConeAngle", light.m_OuterConeAngle },
											 { "Direction",
											   { light.m_Direction.x, light.m_Direction.y, light.m_Direction.z } },
											 { "IsActive", light.m_IsActive } };

			// Serialize shadow settings for directional lights
			if (light.m_Type == Components::LightComponent::Type::Directional) {
				const auto &shadowSettings = light.m_ShadowSettings;
				entityJson["LightComponent"]["ShadowSettings"] = {
					{ "LightSize", shadowSettings.lightSize },
					{ "ShadowBias", shadowSettings.shadowBias },
					{ "NormalBias", shadowSettings.normalBias },
					{ "PCFSamples", shadowSettings.pcfSamples },
					{ "CascadeSplitLambda", shadowSettings.cascadeSplitLambda },
					{ "BlockerSearchSamples", shadowSettings.blockerSearchSamples }
				};
			}
		}

		// Serialize CameraComponent
		if (entity.HasComponent<Components::CameraComponent>()) {
			auto &cam = entity.GetComponent<Components::CameraComponent>();
			entityJson["CameraComponent"] = { { "Primary", cam.primary },
											  { "IsOrthographic", cam.isOrthographic },
											  { "FOV", cam.fov },
											  { "AspectRatio", cam.aspectRatio },
											  { "NearPlane", cam.nearPlane },
											  { "FarPlane", cam.farPlane },
											  { "OrthoLeft", cam.orthoLeft },
											  { "OrthoRight", cam.orthoRight },
											  { "OrthoTop", cam.orthoTop },
											  { "OrthoBottom", cam.orthoBottom } };
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
		if (entity.HasComponent<Components::MaterialComponent>()) {
			auto &matComp = entity.GetComponent<Components::MaterialComponent>();
			entityJson["MaterialComponent"] = {
				{ "Type", static_cast<int>(matComp.type) },
			};
		}

		sceneJson["Entities"][std::to_string(static_cast<int>(entityHandle))] = entityJson;
	}

	const auto vfsFile =
		Aquila::Platform::Filesystem::VirtualFileSystem::Get()->OpenFile(filepath, AccessMode::Write, OpenMode::Binary);
	if (!vfsFile->IsValid()) {
		return false;
	}

	vfsFile->Write(sceneJson.dump(4).data(), sceneJson.dump(4).size());
	vfsFile->Close();

	return true;
}

bool Scene::Deserialize(const std::string &filepath, Assets::AssetManager &assetManager) {
	AQUILA_ASSERT(m_EntityManager != nullptr, "EntityManager is nullptr");

	auto vfsFile =
		Aquila::Platform::Filesystem::VirtualFileSystem::Get()->OpenFile(filepath, AccessMode::Read, OpenMode::Binary);
	if (!vfsFile->IsValid()) {
		return false;
	}

	std::vector<char> buffer(vfsFile->Size());
	vfsFile->Read(buffer.data(), buffer.size());
	vfsFile->Close();

	nlohmann::ordered_json sceneJson;
	sceneJson = nlohmann::ordered_json::parse(buffer.begin(), buffer.end());

	m_SceneName = sceneJson.value("SceneName", "Untitled Scene");

	auto &registry = GetRegistry();
	registry.clear();

	if (!sceneJson.contains("Entities")) {
		return false;
	}

	const auto &entitiesJson = sceneJson["Entities"];

	std::unordered_map<std::string, Entity> uuidToEntity;

	// First pass: Create all entities with metadata
	for (auto &[idStr, entityData] : entitiesJson.items()) {
		if (entityData.contains("MetadataComponent")) {
			const auto &meta = entityData["MetadataComponent"];
			Components::MetadataComponent metadata;
			metadata.SetName(meta.value("Name", ""));
			metadata.SetId(Utils::UUID::FromString(meta.value("UUID", "")));
			metadata.SetVisible(meta.value("Enabled", true));
			metadata.SetSelected(meta.value("Selected", false));

			Entity entity = m_EntityManager->CreateEntity(metadata.GetName());
			uuidToEntity[metadata.GetId().ToString()] = entity;
		}
	}

	// Second pass: Add all components
	for (auto &[idStr, entityData] : entitiesJson.items()) {
		const auto &meta = entityData["MetadataComponent"];
		std::string uuidStr = meta.value("UUID", "");
		Entity entity = uuidToEntity.at(uuidStr);

		// Deserialize TransformComponent
		if (entityData.contains("TransformComponent")) {
			const auto &transformJson = entityData["TransformComponent"];
			vec3 position =
				vec3(transformJson["Position"][0], transformJson["Position"][1], transformJson["Position"][2]);
			vec3 rotation =
				vec3(transformJson["Rotation"][0], transformJson["Rotation"][1], transformJson["Rotation"][2]);
			vec3 scale = vec3(transformJson["Scale"][0], transformJson["Scale"][1], transformJson["Scale"][2]);

			Components::TransformComponent transform;
			transform.SetLocalPosition(position);
			transform.SetLocalRotation(rotation);
			transform.SetLocalScale(scale);
			transform.UpdateWorldMatrix();

			entity.AddOrReplaceComponent<Components::TransformComponent>(transform);
		}

		// Deserialize SceneNodeComponent
		if (entityData.contains("SceneNodeComponent")) {
			const auto &nodeJson = entityData["SceneNodeComponent"];
			Components::SceneNodeComponent node;
			node.Ent = entity;

			if (std::string parentUUID = nodeJson.value("Parent", "null"); parentUUID == "null") {
				node.Parent = Entity::Null();
			} else {
				node.Parent = uuidToEntity.contains(parentUUID) ? uuidToEntity.at(parentUUID) : Entity::Null();
			}

			if (nodeJson.contains("Children")) {
				for (const auto &childUUIDJson : nodeJson["Children"]) {
					if (auto childUUID = childUUIDJson.get<std::string>(); uuidToEntity.contains(childUUID)) {
						node.Children.push_back(uuidToEntity.at(childUUID));
					}
				}
			}

			entity.AddOrReplaceComponent<Components::SceneNodeComponent>(node);
		}

		// Deserialize MeshComponent
		if (entityData.contains("MeshComponent")) {
			const auto &meshJson = entityData["MeshComponent"];
			auto &meshComp = entity.GetOrEmplace<Components::MeshComponent>();

			if (std::string meshPath = meshJson.value("Path", ""); !meshPath.empty()) {
				if (meshPath.starts_with("procedural://")) {
					std::string type = meshPath.substr(13);
					// meshComp.data = assetManager.CreateProceduralMesh(type);
				} else {
					// meshComp.data = assetManager.LoadMesh(meshPath);
				}
			}

			meshComp.castShadows = meshJson.value("CastShadows", true);
		}

		// Deserialize LightComponent with shadow settings
		if (entityData.contains("LightComponent")) {
			const auto &lightJson = entityData["LightComponent"];
			Components::LightComponent light;

			if (lightJson.contains("Type")) {
				light.m_Type = static_cast<Components::LightComponent::Type>(lightJson["Type"].get<int>());
			}

			if (lightJson.contains("Color")) {
				light.m_Color = vec3(lightJson["Color"][0].get<f32>(), lightJson["Color"][1].get<f32>(),
									 lightJson["Color"][2].get<f32>());
			}

			if (lightJson.contains("Intensity")) {
				light.m_Intensity = lightJson["Intensity"].get<f32>();
			}
			if (lightJson.contains("Range")) {
				light.m_Range = lightJson["Range"].get<f32>();
			}
			if (lightJson.contains("InnerConeAngle")) {
				light.m_InnerConeAngle = lightJson["InnerConeAngle"].get<f32>();
			}
			if (lightJson.contains("OuterConeAngle")) {
				light.m_OuterConeAngle = lightJson["OuterConeAngle"].get<f32>();
			}

			if (lightJson.contains("Direction")) {
				light.m_Direction = vec3(lightJson["Direction"][0].get<f32>(), lightJson["Direction"][1].get<f32>(),
										 lightJson["Direction"][2].get<f32>());
			}

			if (lightJson.contains("IsActive")) {
				light.m_IsActive = lightJson["IsActive"].get<bool>();
			}

			// Deserialize shadow settings for directional lights
			if (light.m_Type == Components::LightComponent::Type::Directional && lightJson.contains("ShadowSettings")) {
				const auto &shadowJson = lightJson["ShadowSettings"];

				light.m_ShadowSettings.lightSize = shadowJson.value("LightSize", 8.0f);
				light.m_ShadowSettings.shadowBias = shadowJson.value("ShadowBias", 0.0005f);
				light.m_ShadowSettings.normalBias = shadowJson.value("NormalBias", 1.0f);
				light.m_ShadowSettings.pcfSamples = shadowJson.value("PCFSamples", 32);
				light.m_ShadowSettings.cascadeSplitLambda = shadowJson.value("CascadeSplitLambda", 0.95f);
				light.m_ShadowSettings.blockerSearchSamples = shadowJson.value("BlockerSearchSamples", 16);
			}

			entity.AddOrReplaceComponent<Components::LightComponent>(light);
		}

		// Deserialize CameraComponent
		if (entityData.contains("CameraComponent")) {
			const auto &camJson = entityData["CameraComponent"];
			Components::CameraComponent cam;

			cam.primary = camJson.value("Primary", false);
			cam.isOrthographic = camJson.value("IsOrthographic", false);
			cam.fov = camJson.value("FOV", 45.0f);
			cam.aspectRatio = camJson.value("AspectRatio", 16.0f / 9.0f);
			cam.nearPlane = camJson.value("NearPlane", 0.1f);
			cam.farPlane = camJson.value("FarPlane", 1000.0f);
			cam.orthoLeft = camJson.value("OrthoLeft", -10.0f);
			cam.orthoRight = camJson.value("OrthoRight", 10.0f);
			cam.orthoTop = camJson.value("OrthoTop", 10.0f);
			cam.orthoBottom = camJson.value("OrthoBottom", -10.0f);

			entity.AddOrReplaceComponent<Components::CameraComponent>(cam);
		}

		// Deserialize SkyLightComponent
		if (entityData.contains("SkyLightComponent")) {
			const auto &skyLightJson = entityData["SkyLightComponent"];
			auto &skyLight = entity.GetOrEmplace<Components::SkyLightComponent>();

			skyLight.SetActive(skyLightJson.value("Active", true));
			skyLight.SetIntensity(skyLightJson.value("Intensity", 1.0f));

			if (skyLightJson.contains("Tint")) {
				vec3 tint = vec3(skyLightJson["Tint"][0].get<f32>(), skyLightJson["Tint"][1].get<f32>(),
								 skyLightJson["Tint"][2].get<f32>());
				skyLight.SetTint(tint);
			}

			skyLight.SetRenderSkybox(skyLightJson.value("RenderSkybox", true));

			if (std::string hdrPath = skyLightJson.value("HDRTexturePath", ""); !hdrPath.empty()) {
				// if (auto hdrTexture = assetManager.LoadHDRTexture(hdrPath)) {
				// 	skyLight.SetHDRTexture(hdrTexture);
				// }
			}
		}

		// Deserialize MaterialComponent
		if (entityData.contains("MaterialComponent")) {
			const auto &matJson = entityData["MaterialComponent"];
			auto &matComp = entity.GetOrEmplace<Components::MaterialComponent>();
			matComp.type = static_cast<Graphics::MaterialType>(matJson.value("Type", 0));
		}
	}

	return true;
}
} // namespace Aquila::SceneManagement
