#pragma once
#include "Aquila/Foundation/Singleton.h"
#include "Aquila/Graphics/SurfaceData.h"
#include "Aquila/Graphics/Material/Material.h"

namespace Aquila::Graphics {

struct MaterialDefinition {
	std::string name;
	MaterialType type = MaterialType::PBR;
	std::string shader_path;
	GpuSurfaceData defaults;
};

class MaterialRegistry : public Foundation::Singleton<MaterialRegistry> {
  public:
	void Register(MaterialDefinition definition);

	[[nodiscard]] const MaterialDefinition *find(const std::string &name) const;
	[[nodiscard]] bool has(const std::string &name) const;
	void remove(const std::string &name);

	[[nodiscard]] const std::unordered_map<std::string, MaterialDefinition> &get_all() const { return m_definitions; }

  private:
	std::unordered_map<std::string, MaterialDefinition> m_definitions;
};

} // namespace Aquila::Graphics
