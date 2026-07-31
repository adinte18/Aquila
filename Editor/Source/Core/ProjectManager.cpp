#include "Core/ProjectManager.h"

#include "Aquila/Foundation/Macros.h"
#include "Aquila/Platform/Filesystem/Filesystem.h"
#include "Aquila/Platform/Filesystem/NativeFileSystem.h"
#include "Aquila/Platform/Filesystem/VirtualFileSystem.h"
#include "Aquila/Scene/Components/CameraComponent.h"
#include "Aquila/Scene/Components/TransformComponent.h"
#include "Aquila/Scene/Entity.h"
#include "Aquila/Scene/EntityManager.h"
#include "Aquila/Scene/Scene.h"

#include "json.hpp"

#include <cctype>
#include <chrono>

namespace Editor {

using Aquila::Platform::Filesystem::NativeFileSystem;
using Aquila::Platform::Filesystem::VirtualFileSystem;
namespace Components = Aquila::SceneManagement::Components;

namespace {

Uint64 now_seconds() {
	const auto now = std::chrono::system_clock::now().time_since_epoch();
	return static_cast<Uint64>(std::chrono::duration_cast<std::chrono::seconds>(now).count());
}

} // namespace

ProjectManager::ProjectManager() {
	const std::string root = Aquila::Platform::Filesystem::path_executable_dir() + "/Projects";
	VirtualFileSystem::get()->mount(K_ROOT, std::make_shared<NativeFileSystem>(root));
}

std::string ProjectManager::sanitize(const std::string &name) {
	std::string out;
	out.reserve(name.size());
	for (const char c : name) {
		const auto uc = static_cast<unsigned char>(c);
		if (std::isalnum(uc) || c == '-' || c == '_') {
			out += c;
		} else if (c == ' ') {
			out += '_';
		}
	}
	return out;
}

Option<ProjectInfo> ProjectManager::create(const std::string &name) {
	auto *vfs = VirtualFileSystem::get();

	const std::string safe = sanitize(name);
	if (safe.empty()) {
		AQUILA_LOG_ERROR("ProjectManager: invalid project name '{}'", name);
		return std::nullopt;
	}

	ProjectInfo info;
	info.name = name;
	info.directory = std::string(K_ROOT) + "/" + safe;
	info.project_file = info.directory + "/project.aqproject";
	info.created = now_seconds();

	const std::string scene_file = safe + ".aqscene";
	info.scenes = { scene_file };

	if (vfs->exists(info.project_file)) {
		AQUILA_LOG_WARNING("ProjectManager: project '{}' already exists", safe);
		return std::nullopt;
	}

	if (!vfs->create_dir(info.directory)) {
		AQUILA_LOG_ERROR("ProjectManager: failed to create directory {}", info.directory);
		return std::nullopt;
	}

	Aquila::SceneManagement::Scene scene(name);
	auto *entity_manager = scene.get_entity_manager();
	auto camera = entity_manager->create_entity("Camera");
	auto &camera_component = camera.add_component<Components::CameraComponent>();
	camera_component.fov = 60.F;
	camera_component.near_plane = 0.1f;
	camera_component.far_plane = 500.F;
	camera_component.aspect_ratio = 16.F / 9.F;
	camera_component.primary = true;
	camera.get_component<Components::TransformComponent>().set_local_position({ 0.F, 1.5f, -5.F });
	scene.set_active_camera(camera);
	scene.serialize(info.directory + "/" + scene_file);

	nlohmann::ordered_json root;
	root["version"] = 1;
	root["name"] = name;
	root["created"] = info.created;
	root["scenes"] = info.scenes;

	if (!vfs->write_text_file(info.project_file, root.dump(4))) {
		AQUILA_LOG_ERROR("ProjectManager: failed to write {}", info.project_file);
		return std::nullopt;
	}

	AQUILA_LOG_INFO("ProjectManager: created project '{}' at {}", name, info.directory);
	return info;
}

std::vector<ProjectInfo> ProjectManager::list() const {
	std::vector<ProjectInfo> projects;
	auto *vfs = VirtualFileSystem::get();

	for (const std::string &entry : vfs->list_directory(K_ROOT)) {
		const std::string directory = std::string(K_ROOT) + "/" + entry;
		if (!vfs->is_directory(directory)) {
			continue;
		}
		if (Option<ProjectInfo> info = load(directory)) {
			projects.push_back(*info);
		}
	}

	return projects;
}

Option<ProjectInfo> ProjectManager::load(const std::string &directory) const {
	auto *vfs = VirtualFileSystem::get();

	const std::string project_file = directory + "/project.aqproject";
	if (!vfs->exists(project_file)) {
		return std::nullopt;
	}

	const std::string text = vfs->read_text_file(project_file);
	nlohmann::json root = nlohmann::json::parse(text, nullptr, false);
	if (root.is_discarded()) {
		AQUILA_LOG_WARNING("ProjectManager: malformed project file {}", project_file);
		return std::nullopt;
	}

	ProjectInfo info;
	info.directory = directory;
	info.project_file = project_file;
	info.name = root.value("name", std::string());
	info.created = root.value("created", static_cast<Uint64>(0));
	if (root.contains("scenes") && root["scenes"].is_array()) {
		info.scenes = root["scenes"].get<std::vector<std::string>>();
	}

	return info;
}

} // namespace Editor
