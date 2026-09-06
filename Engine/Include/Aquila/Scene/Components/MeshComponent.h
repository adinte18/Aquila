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
	Uint32 version = 0;

	/**
	 * @brief Set a new mesh and automatically increment version
	 * @param newMesh The mesh to assign
	 */
	void set_mesh(const Ref<Graphics::Resources::Mesh> &new_mesh) {
		if (data != new_mesh) {
			data = new_mesh;
			version++;

			// Reset materials when mesh changes
			materials.clear();
			material_asset_paths.clear();
		}
	}

	// MATERIAL DATA

	/**
	 * @brief Material assets referenced by this mesh
	 * Key: submesh index (0 for single material)
	 * Value: path to .aqmat file (e.g., "assets://materials/wood.aqmat")
	 */
	std::unordered_map<Uint32, std::string> material_asset_paths;

	/**
	 * @brief Runtime material instances
	 * Key: submesh index (0 for single material)
	 * Value: Material instance (can override properties per-mesh)
	 *
	 * These are INSTANCES - changes only affect this mesh.
	 * The underlying asset is shared and loaded through AssetManager.
	 */
	std::unordered_map<Uint32, Ref<Graphics::Material>> materials;

	/**
	 * @brief Set material for entire mesh or specific submesh
	 * @param material Material instance to use
	 * @param submeshIndex Submesh index (default 0 for whole mesh)
	 */
	void set_material(const Ref<Graphics::Material> &material, Uint32 submesh_index = 0) {
		if (material) {
			materials[submesh_index] = material;
		}
	}

	/**
	 * @brief Set material from asset path
	 * Material will be loaded through AssetManager on next access
	 * @param assetPath Path to .aqmat file
	 * @param submeshIndex Submesh index (default 0 for whole mesh)
	 */
	void set_material_asset(const std::string &asset_path, Uint32 submesh_index = 0) {
		if (!asset_path.empty()) {
			material_asset_paths[submesh_index] = asset_path;
			// Clear runtime instance - will be reloaded from asset
			materials.erase(submesh_index);
		}
	}

	/**
	 * @brief Get material for submesh
	 * @param submeshIndex Submesh index
	 * @return Material instance or nullptr if not set
	 */
	Ref<Graphics::Material> get_material(Uint32 submesh_index = 0) const {
		auto it = materials.find(submesh_index);
		return it != materials.end() ? it->second : nullptr;
	}

	/**
	 * @brief Get material asset path for submesh
	 * @param submeshIndex Submesh index
	 * @return Asset path or empty string if not set
	 */
	std::string get_material_asset_path(Uint32 submesh_index = 0) const {
		auto it = material_asset_paths.find(submesh_index);
		return it != material_asset_paths.end() ? it->second : "";
	}

	/**
	 * @brief Check if submesh has a material asset reference
	 */
	bool has_material_asset(Uint32 submesh_index = 0) const {
		return material_asset_paths.contains(submesh_index);
	}

	/**
	 * @brief Check if submesh has a loaded material instance
	 */
	bool has_material(Uint32 submesh_index = 0) const { return materials.contains(submesh_index); }

	/**
	 * @brief Clear all materials
	 */
	void clear_materials() {
		materials.clear();
		material_asset_paths.clear();
	}

	/**
	 * @brief Get number of material slots
	 */
	Uint32 get_material_slot_count() const {
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
	bool cast_shadows = true;

	/**
	 * @brief Whether this mesh should receive shadows
	 */
	bool receive_shadows = true;

	// HELPER METHODS

	/**
	 * @brief Check if this mesh is valid for rendering
	 */
	bool is_valid() const { return data != nullptr; }

	/**
	 * @brief Get the primary material (submesh 0)
	 * Convenience method for single-material meshes
	 */
	Ref<Graphics::Material> get_primary_material() const { return get_material(0); }

	/**
	 * @brief Set the primary material (submesh 0)
	 * Convenience method for single-material meshes
	 */
	void set_primary_material(const Ref<Graphics::Material> &material) { set_material(material, 0); }

	/**
	 * @brief Get material for rendering with fallback chain
	 * @param submeshIndex The submesh to get material for
	 * @param fallbackMaterial Fallback if no material is set
	 * @return Material to use for rendering
	 */
	Ref<Graphics::Material> get_render_material(uint32_t submesh_index = 0,
											  Ref<Graphics::Material> fallback_material = nullptr) const {
		if (has_material(submesh_index)) {
			return get_material(submesh_index);
		}

		if (submesh_index != 0 && has_material(0)) {
			return get_material(0);
		}

		return fallback_material;
	}
};

} // namespace Aquila::SceneManagement::Components

#endif // MESH_COMPONENT_H
