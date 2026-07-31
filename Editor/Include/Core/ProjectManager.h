#pragma once

#include "Aquila/Foundation/PrimitiveTypes.h"

#include <string>
#include <vector>

namespace Editor {

struct ProjectInfo {
	std::string name;
	std::string directory;
	std::string project_file;
	std::vector<std::string> scenes;
	Uint64 created = 0;
};

class ProjectManager {
  public:
	static constexpr const char *K_ROOT = "/projects";

	ProjectManager();

	Option<ProjectInfo> create(const std::string &name);
	[[nodiscard]] std::vector<ProjectInfo> list() const;
	[[nodiscard]] Option<ProjectInfo> load(const std::string &directory) const;

  private:
	static std::string sanitize(const std::string &name);
};

} // namespace Editor
