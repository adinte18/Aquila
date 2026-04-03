// #ifndef AQUILA_SCENE_EVENTS_H
// #define AQUILA_SCENE_EVENTS_H

// #include "Aquila/Application/Events/Event.h"
// #include "Aquila/Scene/Entity.h"

// namespace Aquila::Application::Events {

// class EntitySelectedEvent final : public Event {
//   public:
// 	explicit EntitySelectedEvent(const SceneManagement::Entity entity) : m_Entity(entity) {}

// 	[[nodiscard]] entt::entity GetEntityHandle() const { return m_Entity.GetHandle(); }
// 	[[nodiscard]] SceneManagement::Entity GetEntity() const { return m_Entity; }

// 	EVENT_CLASS_TYPE(EntitySelectedEvent)
// 	EVENT_CLASS_CATEGORY(EventCategory::Scene)

//   private:
// 	SceneManagement::Entity m_Entity;
// };

// class EntityCreatedEvent final : public Event {
//   public:
// 	explicit EntityCreatedEvent(const SceneManagement::Entity entity) : m_Entity(entity) {}

// 	[[nodiscard]] entt::entity GetEntityHandle() const { return m_Entity.GetHandle(); }
// 	[[nodiscard]] SceneManagement::Entity GetEntity() const { return m_Entity; }

// 	EVENT_CLASS_TYPE(EntityCreatedEvent)
// 	EVENT_CLASS_CATEGORY(EventCategory::Scene)

//   private:
// 	SceneManagement::Entity m_Entity;
// };

// class EntityDeletedEvent : public Event {
//   public:
// 	explicit EntityDeletedEvent(const SceneManagement::Entity entity) : m_Entity(entity) {}

// 	[[nodiscard]] entt::entity GetEntityHandle() const { return m_Entity.GetHandle(); }
// 	[[nodiscard]] SceneManagement::Entity GetEntity() const { return m_Entity; }

// 	EVENT_CLASS_TYPE(EntityDeletedEvent)
// 	EVENT_CLASS_CATEGORY(EventCategory::Scene)

//   private:
// 	SceneManagement::Entity m_Entity;
// };

// class EntityVisibilityToggle : public Event {
//   public:
// 	explicit EntityVisibilityToggle(const SceneManagement::Entity &entity, bool isVisible)
// 		: m_Entity(entity), m_IsVisibile(isVisible) {}

// 	[[nodiscard]] entt::entity GetEntityHandle() const { return m_Entity.GetHandle(); }
// 	[[nodiscard]] SceneManagement::Entity GetEntity() const { return m_Entity; }
// 	[[nodiscard]] bool IsVisibile() const { return m_IsVisibile; }

// 	EVENT_CLASS_TYPE(EntityVisibilityToggle);
// 	EVENT_CLASS_CATEGORY(EventCategory::Scene);

//   private:
// 	SceneManagement::Entity m_Entity;
// 	bool m_IsVisibile = true;
// };

// class SceneCreatedEvent final : public Event {
//   public:
// 	explicit SceneCreatedEvent(std::string scenePath) : m_ScenePath(std::move(scenePath)) {}

// 	[[nodiscard]] const std::string &GetScenePath() const { return m_ScenePath; }

// 	EVENT_CLASS_TYPE(SceneCreatedEvent)
// 	EVENT_CLASS_CATEGORY(EventCategory::Scene)

//   private:
// 	const std::string m_ScenePath;
// };

// class SceneLoadedEvent final : public Event {
//   public:
// 	explicit SceneLoadedEvent(std::string scenePath) : m_ScenePath(std::move(scenePath)) {}

// 	[[nodiscard]] const std::string &GetScenePath() const { return m_ScenePath; }

// 	EVENT_CLASS_TYPE(SceneLoadedEvent)
// 	EVENT_CLASS_CATEGORY(EventCategory::Scene)

//   private:
// 	const std::string m_ScenePath;
// };

// class SceneDestroyedEvent final : public Event {
//   public:
// 	explicit SceneDestroyedEvent(const Utils::UUID &handle) : m_SceneHandle(handle) {}

// 	[[nodiscard]] const Utils::UUID &GetHandle() const { return m_SceneHandle; }

// 	EVENT_CLASS_TYPE(SceneDestroyedEvent)
// 	EVENT_CLASS_CATEGORY(EventCategory::Scene)

//   private:
// 	const Utils::UUID m_SceneHandle;
// };

// class SceneSavedEvent final : public Event {
//   public:
// 	explicit SceneSavedEvent(std::string scenePath) : m_ScenePath(std::move(scenePath)) {}

// 	[[nodiscard]] const std::string &GetScenePath() const { return m_ScenePath; }

// 	EVENT_CLASS_TYPE(SceneSavedEvent)
// 	EVENT_CLASS_CATEGORY(EventCategory::Scene)

//   private:
// 	std::string m_ScenePath;
// };

// } // namespace Aquila::Application::Events

// #endif // AQUILA_SCENE_EVENTS_H
