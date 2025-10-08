#include "RenderingSystems/SceneRenderingSystem.h"

#include "Engine/Controller.h"
#include "Engine/EditorCamera.h"

#include "Scene/Scene.h"

#include "Scene/Components/LightComponent.h"
#include "Scene/Components/MaterialComponent.h"
#include "Scene/Components/MeshComponent.h"
#include "Scene/Components/MetadataComponent.h"
#include "Scene/Components/TransformComponent.h"

namespace Engine {
struct PushConstantData {
  glm::mat4 modelMatrix{1.f};
  glm::mat4 normalMatrix{1.f};
};

SceneRenderSystem::SceneRenderSystem(Device &device, VkRenderPass renderPass)
    : RenderingSystemBase(device) {
  CreateDescriptorSetLayout();
  AllocateDescriptorSet();

  // init buffer
  m_Buffer = CreateUnique<Buffer>(
      device, "Scene_CameraInfoUBO", sizeof(SceneUniformData), 1,
      VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
  m_Buffer->Map();

  m_LightBuffer = CreateUnique<Buffer>(
      device, "Scene_LightUBO", sizeof(LightUniformData), 1,
      VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
  m_LightBuffer->Map();

  auto uniformData = m_Buffer->DescriptorInfo();
  auto lightUBO = m_LightBuffer->DescriptorInfo();

  SetUniformData(0, &uniformData);
  SetUniformData(1, &lightUBO);
  m_MaterialSystem = CreateUnique<MaterialSystem>(device);
  m_MaterialSystem->Initialize(renderPass, m_Layout.get());
}

void SceneRenderSystem::CreateDescriptorSetLayout() {
  m_Layout = DescriptorSetLayout::Builder(device)
                 .addBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                             VK_SHADER_STAGE_ALL_GRAPHICS)
                 .addBinding(1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                             VK_SHADER_STAGE_FRAGMENT_BIT)
                 .build();
}

void SceneRenderSystem::CreatePipeline(VkRenderPass renderPass) {
  // PipelineConfigInfo pipelineConfig{};
  // Pipeline::vk_DefaultPipelineConfig(pipelineConfig);
  // pipelineConfig.renderPass = renderPass;
  // pipelineConfig.pipelineLayout = m_PipelineLayout;
  // m_Pipeline = CreateUnique<Pipeline>(
  //     device, std::string(SHADERS_PATH) + "/AqPBRvert.spv",
  //     std::string(SHADERS_PATH) + "/AqPBRfrag.spv", pipelineConfig);
}

void SceneRenderSystem::CreatePipelineLayout() {
  VkPushConstantRange pushConstantRange{};
  pushConstantRange.stageFlags =
      VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
  pushConstantRange.offset = 0;
  pushConstantRange.size = sizeof(PushConstantData);

  auto setLayout = m_Layout->GetDescriptorSetLayout();

  VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
  pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
  pipelineLayoutInfo.setLayoutCount = 1;
  pipelineLayoutInfo.pSetLayouts = &setLayout;
  pipelineLayoutInfo.pushConstantRangeCount = 1;
  pipelineLayoutInfo.pPushConstantRanges = &pushConstantRange;
  if (vkCreatePipelineLayout(device.GetDevice(), &pipelineLayoutInfo, nullptr,
                             &m_PipelineLayout) != VK_SUCCESS) {
    throw std::runtime_error("failed to create pipeline layout!");
  }
}

void SceneRenderSystem::Update(EditorCamera &camera) {
  // Update scene
  SceneUniformData data{};
  data.projection = camera.GetProjection();
  data.view = camera.GetView();
  data.inverseView = camera.GetInverseView();

  m_Buffer->Write(&data);
  m_Buffer->Flush();

  std::vector<LightData> lights;

  auto *scene = Engine::Controller::Get()->GetSceneManager().GetActiveScene();
  auto &registry = scene->GetRegistry();

  auto view = registry.view<LightComponent, TransformComponent>();
  for (auto entity : view) {
    auto [light, transform] =
        view.get<LightComponent, TransformComponent>(entity);

    if (!light.m_IsActive)
      continue;

    LightData ld{};
    ld.color = light.m_Color;
    ld.intensity = light.m_Intensity;
    ld.direction = light.m_Direction;
    ld.range = light.m_Range;
    ld.position = vec3(transform.GetLocalPosition());
    ld.type = static_cast<int>(light.m_Type);
    ld.innerCone = light.m_InnerConeAngle;
    ld.outerCone = light.m_OuterConeAngle;
    ld.isActive = 1;

    lights.push_back(ld);
  }

  //
  LightUniformData lightUniformData{};
  lightUniformData.lightCount = static_cast<int>(lights.size());

  for (size_t i = 0; i < lights.size() && i < 32; ++i) {
    lightUniformData.lights[i] = lights[i];
  }

  m_LightBuffer->Write(&lightUniformData, sizeof(LightUniformData));
  m_LightBuffer->Flush();
}

void SceneRenderSystem::Render(const FrameSpec &frameSpec) {
  auto *scene = Controller::Get()->GetSceneManager().GetActiveScene();
  auto &registry = scene->GetRegistry();

  if (m_MaterialSystem) {
    m_MaterialSystem->UpdateAllDirtyMaterials(m_Layout.get());
  }

  std::unordered_map<Ref<Material::Material>, std::vector<entt::entity>>
      materialGroups;

  auto view = registry.view<MetadataComponent, MeshComponent,
                            TransformComponent, MaterialComponent>();

  for (auto entity : view) {
    auto &metaComp = view.get<MetadataComponent>(entity);
    if (!metaComp.Visible)
      continue;

    auto &matComp = view.get<MaterialComponent>(entity);
    auto material = matComp.GetMaterial();

    if (!material && m_MaterialSystem) {
      material = m_MaterialSystem->GetLibrary().GetFallbackMaterial();
    }

    if (material) {
      materialGroups[material].push_back(entity);
    }
  }

  for (auto &[material, entities] : materialGroups) {
    auto pipeline = material->GetPipeline();
    if (pipeline == VK_NULL_HANDLE) {
      AQUILA_LOG_WARNING("Material {} has no pipeline, skipping",
                         material->name);
      continue;
    }

    vkCmdBindPipeline(frameSpec.commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                      pipeline);

    auto matDescriptor = material->GetDescriptorSet();
    if (matDescriptor != VK_NULL_HANDLE) {
      VkDescriptorSet descriptorSets[2] = {
          m_DescriptorSets[frameSpec.frameIndex],

          matDescriptor};

      vkCmdBindDescriptorSets(frameSpec.commandBuffer,
                              VK_PIPELINE_BIND_POINT_GRAPHICS,
                              material->GetTemplate()->shader->m_Layout, 0, 2,
                              descriptorSets, 0, nullptr);
    } else {

      vkCmdBindDescriptorSets(
          frameSpec.commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
          material->GetTemplate()->shader->m_Layout, 0, 1,
          &m_DescriptorSets[frameSpec.frameIndex], 0, nullptr);
    }

    for (auto entity : entities) {
      auto [meshComp, transformComp] =
          view.get<MeshComponent, TransformComponent>(entity);

      PushConstantData push{};
      push.modelMatrix = transformComp.GetWorldMatrix();
      push.normalMatrix = transformComp.GetNormalMatrix();

      vkCmdPushConstants(
          frameSpec.commandBuffer, material->GetTemplate()->shader->m_Layout,
          VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0,
          sizeof(PushConstantData), &push);

      if (meshComp.data) {
        meshComp.data->Bind(frameSpec.commandBuffer);
        meshComp.data->Draw(frameSpec.commandBuffer);
      }
    }
  }
}

} // namespace Engine