#ifndef MESH_COMPONENT_H
#define MESH_COMPONENT_H

#include "Aquila/Assets/AssetManager.h"
#include "Aquila/Graphics/Material/MaterialLibrary.h"
#include "Aquila/Graphics/Resources/Mesh.h"
#include "Aquila/Graphics/Material/Material.h"

namespace Aquila::SceneManagement::Components {

/**
 * @brief Component for attaching a mesh and materials to an entity
 *
 * Supports:
 * - Single material for entire mesh
 * - Multiple materials for submeshes
 * - Material asset references (.aqmat files)
 * - Per-instance material parameter overrides
 */
struct MeshComponent {
	// MESH DATA
	Ref<Graphics::Resources::Mesh> data{};

	/**
	 * @brief Version counter that increments when mesh changes
	 * Used by renderer to detect mesh swaps without pointer comparison
	 */
	uint32 version = 0;

	/**
	 * @brief Set a new mesh and automatically increment version
	 * @param newMesh The mesh to assign
	 */
	void SetMesh(const Ref<Graphics::Resources::Mesh> &newMesh) {
		if (data != newMesh) {
			data = newMesh;
			version++;

			// Reset materials when mesh changes
			materials.clear();
			materialAssetPaths.clear();
		}
	}

	// MATERIAL DATA

	/**
	 * @brief Material assets referenced by this mesh
	 * Key: submesh index (0 for single material)
	 * Value: path to .aqmat file (e.g., "assets://materials/wood.aqmat")
	 */
	std::unordered_map<uint32, std::string> materialAssetPaths;

	/**
	 * @brief Runtime material instances
	 * Key: submesh index (0 for single material)
	 * Value: Material instance (can override properties per-mesh)
	 *
	 * These are INSTANCES - changes only affect this mesh.
	 * The underlying asset is shared and loaded through AssetManager.
	 */
	std::unordered_map<uint32, Ref<Graphics::Material::Material>> materials;

	/**
	 * @brief Set material for entire mesh or specific submesh
	 * @param material Material instance to use
	 * @param submeshIndex Submesh index (default 0 for whole mesh)
	 */
	void SetMaterial(const Ref<Graphics::Material::Material> &material, uint32 submeshIndex = 0) {
		if (material) {
			materials[submeshIndex] = material;
		}
	}

	/**
	 * @brief Set material from asset path
	 * Material will be loaded through AssetManager on next access
	 * @param assetPath Path to .aqmat file
	 * @param submeshIndex Submesh index (default 0 for whole mesh)
	 */
	void SetMaterialAsset(const std::string &assetPath, uint32 submeshIndex = 0) {
		if (!assetPath.empty()) {
			materialAssetPaths[submeshIndex] = assetPath;
			// Clear runtime instance - will be reloaded from asset
			materials.erase(submeshIndex);
		}
	}

	/**
	 * @brief Get material for submesh
	 * @param submeshIndex Submesh index
	 * @return Material instance or nullptr if not set
	 */
	Ref<Graphics::Material::Material> GetMaterial(uint32 submeshIndex = 0) const {
		auto it = materials.find(submeshIndex);
		return it != materials.end() ? it->second : nullptr;
	}

	/**
	 * @brief Get material asset path for submesh
	 * @param submeshIndex Submesh index
	 * @return Asset path or empty string if not set
	 */
	std::string GetMaterialAssetPath(uint32 submeshIndex = 0) const {
		auto it = materialAssetPaths.find(submeshIndex);
		return it != materialAssetPaths.end() ? it->second : "";
	}

	/**
	 * @brief Check if submesh has a material asset reference
	 */
	bool HasMaterialAsset(uint32 submeshIndex = 0) const {
		return materialAssetPaths.find(submeshIndex) != materialAssetPaths.end();
	}

	/**
	 * @brief Check if submesh has a loaded material instance
	 */
	bool HasMaterial(uint32 submeshIndex = 0) const { return materials.find(submeshIndex) != materials.end(); }

	/**
	 * @brief Clear all materials
	 */
	void ClearMaterials() {
		materials.clear();
		materialAssetPaths.clear();
	}

	/**
	 * @brief Get number of material slots
	 */
	uint32 GetMaterialSlotCount() const {
		if (!data) {
			return 0;
		}
		// TODO: when submeshes will be eventually supported i should use that count
		return 1;
	}

	// RENDERING FLAGS

	/**
	 * @brief Whether this mesh should cast shadows
	 */
	bool castShadows = true;

	/**
	 * @brief Whether this mesh should receive shadows
	 */
	bool receiveShadows = true;

	// HELPER METHODS

	/**
	 * @brief Check if this mesh is valid for rendering
	 */
	bool IsValid() const { return data != nullptr; }

	/**
	 * @brief Get the primary material (submesh 0)
	 * Convenience method for single-material meshes
	 */
	Ref<Graphics::Material::Material> GetPrimaryMaterial() const { return GetMaterial(0); }

	/**
	 * @brief Set the primary material (submesh 0)
	 * Convenience method for single-material meshes
	 */
	void SetPrimaryMaterial(const Ref<Graphics::Material::Material> &material) { SetMaterial(material, 0); }

	/**
	 * @brief Get material for rendering with fallback chain
	 * @param submeshIndex The submesh to get material for
	 * @param fallbackMaterial Fallback if no material is set
	 * @return Material to use for rendering
	 */
	Ref<Graphics::Material::Material>
	GetRenderMaterial(uint32_t submeshIndex = 0, Ref<Graphics::Material::Material> fallbackMaterial = nullptr) const {
		if (HasMaterial(submeshIndex)) {
			return GetMaterial(submeshIndex);
		}

		if (submeshIndex != 0 && HasMaterial(0)) {
			return GetMaterial(0);
		}

		return fallbackMaterial;
	}
};

} // namespace Aquila::SceneManagement::Components

#endif // MESH_COMPONENT_H
