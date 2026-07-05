#pragma once
#include "Aquila/Graphics/Material/Material.h"

namespace Aquila::Graphics {

// Simple named registry for materials.
// Does not own GFX resources directly — materials own their own GPU objects.
class MaterialLibrary {
  public:
	void Register(const std::string &name, Ref<Material> material);
	Ref<Material> get(const std::string &name) const;
	bool has(const std::string &name) const;
	void remove(const std::string &name);

	[[nodiscard]] const std::unordered_map<std::string, Ref<Material>> &get_all() const { return m_materials; }

  private:
	std::unordered_map<std::string, Ref<Material>> m_materials;
};

} // namespace Aquila::Graphics
