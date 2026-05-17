#pragma once
#include "Aquila/Graphics/Material/Material.h"
#include "Aquila/Graphics/Material/MaterialDefinition.h"
#include "Aquila/Graphics/SurfaceData.h"

namespace Aquila::SceneManagement::Components {

struct MaterialComponent {
	Ref<Graphics::Material> material;

	Graphics::MaterialType type = Graphics::MaterialType::Lit;

	Graphics::GpuSurfaceData surfaceProperties;

	uint32 materialIndex = UINT32_MAX;

	MaterialComponent() = default;
	explicit MaterialComponent(Graphics::MaterialType t) : type(t) {}
	explicit MaterialComponent(Ref<Graphics::Material> mat)
		: material(std::move(mat)), type(material ? material->GetType() : Graphics::MaterialType::Lit) {}

	void SyncType() {
		if (material) {
			type = material->GetType();
		}
	}

	static MaterialComponent FromDefinition(const std::string &definitionName) {
		MaterialComponent comp;
		if (const auto *def = Graphics::MaterialRegistry::Get()->Find(definitionName)) {
			comp.type = def->type;
			comp.surfaceProperties = def->defaults;
		}
		return comp;
	}
};

} // namespace Aquila::SceneManagement::Components
