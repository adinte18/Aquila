#include "Aquila/Scene/Entity.h"
#include "Aquila/Foundation/Defines.h"
#include "Aquila/Scene/Components/MetadataComponent.h"

namespace Aquila::SceneManagement {

// by design both are set on entity creation
const Utils::UUID &Entity::GetUUID() const {
	return GetComponent<Components::MetadataComponent>().GetId();
}

const std::string &Entity::GetName() const {
	return GetComponent<Components::MetadataComponent>().GetName();
}

bool Entity::IsNull() const {
	return m_EntityHandle == entt::null || m_Scene == nullptr;
}

void Entity::Kill() const {
	AQUILA_ASSERT(m_Scene, "There should be an active scene");
	if (IsValid()) {
		m_Scene->GetRegistry().destroy(m_EntityHandle);
	} else {
	}
}

bool Entity::IsValid() const {
	if (m_Scene == nullptr || m_EntityHandle == entt::null) {
		return false;
	}
	return m_Scene->GetRegistry().valid(m_EntityHandle);
}

} // namespace Aquila::SceneManagement
