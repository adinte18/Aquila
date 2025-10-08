#ifndef AQUILA_MATERIAL_SYSTEM_H
#define AQUILA_MATERIAL_SYSTEM_H

#include "Engine/Renderer/Descriptor.h"
#include "Engine/Renderer/Device.h"
#include "Engine/Renderer/Material/MaterialLibrary.h"
#include "Engine/Renderer/Pipeline.h"

namespace Engine {
class MaterialSystem {
private:
  Device &m_Device;
  Material::MaterialLibrary m_Library;
  VkRenderPass m_RenderPass;
  std::unordered_map<std::string, Unique<Pipeline>> m_Pipelines;

  void UpdateDescriptorSet(Ref<Material::Material> material);
  void UpdatePipeline(DescriptorSetLayout *globalSet,
                      Ref<Material::Material> material);

public:
  MaterialSystem(Device &device) : m_Device(device), m_Library(device) {};

  void Initialize(VkRenderPass renderPass, DescriptorSetLayout *layout);

  void Shutdown();

  Material::MaterialLibrary &GetLibrary();

  VkRenderPass GetActiveRenderPass();

  void SetRenderPass(VkRenderPass renderPass) { m_RenderPass = renderPass; }

  void UpdateMaterial(Ref<Material::Material> material,
                      DescriptorSetLayout *layout);

  VkPipelineLayout
  CreatePipelineLayoutForMaterial(Ref<Material::Material> material);

  void UpdateAllDirtyMaterials(DescriptorSetLayout *layout);
};
} // namespace Engine

#endif