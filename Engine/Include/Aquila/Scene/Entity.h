#ifndef AQUILA_ENTITY_H
#define AQUILA_ENTITY_H

#include "Aquila/Core/AquilaCore.h"
#include "Aquila/Scene/Scene.h"

namespace Aquila::SceneManagement {

class EntityManager;

enum class EntityPreset {
	Empty,
	Cube,
	Sphere,
	Cylinder,
	Plane,
	PointLight,
	DirectionalLight,
	SpotLight,
	EnvLight,
	OrthographicCamera,
	PerspectiveCamera,
	SkyLight
};

class Entity {
  public:
	Entity() = default;
	Entity(const entt::entity handle, Scene *scene) : m_EntityHandle(handle), m_Scene(scene) {};

	Entity(const Entity &other) = default;
	Entity &operator=(const Entity &other) = default;
	Entity(Entity &&other) noexcept = default;
	Entity &operator=(Entity &&other) noexcept = default;
	~Entity() = default;

	static Entity Null() { return {}; }

	template <typename T, typename... Args> T &AddComponent(Args &&...args) {
		return m_Scene->GetRegistry().emplace<T>(m_EntityHandle, std::forward<Args>(args)...);
	}

	template <typename T> T &GetComponent() { return m_Scene->GetRegistry().get<T>(m_EntityHandle); }

	template <typename T> const T &GetComponent() const {
		AQUILA_ASSERT(m_Scene, "There should be an active scene");
		return m_Scene->GetRegistry().get<T>(m_EntityHandle);
	}

	template <typename T> T *TryGetComponent() {
		if (HasComponent<T>()) {
			return &GetComponent<T>();
		}
		return nullptr;
	}

	template <typename T> const T *TryGetComponent() const {
		if (HasComponent<T>()) {
			return &GetComponent<T>();
		}
		return nullptr;
	}

	template <typename T> void TryRemoveComponent() const {
		if (HasComponent<T>()) {
			RemoveComponent<T>();
		}
	}

	template <typename T> void AddOrReplaceComponent() const {
		m_Scene->GetRegistry().emplace_or_replace<T>(m_EntityHandle);
	}

	template <typename T> void AddOrReplaceComponent(const T &component) const {
		m_Scene->GetRegistry().emplace_or_replace<T>(m_EntityHandle, component);
	}

	template <typename T, typename... Args> void AddOrReplaceComponent(Args &&...args) const {
		m_Scene->GetRegistry().emplace_or_replace<T>(m_EntityHandle, std::forward<Args>(args)...);
	}

	template <typename T> T &GetOrEmplace() { return m_Scene->GetRegistry().get_or_emplace<T>(m_EntityHandle); }

	template <typename T, typename... Args> T &GetOrEmplace(Args &&...args) {
		return m_Scene->GetRegistry().get_or_emplace<T>(m_EntityHandle, std::forward<Args>(args)...);
	}

	template <typename T> [[nodiscard]] bool HasComponent() const {
		AQUILA_ASSERT(m_Scene, "There should be an active scene");
		return m_Scene->GetRegistry().all_of<T>(m_EntityHandle);
	}

	template <typename... Components> [[nodiscard]] bool HasAllComponents() const {
		AQUILA_ASSERT(m_Scene, "There should be an active scene");
		return m_Scene->GetRegistry().all_of<Components...>(m_EntityHandle);
	}

	template <typename... Components> [[nodiscard]] bool HasAnyComponent() const {
		AQUILA_ASSERT(m_Scene, "There should be an active scene");
		return m_Scene->GetRegistry().any_of<Components...>(m_EntityHandle);
	}

	template <typename T> void RemoveComponent() const {
		AQUILA_ASSERT(m_Scene, "There should be an active scene");
		m_Scene->GetRegistry().remove<T>(m_EntityHandle);
	}

	template <typename... Components> void RemoveComponents() const {
		AQUILA_ASSERT(m_Scene, "There should be an active scene");
		m_Scene->GetRegistry().remove<Components...>(m_EntityHandle);
	}

	template <typename T> T &ReplaceComponent(const T &component) {
		return m_Scene->GetRegistry().replace<T>(m_EntityHandle, component);
	}

	template <typename T, typename... Args> T &ReplaceComponent(Args &&...args) {
		return m_Scene->GetRegistry().replace<T>(m_EntityHandle, std::forward<Args>(args)...);
	}

	template <typename T, typename... Args> T &PatchComponent(Args &&...args) {
		return m_Scene->GetRegistry().patch<T>(m_EntityHandle, std::forward<Args>(args)...);
	}

	[[nodiscard]] entt::entity GetHandle() const { return m_EntityHandle; }

	[[nodiscard]] Scene *GetScene() const {
		AQUILA_ASSERT(m_Scene, "There should be an active scene");
		return m_Scene;
	}

	[[nodiscard]] const Utils::UUID &GetUUID() const;
	[[nodiscard]] const std::string &GetName() const;

	void Kill() const;

	[[nodiscard]] bool IsValid() const;
	[[nodiscard]] bool IsNull() const;

	[[nodiscard]] bool Exists() const { return (m_Scene != nullptr) && m_Scene->GetRegistry().valid(m_EntityHandle); }

	[[nodiscard]] entt::registry::version_type GetVersion() const {
		AQUILA_ASSERT(m_Scene, "There should be an active scene");
		return m_Scene->GetRegistry().current(m_EntityHandle);
	}

	bool operator==(const Entity &other) const {
		return m_EntityHandle == other.m_EntityHandle && m_Scene == other.m_Scene;
	}

	bool operator!=(const Entity &other) const { return !(*this == other); }

	bool operator<(const Entity &other) const { return m_EntityHandle < other.m_EntityHandle; }

	explicit operator entt::entity() const { return m_EntityHandle; }

	[[nodiscard]] std::string ToString() const {
		if (IsNull()) {
			return "Entity::Null";
		}
		return "Entity(" + std::to_string(static_cast<uint32_t>(m_EntityHandle)) + ")";
	}

	void CopyFrom(const Entity &other) const {
		AQUILA_ASSERT(m_Scene && other.m_Scene, "Both entities must have valid scenes");
	}

  private:
	entt::entity m_EntityHandle = entt::null;
	Scene *m_Scene = nullptr;

	friend class EntityManager;
};

} // namespace Aquila::SceneManagement

namespace std {
template <> struct hash<Aquila::SceneManagement::Entity> {
	size_t operator()(const Aquila::SceneManagement::Entity &entity) const noexcept {
		return hash<entt::entity>{}(entity.GetHandle());
	}
};
} // namespace std

#endif
