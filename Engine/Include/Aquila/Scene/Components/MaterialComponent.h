#pragma once
#include "Aquila/Graphics/Material/Material.h"

namespace Aquila::SceneManagement::Components {

struct MaterialComponent {
	Ref<Graphics::Material> material;
	std::string materialAssetPath;

	MaterialComponent() = default;
	explicit MaterialComponent(Ref<Graphics::Material> mat) : material(std::move(mat)) {}

	void SetMaterialAsset(const std::string &path) { materialAssetPath = path; }
	const std::string &GetMaterialAssetPath() const { return materialAssetPath; }
	bool HasMaterialAsset() const { return !materialAssetPath.empty(); }
};

} // namespace Aquila::SceneManagement::Components
