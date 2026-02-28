#pragma once
#include "Aquila/Graphics/Material/MaterialSerializer.h"
namespace Aquila::SceneManagement::Components {
struct MaterialComponent {
	std::string materialAssetPath;				// Path to .aqmat file
	Ref<Graphics::Material::Material> material; // Runtime material instance

	MaterialComponent() = default;

	MaterialComponent(const std::string &assetPath) : materialAssetPath(assetPath), material(nullptr) {}

	MaterialComponent(const Ref<Graphics::Material::Material> &mat) : material(mat) {
		// Optionally set path if material was loaded from file
	}

	Ref<Graphics::Material::Material> GetMaterial() const { return material; }

	void SetMaterial(Ref<Graphics::Material::Material> mat) { material = mat; }

	void SetMaterialAsset(const std::string &path) { materialAssetPath = path; }

	const std::string &GetMaterialAssetPath() const { return materialAssetPath; }

	bool HasMaterialAsset() const { return !materialAssetPath.empty(); }
};
} // namespace Aquila::SceneManagement::Components