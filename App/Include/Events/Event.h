#ifndef EVENT_H
#define EVENT_H

#include <string>
#include <unordered_map>
#include <utility>
#include <variant>
#include <functional>

// Base Event class
class Event {
public:
    virtual ~Event() = default;
    bool handled = false;
};

enum class UICommand : uint16_t {
    LoadTexture,
    AddSphere,
    AddCube,
    AddEntity,
    RemoveEntity,
    AddComponent,
    RemoveComponent,
    AddEnvMap,
    AddLight,
    AddMesh,
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
    std::shared_ptr<Engine::Texture2D>,
    std::shared_ptr<ECS::Entity>
>;

using EventCallback = std::function<void(UIEventResult, CommandParam)>;

struct UICommandEvent : Event {
    UICommand m_Command;  // Command type
    std::unordered_map<std::string_view, CommandParam> m_Params;
    EventCallback m_Callback;

    explicit UICommandEvent(const UICommand cmd, std::unordered_map<std::string_view, CommandParam> args = {}, EventCallback cb = nullptr)
        : m_Command(cmd), m_Params(std::move(args)), m_Callback(std::move(cb)) {}

    static const char* GetName() { return "UICommandEvent"; }
};

#endif // EVENT_H
