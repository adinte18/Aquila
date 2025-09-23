#ifndef HIERARCHY__H
#define HIERARCHY__H

#include "Scene/Entity.h"
#include "Scene/EntityManager.h"
#include "Scene/Scene.h"
#include "Scene/SceneGraph.h"
#include "UI/Panels/IPanel.h"

namespace Editor::Panels {
class Hierarchy : public IPanel {
private:
  void DisplayHierarchy(Engine::AquilaScene *scene);
  void DisplayEntityNode(Engine::AquilaScene *scene, entt::entity entity);
  void ToggleVisibility(entt::registry &registry, entt::entity &entity,
                        bool &value);

  void PopupMenu();
  void Menu();

public:
  void Draw() override;
};
} // namespace Editor::Panels

#endif