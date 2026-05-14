#ifndef AQUILA_SCENE_H
#define AQUILA_SCENE_H

#include "entt.h"
#include "json.hpp"
#include "Aquila/Foundation/Defines.h"
#include "Aquila/Foundation/PrimitiveTypes.h"
#include "Aquila/Foundation/UUID.h"
#include "Aquila/Foundation/Invalidation/DirtySet.h"
#include "Components/CameraComponent.h"

namespace Aquila::Assets {
class AssetManager; // Forward declaration
}

namespace Aquila::SceneManagement {
class Entity;
class EntityManager;

class Scene final {
  public:
	explicit Scene();
	explicit Scene(const std::string &name);

	AQUILA_NONCOPYABLE(Scene);
	AQUILA_NONMOVEABLE(Scene);

	~Scene();

	void OnStart();

	[[nodiscard]] entt::registry &GetRegistry() const;
	[[nodiscard]] EntityManager *GetEntityManager() const;
	[[nodiscard]] const std::string &GetSceneName() const;
	[[nodiscard]] Entity GetActiveCameraEntity() const;
	[[nodiscard]] bool HasActiveCamera() const;
	[[nodiscard]] Entity FindPrimaryCamera() const;
	[[nodiscard]] const Utils::UUID GetHandle() const;

	void UpdateTransformHierarchy();
	void UpdateTransformRecursive(Entity entity, const glm::mat4 &parentWorld);
	void MarkTransformDirty(entt::entity entity);

	bool Serialize(const std::string &filepath);
	bool Deserialize(const std::string &filepath, Assets::AssetManager &assetManager);

	void SetAssetManager(Assets::AssetManager *assetManager) { m_AssetManager = assetManager; }

	void SetActiveCamera(Entity cameraEntity);

  protected:
	std::string m_SceneName;
	Unique<EntityManager> m_EntityManager;

  private:
	Foundation::UUID m_SceneID;
	entt::entity m_ActiveCameraEntity = entt::null;
	Assets::AssetManager *m_AssetManager = nullptr;
	Foundation::DirtySet<entt::entity> m_DirtyTransforms;

	void OnTransformConstruct(entt::registry &registry, entt::entity entity);
	[[nodiscard]] bool HasDirtyAncestor(entt::entity entity) const;
	[[nodiscard]] int GetEntityDepth(entt::entity entity) const;

	friend class Entity;
	friend class EntityManager;
	friend class SceneGraph;
};
} // namespace Aquila::SceneManagement

#endif
