#include "Aquila/Graphics/Material/MaterialLibrary.h"

namespace Aquila::Graphics {

void MaterialLibrary::Register(const std::string &name, Ref<Material> material) {
	if (material) {
		material->name = name;
	}
	m_materials[name] = std::move(material);
}

Ref<Material> MaterialLibrary::get(const std::string &name) const {
	auto it = m_materials.find(name);
	return it != m_materials.end() ? it->second : nullptr;
}

bool MaterialLibrary::has(const std::string &name) const {
	return m_materials.contains(name);
}

void MaterialLibrary::remove(const std::string &name) {
	m_materials.erase(name);
}

} // namespace Aquila::Graphics
