#ifndef AQUILA_ENTITY_MNG_H
#define AQUILA_ENTITY_MNG_H

#include "Aquila/Scene/Entity.h"
#include "Aquila/Scene/Scene.h"

namespace Aquila::SceneManagement {

class EntityManager {
  public:
	explicit EntityManager(Scene *scene) : m_Scene(scene) { m_Registry = {}; }
	~EntityManager();

	entt::registry &GetRegistry();

	static Entity Create(Scene *scene) {
		const auto handle = scene->GetRegistry().create();
		return Entity(handle, scene);
	}

	static std::vector<Entity> CreateMany(Scene *scene, size_t count) {
		std::vector<Entity> entities;
		entities.reserve(count);

		auto handles = std::vector<entt::entity>(count);
		scene->GetRegistry().create(handles.begin(), handles.end());

		for (auto handle : handles) {
			entities.emplace_back(handle, scene);
		}
		return entities;
	}

	Entity CreateEntity(const std::string &name);
	void ApplyPreset(Entity &entity, EntityPreset preset);

	template <typename... Components, typename Func> void ForEach(Func &&func) {
		AQUILA_ASSERT(m_Scene, "There should be an active scene");
		for (auto view = m_Registry.view<Components...>(); auto entityHandle : view) {
			Entity entity(entityHandle, m_Scene);
			if constexpr (std::is_invocable_v<Func, Entity, Components &...>) {
				func(entity, view.template get<Components>(entityHandle)...);
			} else if constexpr (std::is_invocable_v<Func, Components &...>) {
				func(view.template get<Components>(entityHandle)...);
			} else {
				func(entity);
			}
		}
	}

	template <typename... Components, typename Func> void ForEach(Func &&func) const {
		AQUILA_ASSERT(m_Scene, "There should be an active scene");
		for (auto view = m_Registry.view<const Components...>(); auto entityHandle : view) {
			Entity entity(entityHandle, const_cast<Scene *>(m_Scene));
			if constexpr (std::is_invocable_v<Func, Entity, const Components &...>) {
				func(entity, view.template get<const Components>(entityHandle)...);
			} else if constexpr (std::is_invocable_v<Func, const Components &...>) {
				func(view.template get<const Components>(entityHandle)...);
			} else {
				func(entity);
			}
		}
	}

	template <typename... Components> [[nodiscard]] size_t Count() const {
		return m_Registry.view<Components...>().size();
	}

	template <typename... Components> Entity GetFirstEntityWith() {
		AQUILA_ASSERT(m_Scene, "There should be an active scene");
		auto view = m_Registry.view<Components...>();

		if (view.begin() == view.end()) {
			return Entity{};
		}

		return Entity{ *view.begin(), m_Scene };
	}

	template <typename... Components> std::vector<Entity> GetAllWith() {
		AQUILA_ASSERT(m_Scene, "There should be an active scene");
		auto view = m_Registry.view<Components...>();

		std::vector<Entity> result{};
		result.reserve(std::distance(view.begin(), view.end()));

		for (auto entity : view) {
			result.emplace_back(Entity{ entity, m_Scene });
		}

		return result;
	}

	template <typename... Components, typename Predicate> Entity FindFirst(Predicate &&predicate) {
		AQUILA_ASSERT(m_Scene, "There should be an active scene");
		auto view = m_Registry.view<Components...>();
		for (auto entityHandle : view) {
			Entity entity(entityHandle, m_Scene);
			if (predicate(entity, view.template get<Components>(entityHandle)...)) {
				return entity;
			}
		}
		return Entity::Null();
	}

	template <typename... Components> auto GetView() { return m_Registry.view<Components...>(); }

	template <typename... Owned, typename... Get, typename... Exclude> auto GetGroup() {
		return m_Registry.group<Owned...>(entt::get<Get...>, entt::exclude<Exclude...>);
	}

	std::optional<Entity> FindEntityByName(const std::string &name);
	std::optional<Entity> FindEntityByUUID(const Utils::UUID &uuid);
	std::string GetDefaultName(EntityPreset preset);
	[[nodiscard]] std::vector<Entity> GetChildren(Entity parent) const;
	[[nodiscard]] std::optional<Entity> GetParent(Entity child) const;

	[[nodiscard]] bool Exists(const Utils::UUID &uuid);
	[[nodiscard]] bool IsValid(Entity entity) const;
	[[nodiscard]] bool IsRegistryEmpty();
	void DestroyEntity(Entity entity);
	void QueueForKill(Entity entity);
	void FlushDeletionQueue();

	void ConstructSceneGraph();
	void AddChild(Entity parent, Entity child);
	void AttachTo(Entity parent, Entity node);
	void RemoveChild(Entity parent, Entity child);
	void RemoveAllChildren(Entity parent);
	bool IsDescendant(Entity potentialParent, Entity entityToCheck);
	std::string GenerateUniqueName(const std::string &baseName);

	void Clear();

  private:
	void OnSceneNodeConstruct(entt::registry &registry, entt::entity entityHandle);
	void OnSceneNodeDestroy(entt::registry &registry, entt::entity entityHandle);

	Scene *m_Scene;
	entt::registry m_Registry;
	std::vector<entt::entity> m_DeletionQueue;
};

} // namespace Aquila::SceneManagement

#endif
