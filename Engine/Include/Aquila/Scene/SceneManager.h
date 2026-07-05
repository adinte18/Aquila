#ifndef AQUILA_SCENE_MANAGER_H
#define AQUILA_SCENE_MANAGER_H

#include "Aquila/Scene/Scene.h"

namespace Aquila::Assets {
class AssetManager; // Forward declaration
}

namespace Aquila::SceneManagement {

class SceneManager {
  public:
	SceneManager() = default;
	~SceneManager() = default;

	Scene *get_active_scene() const;
	Scene *get_scene(const Foundation::UUID &handle) const;
	Scene *get_scene_by_name(const std::string &name) const;
	bool has_scene() const;
	bool has_scene(const Foundation::UUID &handle) const;

	// Scene Creation & Loading
	Scene *create_scene(const std::string &name);
	Scene *load_scene(const std::string &filepath, Assets::AssetManager &asset_manager);
	Scene *load_scene_async(const std::string &filepath, Assets::AssetManager &asset_manager,
							const Delegate<void(Scene *)> &on_loaded = nullptr);

	// Scene Management
	void enqueue_scene(Unique<Scene> scene, const Delegate<void(Scene *)> &on_activated = nullptr);
	void change_scene(const Foundation::UUID &handle);
	void remove_scene(const Foundation::UUID &handle);
	void unload_scene(const Foundation::UUID &handle);
	void unload_all_scenes_except_active();

	// Scene Operations
	bool save_scene(const Foundation::UUID &handle, const std::string &filepath);
	bool save_active_scene(const std::string &filepath);
	Scene *duplicate_scene(const Foundation::UUID &handle, Assets::AssetManager &asset_manager,
						   const std::string &new_name = "");

	// Scene Change Requests
	void request_scene_change(const Foundation::UUID &handle);
	void request_scene_change(); // Uses last enqueued scene
	bool has_pending_scene_change() const;
	void process_scene_change();

	void activate_scene(const Foundation::UUID &handle);
	void activate_scene(Scene *scene);

	// Loading without activation
	Scene *load_scene_in_background(const std::string &filepath, Assets::AssetManager &asset_manager);

	// Query inactive scenes
	std::vector<Scene *> get_inactive_scenes() const;
	bool is_scene_active(const Foundation::UUID &handle) const;

	// Utilities
	std::vector<Scene *> get_all_scenes() const;
	std::vector<std::string> get_all_scene_names() const;
	size_t get_scene_count() const { return m_scenes.size(); }

	// Callbacks
	void set_on_scene_activated(const Delegate<void(Scene *)> &callback) { m_on_scene_activated = callback; }
	void set_on_scene_unloaded(const Delegate<void(Scene *)> &callback) { m_on_scene_unloaded = callback; }

  private:
	std::unordered_map<Foundation::UUID, Unique<Scene>> m_scenes;
	Scene *m_active_scene = nullptr;
	// Scene change handling
	Foundation::UUID m_pending_scene_change_handle = Foundation::UUID::null();
	bool m_has_pending_scene_change = false;

	// Callbacks
	Delegate<void(Scene *)> m_on_scene_activated;
	Delegate<void(Scene *)> m_on_scene_unloaded;

	// Helper methods
	std::string extract_scene_name(const std::string &filepath);
	bool validate_scene_file(const std::string &filepath);
};

} // namespace Aquila::SceneManagement

#endif
