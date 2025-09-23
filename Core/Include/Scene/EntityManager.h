#ifndef AQUILA_ENTITY_MNG_H
#define AQUILA_ENTITY_MNG_H

#include "Entity.h"
#include "Scene.h"

namespace Engine {
class EntityManager {
public:
  EntityManager(AquilaScene *scene) : m_Scene(scene) { m_Registry = {}; }
  ~EntityManager();

  entt::registry &GetRegistry();

  template <typename... Components> Entity GetFirstEntityWith() {
    AQUILA_ASSERT(m_Scene, "There should be an active scene");
    auto view = m_Registry.view<Components...>();

    if (view.begin() == view.end()) {
      return Entity{};
    }

    entt::entity firstEntity = *view.begin();
    return Entity{firstEntity, m_Scene};
  }

  template <typename... Components> std::vector<Entity> GetAllWith() {
    AQUILA_ASSERT(m_Scene, "There should be an active scene");
    auto view = m_Registry.view<Components...>();

    std::vector<Entity> result{};
    result.reserve(std::distance(view.begin(), view.end()));

    for (auto entity : view) {
      result.emplace(result.begin(), Entity{entity, m_Scene});
    }

    return result;
  }

  Entity AddEntity(const std::string &name);
  Entity GetEntityByUUID(Utility::UUID id);

  std::string GenerateUniqueName(const std::string &baseName);
  std::optional<Entity> FindEntityByName(const std::string &name);

  void DeleteEntity();
  void Clear();

  void ApplyPreset(Entity &entity, EntityPreset preset);
  std::string GetDefaultName(EntityPreset preset);
  bool EntityExists(const Utility::UUID &uuid);
  void KillEntity(Entity entity);
  bool IsEntityValid(Entity entity) const;

  void QueueForKill(entt::entity entity);
  void FlushScene();

private:
  AquilaScene *m_Scene;
  entt::registry m_Registry;
  std::vector<entt::entity> m_DeletionQueue;
};
} // namespace Engine

#endif