#include "Aquila/Graphics/Material/MaterialLibrary.h"

namespace Aquila::Graphics {

void MaterialLibrary::Register(const std::string &name, Ref<Material> material) {
	if (material) {
		material->name = name;
	}
	m_Materials[name] = std::move(material);
}

Ref<Material> MaterialLibrary::Get(const std::string &name) const {
	auto it = m_Materials.find(name);
	return it != m_Materials.end() ? it->second : nullptr;
}

bool MaterialLibrary::Has(const std::string &name) const {
	return m_Materials.contains(name);
}

void MaterialLibrary::Remove(const std::string &name) {
	m_Materials.erase(name);
}

} // namespace Aquila::Graphics
