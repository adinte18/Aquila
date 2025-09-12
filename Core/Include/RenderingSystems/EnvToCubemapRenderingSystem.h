//
// Created by alexa on 08/04/2025.
//

#ifndef CUBEMAPRENDERINGSYSTEM_H
#define CUBEMAPRENDERINGSYSTEM_H

#include <Engine/Mesh.h>
#include <Engine/Renderer/Device.h>
#include <Engine/Renderer/Pipeline.h>


namespace Engine {
class EnvToCubemapRenderingSystem {
public:
  EnvToCubemapRenderingSystem(Device &device, VkRenderPass renderPass,
                              VkDescriptorSetLayout layout);
  ~EnvToCubemapRenderingSystem() {
    vkDestroyPipelineLayout(device.GetDevice(), pipelineLayout, nullptr);
  };

  EnvToCubemapRenderingSystem(const EnvToCubemapRenderingSystem &) = delete;
  EnvToCubemapRenderingSystem &
  operator=(const EnvToCubemapRenderingSystem &) = delete;

  void Render(VkCommandBuffer commandBuffer, VkDescriptorSet descriptorSet,
              glm::mat4 &viewMatrix);
  void RecreatePipeline(VkRenderPass renderPass);

private:
  Device &device;

  Unique<Pipeline> pipeline;
  VkPipelineLayout pipelineLayout;
  Ref<Mesh> model;

  void CreatePipeline(VkRenderPass renderPass);
  void CreatePipelineLayout(VkDescriptorSetLayout layout);
};
} // namespace Engine

#endif // CUBEMAPRENDERINGSYSTEM_H
