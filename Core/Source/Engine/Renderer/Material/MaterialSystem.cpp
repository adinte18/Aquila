#include "Engine/Renderer/Material/MaterialSystem.h"
#include "Engine/Controller.h"
#include "Engine/Renderer/Descriptor.h"
#include "RenderingSystems/SceneRenderingSystem.h"

namespace Engine {

void MaterialSystem::UpdateDescriptorSet(Ref<Material::Material> material) {
  if (material->GetDescriptorSet() == VK_NULL_HANDLE) {
    auto tmpl = material->GetTemplate();
    if (!tmpl || !tmpl->shader ||
        tmpl->shader->m_DescriptorSetLayout == VK_NULL_HANDLE) {
      return;
    }

    VkDescriptorSet descriptorSet;
    if (!DescriptorAllocator::Allocate(
            tmpl->shader->m_DescriptorSetLayout->GetDescriptorSetLayout(),
            descriptorSet)) {
      AQUILA_LOG_ERROR("Failed to allocate descriptor set for material: {}",
                       material->name);
      return;
    }
    material->SetDescriptorSet(descriptorSet);
  }

  auto tmpl = material->GetTemplate();
  if (!tmpl || !tmpl->shader)
    return;

  DescriptorWriter writer(*tmpl->shader->m_DescriptorSetLayout,
                          *DescriptorAllocator::GetSharedPool());

  std::map<int, std::pair<std::string, Material::MaterialParameter>>
      textureBindings;

  for (const auto &[name, prop] : tmpl->properties) {
    if (prop.m_Type == Material::ParameterType::Texture2D &&
        prop.m_BindingIndex >= 0) {
      textureBindings[prop.m_BindingIndex] = {name, prop};
    }
  }

  for (const auto &[binding, namePropPair] : textureBindings) {
    const auto &[name, prop] = namePropPair;
    auto texture = material->GetTexture(name);

    if (!texture) {
      texture = GetLibrary().GetFallbackTextureForType(name);
    }

    if (texture) {
      VkDescriptorImageInfo imageInfo = texture->GetDescriptorSetInfo();
      writer.writeImage(binding, &imageInfo);
    }
  }

  if (material->GetMaterialUBO()) {
    int uboBinding = textureBindings.size();
    VkDescriptorBufferInfo bufferInfo =
        material->GetMaterialUBO()->DescriptorInfo();
    writer.writeBuffer(uboBinding, &bufferInfo);
  }

  auto set = material->GetDescriptorSet();
  writer.overwrite(set);
}

struct PushConstantData {
  glm::mat4 modelMatrix{1.f};
  glm::mat4 normalMatrix{1.f};
};

void MaterialSystem::UpdatePipeline(DescriptorSetLayout *globalSet,
                                    Ref<Material::Material> material) {
  PipelineConfigInfo configInfo{};
  Pipeline::vk_DefaultPipelineConfig(configInfo);
  configInfo.renderPass = m_RenderPass;

  auto shader = material->GetTemplate()->shader;
  if (!shader)
    return;

  VkDescriptorSetLayout setLayouts[2] = {
      globalSet->GetDescriptorSetLayout(),
      shader->m_DescriptorSetLayout->GetDescriptorSetLayout()};

  VkPushConstantRange pushConstantRange{};
  pushConstantRange.stageFlags =
      VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
  pushConstantRange.offset = 0;
  pushConstantRange.size = sizeof(PushConstantData);

  VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
  pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
  pipelineLayoutInfo.setLayoutCount = 2;
  pipelineLayoutInfo.pSetLayouts = setLayouts;
  pipelineLayoutInfo.pushConstantRangeCount = 1;
  pipelineLayoutInfo.pPushConstantRanges = &pushConstantRange;
  if (vkCreatePipelineLayout(m_Device.GetDevice(), &pipelineLayoutInfo, nullptr,
                             &shader->m_Layout) != VK_SUCCESS) {
    throw std::runtime_error("failed to create pipeline layout!");
  }

  auto pipeline = CreateUnique<Pipeline>(m_Device, material, configInfo);

  material->SetPipeline(pipeline->GetPipeline());

  m_Pipelines[material->name] = std::move(pipeline);

  AQUILA_LOG_DEBUG("Created pipeline for material : {}", material->name);
}

void MaterialSystem::Initialize(VkRenderPass renderPass,
                                DescriptorSetLayout *layout) {
  m_RenderPass = renderPass;
  m_Library.Initialize();

  for (const auto &[name, mat] : m_Library.GetAllMaterials()) {
    UpdateMaterial(mat, layout);
  }
}

void MaterialSystem::Shutdown() {
  m_Pipelines.clear();
  m_Library.Shutdown();
}

Material::MaterialLibrary &MaterialSystem::GetLibrary() { return m_Library; }

void MaterialSystem::UpdateMaterial(Ref<Material::Material> material,
                                    DescriptorSetLayout *layout) {
  if (!material || !material->IsDirty())
    return;

  if (material->IsDescriptorDirty()) {
    UpdateDescriptorSet(material);
  }

  if (material->IsPipelineDirty() && !m_Pipelines[material->name]) {
    UpdatePipeline(layout, material);
  }

  material->MarkClean();
}

void MaterialSystem::UpdateAllDirtyMaterials(DescriptorSetLayout *layout) {
  for (const auto &[name, mat] : m_Library.GetAllMaterials()) {
    UpdateMaterial(mat, layout);
  }
}
} // namespace Engine