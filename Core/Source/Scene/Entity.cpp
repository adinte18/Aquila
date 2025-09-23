#include "Scene/Entity.h"

namespace Engine {

// by design both are set on entity creation
const Utility::UUID &Entity::GetUUID() const {
  return TryGetComponent<MetadataComponent>()->ID;
}

const std::string &Entity::GetName() const {
  return TryGetComponent<MetadataComponent>()->Name;
}

void Entity::Kill() {
  AQUILA_ASSERT(m_Scene, "There should be an active scene");
  m_Scene->GetRegistry().destroy(m_EntityHandle);
}

bool Entity::IsValid() const {
  AQUILA_ASSERT(m_Scene, "There should be an active scene");
  return m_Scene && m_Scene->GetRegistry().valid(m_EntityHandle);
}

} // namespace Engine