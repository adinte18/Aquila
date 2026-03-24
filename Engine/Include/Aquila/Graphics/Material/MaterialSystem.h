#ifndef AQUILA_MATERIAL_SYSTEM_H
#define AQUILA_MATERIAL_SYSTEM_H

#include "Aquila/Graphics/Core/Device.h"
#include "Aquila/Graphics/Material/MaterialLibrary.h"
#include "Aquila/Graphics/Pipeline/Descriptor.h"
#include "Aquila/Graphics/Pipeline/DynamicRenderingHelper.h"
#include "Aquila/Graphics/Pipeline/Pipeline.h"
#include "Aquila/Graphics/Core/DeletionManager.h"
#include "Aquila/Scene/Scene.h"

namespace Aquila::Graphics::Material {

struct SceneMaterialData {
	// Map: material name -> per-frame descriptor sets
	std::unordered_map<std::string, std::vector<VkDescriptorSet>> materialDescriptorSets;

	std::unordered_set<std::string> dirtyMaterials;

	bool needsUpdate = true;
};

class MaterialSystem {
  private:
	Device &m_Device;
	MaterialLibrary m_Library;

	std::unordered_map<std::string, Unique<RenderingPipeline::Pipeline>> m_Pipelines;
	std::unordered_map<std::string, std::vector<std::string>> m_ShaderToMaterials;

	RenderingPipeline::DescriptorSetLayout *m_GlobalLayout = nullptr;

	Delegate<void(const Ref<Material> &)> m_OnMaterialSaveRequested;

	// Not per-scene
	std::unordered_map<std::string, std::vector<VkDescriptorSet>> m_GlobalMaterialDescriptors;

	// Per-scene material descriptor sets
	std::unordered_map<SceneManagement::Scene *, SceneMaterialData> m_SceneMaterialData;

	void UpdateGlobalMaterialDescriptor(const Ref<Material> &material, uint32 frameIndex);
	void UpdateDescriptorSetForScene(const Ref<Material> &material, SceneManagement::Scene *scene, uint32 frameIndex);
	void UpdateGlobalEngineMaterials(uint32 frameIndex);
	SceneMaterialData &GetOrCreateSceneMaterialData(SceneManagement::Scene *scene);

  public:
	MaterialSystem(Device &device) : m_Device(device), m_Library(device) {};

	AQUILA_NONCOPYABLE(MaterialSystem);
	AQUILA_NONMOVEABLE(MaterialSystem);

	void Initialize(RenderingPipeline::DescriptorSetLayout *layout);

	void UpdatePipeline(const Ref<Material> &material, uint32 frameIndex);

	void Shutdown();
	void BuildShaderMaterialMapping();
	void TickHotReload(uint32 frameIndex);
	MaterialLibrary &GetLibrary();

	void AllocateGlobalMaterialDescriptors(const Ref<Material> &material);

	// Per-scene material management
	void AllocateDescriptorsForMaterial(const Ref<Material> &material, SceneManagement::Scene *scene);
	void AllocateDescriptorsForMaterialAllScenes(const Ref<Material> &material);
	void UpdateMaterialForScene(const Ref<Material> &material, SceneManagement::Scene *scene, uint32 frameIndex);
	void UpdateAllDirtyMaterialsForScene(SceneManagement::Scene *scene, uint32 frameIndex);
	void CleanupSceneMaterialData(SceneManagement::Scene *scene);

	VkDescriptorSet GetMaterialDescriptorSet(const Ref<Material> &material, SceneManagement::Scene *scene,
											 uint32 frameIndex);

	void SetMaterialSaveCallback(std::function<void(const Ref<Material> &)> callback) {
		m_OnMaterialSaveRequested = std::move(callback);
	}
};

} // namespace Aquila::Graphics::Material

#endif // AQUILA_MATERIAL_SYSTEM_H
