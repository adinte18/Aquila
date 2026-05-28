#include "Aquila/Scene/EntityManager.h"
#include "Aquila/Scene/Components/MaterialComponent.h"
#include "Aquila/Scene/Components/MetadataComponent.h"
#include "Aquila/Scene/Components/SceneNodeComponent.h"
#include "Aquila/Scene/Components/TransformComponent.h"
#include "Aquila/Scene/Scene.h"

namespace Aquila::SceneManagement {

EntityManager::~EntityManager() = default;

entt::registry &EntityManager::GetRegistry() {
	AQUILA_ASSERT(m_Scene, "Scene should not be nullptr");
	return m_Registry;
}

void EntityManager::QueueForKill(Entity entity) {
	if (!entity.IsValid()) {
		return;
	}
	m_DeletionQueue.emplace_back(entity.GetHandle());
}

void EntityManager::FlushDeletionQueue() {
	for (auto &entityHandle : m_DeletionQueue) {
		Entity entity{ entityHandle, m_Scene };
		if (!entity.IsValid()) {
			continue;
		}

		RemoveAllChildren(entity);

		entity.Kill();
	}

	m_DeletionQueue.clear();
}

bool EntityManager::IsRegistryEmpty() {
	return m_Registry.storage<entt::entity>().empty();
}

std::string EntityManager::GetDefaultName(EntityPreset preset) {
	switch (preset) {
	case EntityPreset::Empty:
		return "Empty Entity";
	case EntityPreset::Cube:
		return "Cube";
	case EntityPreset::Sphere:
		return "Sphere";
	case EntityPreset::Cylinder:
		return "Cylinder";
	case EntityPreset::Plane:
		return "Plane";
	case EntityPreset::DirectionalLight:
		return "Directional Light";
	case EntityPreset::PointLight:
		return "Point Light";
	case EntityPreset::SpotLight:
		return "Spot Light";
	case EntityPreset::PerspectiveCamera:
		return "Camera";
	case EntityPreset::OrthographicCamera:
		return "Orthographic Camera";
	default:
		return "Unknown Entity";
	}
}

Entity EntityManager::CreateEntity(const std::string &name) {
	AQUILA_ASSERT(m_Scene, "Scene should not be nullptr");

	Entity entity{ m_Registry.create(), m_Scene };

	std::string uniqueName = GenerateUniqueName(name);

	entity.AddComponent<Components::MetadataComponent>(Utils::UUID::Generate(), uniqueName, true);
	entity.AddComponent<Components::SceneNodeComponent>();
	entity.AddComponent<Components::TransformComponent>();

	return entity;
}

void EntityManager::DestroyEntity(Entity entity) {
	if (!entity.IsValid()) {
		return;
	}

	RemoveAllChildren(entity);

	entity.Kill();
}

bool EntityManager::IsValid(Entity entity) const {
	return entity.IsValid();
}

std::string EntityManager::GenerateUniqueName(const std::string &baseName) {
	auto entities = GetAllWith<Components::MetadataComponent>();

	std::unordered_set<std::string> existingNames;
	for (auto entity : entities) {
		auto &metadata = entity.GetComponent<Components::MetadataComponent>();
		existingNames.insert(metadata.GetName());
	}

	if (existingNames.find(baseName) == existingNames.end()) {
		return baseName;
	}

	int counter = 1;
	std::string candidateName;
	do {
		candidateName = baseName + " (" + std::to_string(counter) + ")";
		counter++;
	} while (existingNames.find(candidateName) != existingNames.end());

	return candidateName;
}

bool EntityManager::Exists(const Utils::UUID &uuid) {
	auto entities = GetAllWith<Components::MetadataComponent>();

	for (auto entity : entities) {
		auto &metadata = entity.GetComponent<Components::MetadataComponent>();
		if (metadata.GetId() == uuid) {
			return true;
		}
	}

	return false;
}

std::optional<Entity> EntityManager::FindEntityByUUID(const Utils::UUID &uuid) {
	auto entities = GetAllWith<Components::MetadataComponent>();

	for (auto entity : entities) {
		auto &metadata = entity.GetComponent<Components::MetadataComponent>();
		if (metadata.GetId() == uuid) {
			return entity;
		}
	}

	return std::nullopt;
}

std::optional<Entity> EntityManager::FindEntityByName(const std::string &name) {
	auto entities = GetAllWith<Components::MetadataComponent>();

	for (auto entity : entities) {
		auto &metadata = entity.GetComponent<Components::MetadataComponent>();
		if (metadata.GetName() == name) {
			return entity;
		}
	}

	return std::nullopt;
}

void EntityManager::Clear() {
	m_DeletionQueue.clear();

	for (auto [entity] : m_Registry.storage<entt::entity>().each()) {
		m_Registry.destroy(entity);
	}

	m_Registry.clear();
}

/**
 * @brief Constructs the scene graph by connecting callbacks to
 * SceneNodeComponent events.
 *
 * This function is called to set up the scene graph, allowing it to respond to
 * construction, update, and destruction of SceneNodeComponents in the entt
 * registry.
 */
void EntityManager::ConstructSceneGraph() {
	// Connect callbacks to SceneNodeComponent events
	m_Registry.on_construct<Components::SceneNodeComponent>().connect<&EntityManager::OnSceneNodeConstruct>(*this);
	m_Registry.on_destroy<Components::SceneNodeComponent>().connect<&EntityManager::OnSceneNodeDestroy>(*this);
}

/**
 * @brief Constructs a scene node and sets its parent-child relationship in the
 * scene graph.
 *
 * This function is called when a SceneNodeComponent is constructed in the
 * registry. It initializes the SceneNodeComponent and establishes
 * the parent-child relationship if a parent is specified.
 *
 * @param registry The entt registry containing the scene graph.
 * @param entityHandle The entity being constructed.
 */
void EntityManager::OnSceneNodeConstruct(entt::registry &registry, entt::entity entityHandle) {
	Entity entity(entityHandle, m_Scene);

	auto &node = entity.GetOrEmplace<Components::SceneNodeComponent>();
	node.Ent = entity;

	if (!node.Parent.IsNull()) {
		auto &parentNode = m_Registry.get_or_emplace<Components::SceneNodeComponent>(node.Parent.GetHandle());

		auto &siblings = parentNode.Children;
		if (std::find(siblings.begin(), siblings.end(), entity) == siblings.end()) {
			siblings.push_back(entity);
		}
	}
}

void EntityManager::OnSceneNodeDestroy(entt::registry &registry, entt::entity entityHandle) {
	Entity entity(entityHandle, m_Scene);

	auto *node = entity.TryGetComponent<Components::SceneNodeComponent>();
	if (!node) {
		return;
	}

	for (auto child : node->Children) {
		if (child.IsValid()) {
			QueueForKill(child);
		}
	}
	node->Children.clear();

	if (!node->Parent.IsNull()) {
		auto *parentNode = node->Parent.TryGetComponent<Components::SceneNodeComponent>();
		if (parentNode) {
			auto &siblings = parentNode->Children;
			if (!siblings.empty()) {
				siblings.erase(std::remove(siblings.begin(), siblings.end(), entity), siblings.end());
			}
		}
	}
}

/**
 * @brief Adds a child entity to a parent in the scene graph.
 *
 * @param parent The parent entity to which the child will be added.
 * @param child The child entity to be added.
 */
void EntityManager::AddChild(Entity parent, Entity child) {
	if (!parent.IsValid() || !child.IsValid()) {
		return;
	}

	auto *parentNode = parent.TryGetComponent<Components::SceneNodeComponent>();
	auto *childNode = child.TryGetComponent<Components::SceneNodeComponent>();
	if (!parentNode || !childNode) {
		return;
	}

	childNode->Parent = parentNode->Ent;

	if (std::find(parentNode->Children.begin(), parentNode->Children.end(), child) == parentNode->Children.end()) {
		parentNode->Children.push_back(child);
	}
}

/**
 * @brief Attaches a node to a parent in the scene graph.
 *
 * @param parent The parent entity to which the node will be attached.
 * @param node The node entity to be attached.
 */
void EntityManager::AttachTo(Entity parent, Entity node) {
	if (!parent.IsValid() || !node.IsValid()) {
		return;
	}

	auto *parentNode = parent.TryGetComponent<Components::SceneNodeComponent>();
	auto *nodeToAttach = node.TryGetComponent<Components::SceneNodeComponent>();
	if (!parentNode || !nodeToAttach) {
		return;
	}

	auto *nodeTransform = node.TryGetComponent<Components::TransformComponent>();
	auto *parentTransform = parent.TryGetComponent<Components::TransformComponent>();
	if (!nodeTransform || !parentTransform) {
		return;
	}

	// Store the current world transform before reparenting
	vec3 worldPos = nodeTransform->GetWorldPosition();
	glm::quat worldRot = nodeTransform->GetWorldRotation();
	vec3 worldScale = nodeTransform->GetWorldScale();

	// if has parent -> detach from parent
	if (nodeToAttach->Parent.GetHandle() != entt::null) {
		RemoveChild(nodeToAttach->Parent, nodeToAttach->Ent);
	}

	// reattach to another parent
	nodeToAttach->Parent = parentNode->Ent;
	parentNode->Children.push_back(nodeToAttach->Ent);

	// Convert world transform to local space relative to new parent
	mat4 parentWorld = parentTransform->GetWorldMatrixLazy();
	mat4 parentInverse = inverse(parentWorld);

	// Calculate local position
	vec4 localPos4 = parentInverse * vec4(worldPos, 1.0f);
	vec3 localPos = vec3(localPos4);

	// Calculate local rotation
	glm::quat parentWorldRot = parentTransform->GetWorldRotation();
	glm::quat localRot = inverse(parentWorldRot) * worldRot;

	// Calculate local scale
	vec3 parentWorldScale = parentTransform->GetWorldScale();
	vec3 localScale = worldScale / parentWorldScale;

	// Apply the new local transform
	nodeTransform->SetLocalPosition(localPos);
	nodeTransform->SetLocalRotation(localRot);
	nodeTransform->SetLocalScale(localScale);
}

/**
 * @brief Checks if a given entity is a descendant of a potential parent in the
 * scene graph.
 *
 * @param potentialParent The entity that is being checked as a potential
 * parent.
 * @param entityToCheck The entity that is being checked for being a descendant.
 * @return true if entityToCheck is a descendant of potentialParent, false
 * otherwise.
 */
bool EntityManager::IsDescendant(Entity potentialParent, Entity entityToCheck) {
	auto *node = potentialParent.TryGetComponent<Components::SceneNodeComponent>();
	if (!node) {
		return false;
	}

	for (const auto &child : node->Children) {
		if (child == entityToCheck) {
			return true;
		}
		if (IsDescendant(child, entityToCheck)) {
			return true;
		}
	}
	return false;
}

/**
 * @brief Removes a child entity from its parent in the scene graph.
 *
 * @param parent The parent entity from which the child will be removed.
 * @param child The child entity to be removed.
 */
void EntityManager::RemoveChild(Entity parent, Entity child) {
	if (!parent.IsValid() || !child.IsValid()) {
		return;
	}

	auto *parentNode = parent.TryGetComponent<Components::SceneNodeComponent>();
	auto *childNode = child.TryGetComponent<Components::SceneNodeComponent>();
	if (!parentNode || !childNode) {
		return;
	}

	auto &siblings = parentNode->Children;
	siblings.erase(std::remove(siblings.begin(), siblings.end(), child), siblings.end());

	childNode->Parent = Entity::Null();
}

/**
 * @brief Recursively removes all children of a given parent entity.
 *
 * @param parent The parent entity whose children will be removed.
 */
void EntityManager::RemoveAllChildren(Entity parent) {
	if (!parent.IsValid()) {
		return;
	}

	auto *parentNode = parent.TryGetComponent<Components::SceneNodeComponent>();
	if (parentNode == nullptr) {
		return;
	}

	auto childrenCopy = parentNode->Children;

	for (auto &child : childrenCopy) {
		RemoveAllChildren(child);

		if (child.IsValid()) {
			QueueForKill(child);
		}
	}

	parentNode->Children.clear();
}

} // namespace Aquila::SceneManagement
