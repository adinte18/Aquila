#ifndef AQUILA_EVENT_H
#define AQUILA_EVENT_H

#include "AquilaCore.h"
#include "Engine/Renderer/Texture2D.h"
#include "Scene/Components/SceneNodeComponent.h"
#include "Scene/Entity.h"

namespace Engine {

struct EventResult {
  enum class Status {
    Success,
    InvalidParameters,
    ResourceNotFound,
    OperationFailed,
    InternalError,
    EntityNotFound,
    SceneNotActive
  };

  Status status = Status::Success;
  std::string errorMessage;
  std::optional<std::any> data;

  static EventResult Success(std::any data = {}) {
    return {Status::Success, "", std::move(data)};
  }

  static EventResult Error(Status status, const std::string &message) {
    AQUILA_LOG_ERROR(message);
    return {status, message, {}};
  }

  bool IsSuccess() const { return status == Status::Success; }

  template <typename T> std::optional<T> GetData() const {
    if (!data.has_value())
      return std::nullopt;
    try {
      return std::any_cast<T>(data.value());
    } catch (const std::bad_any_cast &) {
      return std::nullopt;
    }
  }
};

using EventCallback = Delegate<void(const EventResult &)>;
class Event {
public:
  virtual ~Event() = default;
  virtual const char *GetName() const = 0;
  virtual std::string GetDescription() const { return GetName(); }

  bool handled = false;
  EventCallback callback;
  std::chrono::steady_clock::time_point timestamp =
      std::chrono::steady_clock::now();
};

template <typename EventType> class TypedEvent : public Event {
public:
  static constexpr const char *EVENT_NAME = EventType::EVENT_NAME;

  const char *GetName() const override { return EVENT_NAME; }
};

struct AddEntityEvent : public TypedEvent<AddEntityEvent> {
  static constexpr const char *EVENT_NAME = "AddEntityEvent";
  std::optional<std::string> entityName;
  std::optional<entt::entity> parent;
  EntityPreset preset = EntityPreset::Empty;

  std::string GetDescription() const override {
    return entityName ? "Add entity: " + *entityName : "Add entity";
  }
};

struct LoadMeshEvent : public TypedEvent<LoadMeshEvent> {
  static constexpr const char *EVENT_NAME = "LoadMeshEvent";
  std::string meshPath;
  std::optional<entt::entity> targetEntity;

  std::string GetDescription() const override {
    return "Load mesh: " + meshPath;
  }
};

struct AddPrimitiveEvent : public TypedEvent<AddPrimitiveEvent> {
  static constexpr const char *EVENT_NAME = "AddPrimitiveEvent";
  enum class PrimitiveType { Cube, Sphere, Cylinder, Capsule };

  PrimitiveType primitiveType;
  std::optional<entt::entity> parent;
  std::optional<std::string> name;

  std::string GetDescription() const override {
    const char *typeNames[] = {"Cube", "Sphere", "Cylinder", "Capsule"};
    return std::string("Add primitive: ") +
           typeNames[static_cast<int>(primitiveType)];
  }
};

struct CreateChildEntityEvent : public TypedEvent<CreateChildEntityEvent> {
  static constexpr const char *EVENT_NAME = "CreateChildEntityEvent";
  entt::entity parentEntity;
  std::optional<std::string> childName;
};

struct DisownEntityEvent : public TypedEvent<DisownEntityEvent> {
  static constexpr const char *EVENT_NAME = "DisownEntityEvent";
  entt::entity entity;
};

struct DeleteEntityEvent : public TypedEvent<DeleteEntityEvent> {
  static constexpr const char *EVENT_NAME = "DeleteEntityEvent";

  entt::entity entity;

  std::string GetDescription() const override {
    return "Delete entity: " + std::to_string(static_cast<uint32>(entity));
  }
};

struct AttachToEntityEvent : public TypedEvent<AttachToEntityEvent> {
  static constexpr const char *EVENT_NAME = "AttachToEntityEvent";
  entt::entity entity;
  entt::entity parent;
};

struct SaveSceneEvent : public TypedEvent<SaveSceneEvent> {
  static constexpr const char *EVENT_NAME = "SaveSceneEvent";
  std::optional<std::string> customPath;
};

struct OpenSceneEvent : public TypedEvent<OpenSceneEvent> {
  static constexpr const char *EVENT_NAME = "OpenSceneEvent";
  std::string scenePath;
};

struct NewSceneEvent : public TypedEvent<NewSceneEvent> {
  static constexpr const char *EVENT_NAME = "NewSceneEvent";
  std::string sceneName;
};

struct ViewportResizedEvent : public TypedEvent<ViewportResizedEvent> {
  static constexpr const char *EVENT_NAME = "ViewportResizedEvent";
  uint32 width;
  uint32 height;
};

struct EntityHasParentQuery : public TypedEvent<EntityHasParentQuery> {
  static constexpr const char *EVENT_NAME = "EntityHasParentQuery";
  entt::entity entity;
};

struct EntityIsDescendantQuery : public TypedEvent<EntityIsDescendantQuery> {
  static constexpr const char *EVENT_NAME = "EntityIsDescendantQuery";
  entt::entity ancestor;
  entt::entity descendant;
};

struct GetExistingEntitiesQuery : public TypedEvent<GetExistingEntitiesQuery> {
  static constexpr const char *EVENT_NAME = "GetExistingEntitiesQuery";
};

struct GetAttachableEntitiesQuery
    : public TypedEvent<GetAttachableEntitiesQuery> {
  static constexpr const char *EVENT_NAME = "GetAttachableEntitiesQuery";
  entt::entity entity;
};

struct ContentBrowserTexturesQuery
    : public TypedEvent<ContentBrowserTexturesQuery> {
  static constexpr const char *EVENT_NAME = "ContentBrowserTexturesQuery";
};

template <typename EventType> class EventValidator {
public:
  static EventResult Validate(const EventType &event) {
    return EventResult::Success();
  }
};

template <> class EventValidator<LoadMeshEvent> {
public:
  static EventResult Validate(const LoadMeshEvent &event) {
    if (event.meshPath.empty()) {
      return EventResult::Error(EventResult::Status::InvalidParameters,
                                "Mesh path cannot be empty");
    }
    return EventResult::Success();
  }
};

template <> class EventValidator<OpenSceneEvent> {
public:
  static EventResult Validate(const OpenSceneEvent &event) {
    if (event.scenePath.empty()) {
      return EventResult::Error(EventResult::Status::InvalidParameters,
                                "Scene path cannot be empty");
    }
    return EventResult::Success();
  }
};

template <> class EventValidator<NewSceneEvent> {
public:
  static EventResult Validate(const NewSceneEvent &event) {
    if (event.sceneName.empty()) {
      return EventResult::Error(EventResult::Status::InvalidParameters,
                                "Scene name cannot be empty");
    }
    return EventResult::Success();
  }
};

template <> class EventValidator<ViewportResizedEvent> {
public:
  static EventResult Validate(const ViewportResizedEvent &event) {
    if (event.width == 0 || event.height == 0) {
      return EventResult::Error(
          EventResult::Status::InvalidParameters,
          "Viewport dimensions must be greater than zero");
    }
    return EventResult::Success();
  }
};

} // namespace Engine

#endif