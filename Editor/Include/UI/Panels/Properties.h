#ifndef PROPERTIES_ELEM_H
#define PROPERTIES_ELEM_H

#include "Scene/Scene.h"
#include "UI/Panels/IPanel.h"

namespace Editor::Panels {
class Properties : public IPanel {
private:
  struct ComponentMenuAction {
    enum Type { RESET, REMOVE_COMPONENT, CUSTOM };

    Type type;
    const char *icon;
    const char *label;
    std::function<void()> callback;

    ComponentMenuAction(Type t, const char *i, const char *l,
                        std::function<void()> c = nullptr)
        : type(t), icon(i), label(l), callback(c) {}
  };

  void DrawComponent_Transform(entt::registry &registry, entt::entity entity);
  void DrawComponent_Metadata(entt::registry &registry, entt::entity entity);
  void DrawComponent_Mesh(entt::registry &registry, entt::entity entity);
  void DrawComponent_Camera(entt::registry &registry, entt::entity entity);
  void DrawComponent_Light(entt::registry &registry, entt::entity entity);
  void DrawComponent_Material(entt::registry &registry, entt::entity entity);

  bool
  DrawComponentHeader(const char *icon, const char *label, const char *menuId,
                      const std::vector<ComponentMenuAction> &menuActions = {});

public:
  void Draw() override;
};
} // namespace Editor::Panels

#endif