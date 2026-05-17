#include "Aquila/Graphics/Material/MaterialDefinition.h"
#include "Aquila/Foundation/Macros.h"

namespace Aquila::Graphics {

void MaterialRegistry::Register(MaterialDefinition definition) {
	if (definition.name.empty()) {
		AQUILA_LOG_WARNING("MaterialRegistry: attempted to register a definition with an empty name");
		return;
	}
	m_Definitions[definition.name] = std::move(definition);
}

const MaterialDefinition *MaterialRegistry::Find(const std::string &name) const {
	auto it = m_Definitions.find(name);
	return it != m_Definitions.end() ? &it->second : nullptr;
}

bool MaterialRegistry::Has(const std::string &name) const {
	return m_Definitions.contains(name);
}

void MaterialRegistry::Remove(const std::string &name) {
	m_Definitions.erase(name);
}

} // namespace Aquila::Graphics
