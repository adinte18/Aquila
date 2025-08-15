#ifndef EVENT_H
#define EVENT_H

#include "AquilaCore.h"

#include "Scene/Components/SceneNodeComponent.h"
#include "Scene/Entity.h"
#include "Engine/Renderer/Texture2D.h"

// Base Event class
class Event {
public:
    virtual ~Event() = default;
    bool handled = false;
};

enum class UICommand : uint16_t {
    AddEntity,
    LoadMesh,
    AddPrimitiveCube,
    AddPrimitiveSphere,
    AddPrimitiveCylinder,
    AddPrimitiveCapsule,
    CreateChildEntity,
    DisownEntity,
    DeleteEntity,
    AttachToEntity,
    SaveScene,
    OpenScene,
    NewScene,
    ViewportResized
};

enum class UIEventResult : uint16_t {
    Success,
    Failure
};

using CommandParam = std::variant<
    int,
    float,
    bool,
    std::string,
    entt::entity,
    std::vector<Engine::AqEntity>,
    std::vector<Ref<Engine::Texture2D>>>;

using EventCallback = std::function<void(UIEventResult, CommandParam)>;

struct UICommandEvent : Event {
    UICommand m_Command;
    std::unordered_map<std::string_view, CommandParam> m_Params;
    EventCallback m_Callback;

    explicit UICommandEvent(const UICommand cmd, std::unordered_map<std::string_view, CommandParam> args = {}, EventCallback cb = nullptr)
        : m_Command(cmd), m_Params(std::move(args)), m_Callback(std::move(cb)) {}

    static const char* GetName() { return "UICommandEvent"; }
};

enum class QueryCommand : uint16_t {
    EntityHasParent,
    EntityIsDescendant,
    ExistingEntities,
    GetAttachableEntities,
    ContentBrowserAskTextures
};

struct QueryEvent : Event {
    QueryCommand m_Command;
    std::unordered_map<std::string_view, CommandParam> m_Params;
    EventCallback m_Callback;

    explicit QueryEvent(const QueryCommand cmd, std::unordered_map<std::string_view, CommandParam> args = {}, EventCallback cb = nullptr)
        : m_Command(cmd), m_Params(std::move(args)), m_Callback(std::move(cb)) {}

    static const char* GetName() { return "QueryEvent"; }
};


#endif // EVENT_H
