#include "Aquila/Graphics/Material/MaterialDefinition.h"
#include "Aquila/Foundation/Macros.h"

namespace Aquila::Graphics {

void MaterialRegistry::Register(MaterialDefinition definition) {
	if (definition.name.empty()) {
		AQUILA_LOG_WARNING("MaterialRegistry: attempted to register a definition with an empty name");
		return;
	}
	m_definitions[definition.name] = std::move(definition);
}

const MaterialDefinition *MaterialRegistry::find(const std::string &name) const {
	auto it = m_definitions.find(name);
	return it != m_definitions.end() ? &it->second : nullptr;
}

bool MaterialRegistry::has(const std::string &name) const {
	return m_definitions.contains(name);
}

void MaterialRegistry::remove(const std::string &name) {
	m_definitions.erase(name);
}

} // namespace Aquila::Graphics
