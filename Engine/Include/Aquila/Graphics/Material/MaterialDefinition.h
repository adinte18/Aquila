#pragma once
#include "Aquila/Foundation/Singleton.h"
#include "Aquila/Graphics/SurfaceData.h"
#include "Aquila/Graphics/Material/Material.h"

namespace Aquila::Graphics {

struct MaterialDefinition {
	std::string name;
	MaterialType type = MaterialType::PBR;
	std::string shaderPath;
	GpuSurfaceData defaults;
};

class MaterialRegistry : public Foundation::Singleton<MaterialRegistry> {
  public:
	void Register(MaterialDefinition definition);

	[[nodiscard]] const MaterialDefinition *Find(const std::string &name) const;
	[[nodiscard]] bool Has(const std::string &name) const;
	void Remove(const std::string &name);

	[[nodiscard]] const std::unordered_map<std::string, MaterialDefinition> &GetAll() const { return m_Definitions; }

  private:
	std::unordered_map<std::string, MaterialDefinition> m_Definitions;
};

} // namespace Aquila::Graphics
