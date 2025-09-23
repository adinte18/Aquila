#ifndef AQUILA_SCENEGRAPH_H
#define AQUILA_SCENEGRAPH_H

#include "AquilaCore.h"
#include "entt.h"

namespace Engine {
class SceneGraph {
private:
  static void OnConstruct(entt::registry &registry, entt::entity entity);
  static void OnDestroy(entt::registry &registry, entt::entity entity);

public:
  SceneGraph() {};
  ~SceneGraph();

  SceneGraph(const SceneGraph &) = delete;
  SceneGraph(const SceneGraph &&) = delete;

  // construct
  void Construct(entt::registry &registry);

  void AddChild(entt::registry &registry, entt::entity parent,
                entt::entity child);
  void AttachTo(entt::registry &registry, entt::entity parent,
                entt::entity node);

  bool IsDescendant(entt::registry &registry, entt::entity potentialParent,
                    entt::entity entityToCheck);

  void RemoveChild(entt::registry &registry, entt::entity parent,
                   entt::entity child);
  void RemoveAllChildren(entt::registry &registry, entt::entity parent);
};
} // namespace Engine

#endif