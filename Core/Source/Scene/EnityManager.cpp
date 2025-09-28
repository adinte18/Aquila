#include "Engine/Controller.h"
#include "Engine/Mesh.h"
#include "Scene/Components/CameraComponent.h"
#include "Scene/Components/LightComponent.h"
#include "Scene/Components/MeshComponent.h"
#include "Scene/Components/MetadataComponent.h"
#include "Scene/Components/SceneNodeComponent.h"
#include "Scene/Components/TransformComponent.h"
#include "Scene/EntityManager.h"
#include "Scene/Scene.h"
#include "Scene/SceneGraph.h"

namespace Engine {
EntityManager::~EntityManager() = default;

/**
 * @brief Initializes the EntityManager with a given scene.
 *
 * @param scene The scene to be managed by this EntityManager.
 */
entt::registry &EntityManager::GetRegistry() {
  AQUILA_ASSERT(m_Scene, "Scene should not be nullptr");

  return m_Registry;
}

void EntityManager::QueueForKill(entt::entity handle) {
  m_DeletionQueue.emplace_back(handle);
}

void EntityManager::FlushScene() {
  for (auto &entityHandle : m_DeletionQueue) {
    auto entity = Entity{entityHandle, m_Scene};

    auto &node = entity.GetComponent<SceneNodeComponent>();

    if (!node.Children.empty())
      m_Scene->GetSceneGraph()->RemoveAllChildren(m_Scene->GetRegistry(),
                                                  node.Entity);

    entity.Kill();
  }

  m_DeletionQueue.clear();
}

void EntityManager::ApplyPreset(Entity &entity, EntityPreset preset) {
  switch (preset) {
  case EntityPreset::Empty:
    break;

  case EntityPreset::Cube: {
    auto &component = entity.AddComponent<MeshComponent>();

    auto mesh = CreateRef<Mesh>(Engine::Controller::Get()->GetDevice(),
                                "procedural_cube");
    auto meshData = mesh->GenerateCube(1.f);
    mesh->LoadFromData(meshData);

    component.data = mesh;

    break;
  }
  case EntityPreset::Sphere: {
    auto &component = entity.AddComponent<MeshComponent>();

    auto mesh = CreateRef<Mesh>(Engine::Controller::Get()->GetDevice(),
                                "procedural_sphere");
    auto meshData = mesh->GenerateSphere();
    mesh->LoadFromData(meshData);

    component.data = mesh;

    break;
  }

  case EntityPreset::Cylinder: {
    auto &component = entity.AddComponent<MeshComponent>();

    auto mesh = CreateRef<Mesh>(Engine::Controller::Get()->GetDevice(),
                                "procedural_cylinder");
    auto meshData = mesh->GenerateCylinder();
    mesh->LoadFromData(meshData);

    component.data = mesh;

    break;
  }
  case EntityPreset::Plane: {
    auto &component = entity.AddComponent<MeshComponent>();

    auto mesh = CreateRef<Mesh>(Engine::Controller::Get()->GetDevice(),
                                "procedural_plane");
    auto meshData = mesh->GeneratePlane();
    mesh->LoadFromData(meshData);

    component.data = mesh;

    break;
  }
  // Lights
  case EntityPreset::DirectionalLight: {
    auto &component = entity.AddComponent<LightComponent>();
    component.m_Type = LightComponent::Type::Directional;
    break;
  }
  case EntityPreset::PointLight: {
    auto &component = entity.AddComponent<LightComponent>();
    component.m_Type = LightComponent::Type::Point;
    break;
  }

  case EntityPreset::SpotLight: {
    auto &component = entity.AddComponent<LightComponent>();
    component.m_Type = LightComponent::Type::Spot;
    break;
  }
  // Cameras
  case EntityPreset::PerspectiveCamera: {
    auto &component = entity.AddComponent<CameraComponent>();
    component.isOrthographic = false;

    break;
  }

  case EntityPreset::OrthographicCamera: {
    auto &component = entity.AddComponent<CameraComponent>();
    component.isOrthographic = true;

    break;
  }
  default:
    AQUILA_LOG_WARNING("Unknown EntityPreset");
    break;
  }
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

Entity EntityManager::AddEntity(const std::string &name) {
  AQUILA_ASSERT(m_Scene, "Scene should not be nullptr");

  auto entity = Entity{m_Registry.create(), m_Scene};

  std::string uniqueName = GenerateUniqueName(name);

  entity.AddComponent<MetadataComponent>(Utility::UUID::Generate(), uniqueName,
                                         true);
  entity.AddComponent<SceneNodeComponent>();
  entity.AddComponent<TransformComponent>();

  return entity;
}

std::string EntityManager::GenerateUniqueName(const std::string &baseName) {
  auto entities = GetAllWith<MetadataComponent>();

  std::unordered_set<std::string> existingNames;
  for (auto entity : entities) {
    auto &metadata = m_Registry.get<MetadataComponent>(entity.GetHandle());
    existingNames.insert(metadata.Name);
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
/**
 * @brief Verify if entity exists in the scene
 *
 * @param uuid
 * @return true if exsists
 * @return false if not
 */

bool EntityManager::EntityExists(const Utility::UUID &uuid) {
  auto entities = GetAllWith<MetadataComponent>();

  for (auto entity : entities) {
    auto &metadata = m_Registry.get<MetadataComponent>(entity.GetHandle());
    if (metadata.ID == uuid) {
      return true;
    }
  }

  return false;
}

std::optional<Entity> EntityManager::FindEntityByName(const std::string &name) {
  auto entities = GetAllWith<MetadataComponent>();

  for (auto entity : entities) {
    auto &metadata = m_Registry.get<MetadataComponent>(entity.GetHandle());
    if (metadata.Name == name) {
      return entity;
    }
  }

  return std::nullopt;
}
/**
 * @brief Clears all entities from the scene.
 *
 * This function destroys all entities in the registry and clears the registry.
 */
void EntityManager::Clear() {
  for (auto [entity] : m_Registry.storage<entt::entity>().each()) {
    m_Registry.destroy(entity);
  }

  m_Registry.clear();
}

/**
 * @brief Get entity by his UUID
 *
 * @param uuid UUID of the entity to find
 * @return Entity Entity with the given UUID
 */
Entity EntityManager::GetEntityByUUID(Utility::UUID uuid) {
  AQUILA_ASSERT(m_Scene, "There should be an active scene");

  auto view = m_Registry.view<MetadataComponent>(); // get all entities with
                                                    // metadata component

  for (auto &entity : view) {
    auto idComponent = m_Registry.get<MetadataComponent>(entity);
    if (idComponent.ID == uuid) {
      AQUILA_ASSERT(m_Scene, "There should be an active scene");
      return Entity(entity, m_Scene);
    }
  }

  return Entity();
}

/**
 * @brief Kill a given entity
 *
 * @param entity
 */
void EntityManager::KillEntity(Entity entity) {
  AQUILA_ASSERT(m_Scene, "Scene should not be nullptr");
  m_Registry.destroy(entity.GetHandle());
}

/**
 * @brief Verify if the entity is valid
 *
 * @param entity
 * @return true if valid
 * @return false if not
 */
bool EntityManager::IsEntityValid(Entity entity) const {
  AQUILA_ASSERT(m_Scene, "Scene should not be nullptr");
  return m_Registry.valid(entity.GetHandle());
}
} // namespace Engine