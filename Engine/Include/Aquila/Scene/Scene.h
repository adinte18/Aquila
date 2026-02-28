#ifndef AQUILA_SCENE_H
#define AQUILA_SCENE_H

#include "entt.h"
#include "json.hpp"
#include "Aquila/Core/AquilaCore.h"
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

	bool Serialize(const std::string &filepath);
	bool Deserialize(const std::string &filepath, Assets::AssetManager &assetManager);

	void SetAssetManager(Assets::AssetManager *assetManager) { m_AssetManager = assetManager; }

	void SetActiveCamera(Entity cameraEntity);

  protected:
	std::string m_SceneName;
	Unique<EntityManager> m_EntityManager;

  private:
	Utils::UUID m_SceneID; // scene handle
	entt::entity m_ActiveCameraEntity = entt::null;
	Assets::AssetManager *m_AssetManager = nullptr;

	friend class Entity;
	friend class EntityManager;
	friend class SceneGraph;
};
} // namespace Aquila::SceneManagement

#endif
