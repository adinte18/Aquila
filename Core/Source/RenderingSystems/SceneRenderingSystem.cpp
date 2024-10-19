//
// Created by alexa on 19/10/2024.
//

#include <utility>

#include "RenderingSystems/SceneRenderingSystem.h"

RenderingSystem::SceneRenderingSystem::SceneRenderingSystem(Engine::Device &device, VkRenderPass renderPass,
    std::vector<VkDescriptorSetLayout> setLayouts): device(device) {
    CreatePipelineLayout(std::move(setLayouts));
    CreatePipeline(renderPass);
}

void RenderingSystem::SceneRenderingSystem::Render() {

}

void RenderingSystem::SceneRenderingSystem::CreatePipeline(VkRenderPass renderPass) {
}

void RenderingSystem::SceneRenderingSystem::CreatePipelineLayout(std::vector<VkDescriptorSetLayout> setLayouts) {
}
