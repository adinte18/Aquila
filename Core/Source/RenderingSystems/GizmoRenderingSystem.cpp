#include "RenderingSystems/GizmoRenderingSystem.h"
#include "Engine/Controller.h"
#include "Engine/Renderer/Swapchain.h"
#include "Scene/Components/CameraComponent.h"
#include "Scene/Components/LightComponent.h"
#include "Scene/Components/MeshComponent.h"
#include "Scene/Components/MetadataComponent.h"
#include "Scene/Components/TransformComponent.h"
#include "vulkan/vulkan_core.h"

namespace Engine {

GizmoRenderSystem::GizmoRenderSystem(Device &device, VkRenderPass renderPass)
    : RenderingSystemBase(device) {
  CreateDescriptorSetLayout();
  AllocateDescriptorSet();

  m_Buffer = CreateUnique<Buffer>(
      device, "Gizmo_UniformUBO", sizeof(GizmoUniformData), 1,
      VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
  m_Buffer->Map();

  auto uniformData = m_Buffer->DescriptorInfo();
  SetUniformData(0, &uniformData);

  CreatePipelineLayout();
  CreatePipeline(renderPass);

  m_VertexBuffers.resize(Swapchain::MAX_FRAMES_IN_FLIGHT);
  m_IndexBuffers.resize(Swapchain::MAX_FRAMES_IN_FLIGHT);

  m_Vertices.reserve(10000);
  m_Indices.reserve(20000);
}

void GizmoRenderSystem::CreateDescriptorSetLayout() {
  m_Layout = DescriptorSetLayout::Builder(device)
                 .addBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                             VK_SHADER_STAGE_VERTEX_BIT)
                 .build();
}

void GizmoRenderSystem::CreatePipeline(VkRenderPass renderPass) {
  PipelineConfigInfo pipelineConfig{};
  Pipeline::vk_DefaultPipelineConfig(pipelineConfig);

  pipelineConfig.renderPass = renderPass;
  pipelineConfig.pipelineLayout = m_PipelineLayout;
  pipelineConfig.inputAssemblyInfo.topology = VK_PRIMITIVE_TOPOLOGY_LINE_LIST;
  pipelineConfig.rasterizationInfo.lineWidth = 0.5f;
  pipelineConfig.colorBlendAttachment.blendEnable = VK_FALSE;

  m_Pipeline = CreateUnique<Pipeline>(
      device, std::string(SHADERS_PATH) + "/guizmo_vert.spv",
      std::string(SHADERS_PATH) + "/guizmo_frag.spv", pipelineConfig);
}

void GizmoRenderSystem::CreatePipelineLayout() {
  auto setLayout = m_Layout->GetDescriptorSetLayout();

  VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
  pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
  pipelineLayoutInfo.setLayoutCount = 1;
  pipelineLayoutInfo.pSetLayouts = &setLayout;
  pipelineLayoutInfo.pushConstantRangeCount = 0;
  pipelineLayoutInfo.pPushConstantRanges = nullptr;

  if (vkCreatePipelineLayout(device.GetDevice(), &pipelineLayoutInfo, nullptr,
                             &m_PipelineLayout) != VK_SUCCESS) {
    throw std::runtime_error("failed to create gizmo pipeline layout!");
  }
}

void GizmoRenderSystem::Update(EditorCamera &camera) {
  GizmoUniformData data{};
  data.view = camera.GetView();
  data.projection = camera.GetProjection();

  m_Buffer->Write(&data, sizeof(GizmoUniformData));
  m_Buffer->Flush();

  Clear();

  auto *scene = Engine::Controller::Get()->GetSceneManager().GetActiveScene();
  auto &registry = scene->GetRegistry();

  auto view = registry.view<MetadataComponent, TransformComponent>();
  for (auto entity : view) {
    if (registry.any_of<LightComponent>(entity))
      continue;

    auto [metadata, transform] =
        view.get<MetadataComponent, TransformComponent>(entity);

    if (metadata.Visible) {
      vec3 pos = transform.GetLocalPosition();
      vec3 scale = transform.GetLocalScale();
      quat rotation = transform.GetLocalRotation();

      if (registry.any_of<MeshComponent>(entity)) {
        if (metadata.Selected) {
          AddWireframeBox(pos, scale * 2.0f, rotation,
                          vec3(0.259f, 0.961f, 0.471f));
        }
      }

      if (registry.any_of<CameraComponent>(entity)) {
        CameraComponent &camera = registry.get<CameraComponent>(entity);

        mat4 viewMatrix = camera.GetViewMatrix(transform.GetLocalPosition(),
                                               transform.GetLocalRotation());
        mat4 projMatrix = camera.GetProjectionMatrix();
        AddCameraFrustum(viewMatrix, projMatrix, vec3(1.0f, 1.0f, 0.0f),
                         transform.GetLocalPosition());
      }
    }
  }
  m_BuffersNeedUpdate = true;
}

void GizmoRenderSystem::Render(const FrameSpec &frameSpec) {
  if (m_Vertices.empty())
    return;

  if (m_BuffersNeedUpdate) {
    UpdateBuffers(frameSpec.frameIndex);
    m_BuffersNeedUpdate = false;
  }

  m_Pipeline->Bind(frameSpec.commandBuffer);

  vkCmdBindDescriptorSets(frameSpec.commandBuffer,
                          VK_PIPELINE_BIND_POINT_GRAPHICS, m_PipelineLayout, 0,
                          1, &m_DescriptorSets[frameSpec.frameIndex], 0,
                          nullptr);
  vkCmdSetLineWidth(frameSpec.commandBuffer, 0.5f);

  VkBuffer vertexBuffers[] = {
      m_VertexBuffers[frameSpec.frameIndex]->GetBuffer()};
  VkDeviceSize offsets[] = {0};
  vkCmdBindVertexBuffers(frameSpec.commandBuffer, 0, 1, vertexBuffers, offsets);

  vkCmdBindIndexBuffer(frameSpec.commandBuffer,
                       m_IndexBuffers[frameSpec.frameIndex]->GetBuffer(), 0,
                       VK_INDEX_TYPE_UINT32);
  vkCmdDrawIndexed(frameSpec.commandBuffer, m_CurrentIndexCount, 1, 0, 0, 0);
}

void GizmoRenderSystem::Clear() {
  m_Vertices.clear();
  m_Indices.clear();
  m_CurrentVertexCount = 0;
  m_CurrentIndexCount = 0;
  m_BuffersNeedUpdate = true;
}

void GizmoRenderSystem::AddLine(const vec3 &start, const vec3 &end,
                                const vec3 &color) {
  uint32_t startIndex = m_CurrentVertexCount;

  Vertex startVertex{};
  startVertex.pos = start;
  startVertex.color = color;

  Vertex endVertex{};
  endVertex.pos = end;
  endVertex.color = color;

  m_Vertices.push_back(startVertex);
  m_Vertices.push_back(endVertex);

  m_Indices.push_back(startIndex);
  m_Indices.push_back(startIndex + 1);

  m_CurrentVertexCount += 2;
  m_CurrentIndexCount += 2;
  m_BuffersNeedUpdate = true;
}

void GizmoRenderSystem::AddWireframeBox(const vec3 &center, const vec3 &size,
                                        const quat &rotation,
                                        const vec3 &color) {

  vec3 halfSize = size * 0.5f;
  vec3 corners[8] = {vec3(-halfSize.x, -halfSize.y, -halfSize.z),
                     vec3(halfSize.x, -halfSize.y, -halfSize.z),
                     vec3(halfSize.x, -halfSize.y, halfSize.z),
                     vec3(-halfSize.x, -halfSize.y, halfSize.z),
                     vec3(-halfSize.x, halfSize.y, -halfSize.z),
                     vec3(halfSize.x, halfSize.y, -halfSize.z),
                     vec3(halfSize.x, halfSize.y, halfSize.z),
                     vec3(-halfSize.x, halfSize.y, halfSize.z)};

  for (int i = 0; i < 8; i++) {
    corners[i] = center + (rotation * corners[i]);
  }

  AddLine(corners[0], corners[1], color);
  AddLine(corners[1], corners[2], color);
  AddLine(corners[2], corners[3], color);
  AddLine(corners[3], corners[0], color);

  AddLine(corners[4], corners[5], color);
  AddLine(corners[5], corners[6], color);
  AddLine(corners[6], corners[7], color);
  AddLine(corners[7], corners[4], color);

  AddLine(corners[0], corners[4], color);
  AddLine(corners[1], corners[5], color);
  AddLine(corners[2], corners[6], color);
  AddLine(corners[3], corners[7], color);
}

void GizmoRenderSystem::AddCoordinateAxes(const vec3 &origin, f32 length) {
  AddLine(origin, origin + vec3(length, 0, 0), vec3(1, 0, 0));
  AddLine(origin, origin + vec3(0, length, 0), vec3(0, 1, 0));
  AddLine(origin, origin + vec3(0, 0, length), vec3(0, 0, 1));
}

void GizmoRenderSystem::AddCameraFrustum(const mat4 &viewMatrix,
                                         const mat4 &projMatrix,
                                         const vec3 &color,
                                         const vec3 &cameraPos) {

  mat4 invVP = inverse(projMatrix * viewMatrix);

  vec4 frustumCorners[8] = {
      {-1, -1, 0, 1}, {1, -1, 0, 1}, {1, 1, 0, 1}, {-1, 1, 0, 1},

      {-1, -1, 1, 1}, {1, -1, 1, 1}, {1, 1, 1, 1}, {-1, 1, 1, 1}};

  vec3 worldCorners[8];
  for (int i = 0; i < 8; ++i) {
    vec4 worldPos = invVP * frustumCorners[i];
    worldCorners[i] = vec3(worldPos) / worldPos.w;
  }

  vec3 nearColor = vec3(1, 0, 0);
  AddLine(worldCorners[0], worldCorners[1], nearColor);
  AddLine(worldCorners[1], worldCorners[2], nearColor);
  AddLine(worldCorners[2], worldCorners[3], nearColor);
  AddLine(worldCorners[3], worldCorners[0], nearColor);

  vec3 farColor = vec3(0, 1, 0);
  AddLine(worldCorners[4], worldCorners[5], farColor);
  AddLine(worldCorners[5], worldCorners[6], farColor);
  AddLine(worldCorners[6], worldCorners[7], farColor);
  AddLine(worldCorners[7], worldCorners[4], farColor);

  vec3 connectColor = vec3(0, 0, 1);
  AddLine(worldCorners[0], worldCorners[4], connectColor);
  AddLine(worldCorners[1], worldCorners[5], connectColor);
  AddLine(worldCorners[2], worldCorners[6], connectColor);
  AddLine(worldCorners[3], worldCorners[7], connectColor);
}

void GizmoRenderSystem::UpdateBuffers(uint32 frameIndex) {
  if (m_Vertices.empty())
    return;

  VkDeviceSize vertexBufferSize = sizeof(Vertex) * m_Vertices.size();
  VkDeviceSize indexBufferSize = sizeof(uint32_t) * m_Indices.size();

  auto &currentVertexBuffer = m_VertexBuffers[frameIndex];
  auto &currentIndexBuffer = m_IndexBuffers[frameIndex];

  currentVertexBuffer =
      CreateUnique<Buffer>(device, "Gizmo_VertexBuffer", vertexBufferSize, 1,
                           VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                           VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                               VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

  currentVertexBuffer->Map();
  currentVertexBuffer->Write(m_Vertices.data());
  currentVertexBuffer->Flush();

  currentIndexBuffer =
      CreateUnique<Buffer>(device, "Gizmo_IndexBuffer", indexBufferSize, 1,
                           VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
                           VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                               VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

  currentIndexBuffer->Map();
  currentIndexBuffer->Write(m_Indices.data());
  currentIndexBuffer->Flush();
}

} // namespace Engine