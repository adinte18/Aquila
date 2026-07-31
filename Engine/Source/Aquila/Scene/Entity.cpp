#include "Aquila/Scene/Entity.h"
#include "Aquila/Foundation/Defines.h"
#include "Aquila/Scene/Components/MetadataComponent.h"

namespace Aquila::SceneManagement {

// by design both are set on entity creation
const Foundation::UUID &Entity::get_uuid() const {
	return try_get_component<Components::MetadataComponent>()->get_id();
}

const std::string &Entity::get_name() const {
	return try_get_component<Components::MetadataComponent>()->get_name();
}

void Entity::set_name(const std::string &new_name) {
	try_get_component<Components::MetadataComponent>()->set_name(new_name);
}

bool Entity::is_null() const {
	return m_entity_handle == entt::null || m_scene == nullptr;
}

void Entity::kill() const {
	AQUILA_ASSERT(m_scene, "There should be an active scene");
	if (is_valid()) {
		m_scene->get_registry().destroy(m_entity_handle);
	} else {
	}
}

bool Entity::is_valid() const {
	if (m_scene == nullptr || m_entity_handle == entt::null) {
		return false;
	}
	return m_scene->get_registry().valid(m_entity_handle);
}

} // namespace Aquila::SceneManagement
