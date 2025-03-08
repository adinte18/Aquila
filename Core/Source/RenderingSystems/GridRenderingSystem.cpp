//
// Created by alexa on 15/02/2025.
//

#include "RenderingSystems/GridRenderingSystem.h"


#include <utility>
#include "Components.h"

struct PushConstantData
{
    glm::mat4 modelMatrix{1.f}; //identity
    glm::mat4 normalMatrix{1.f};
};

RenderingSystem::GridRenderingSystem::GridRenderingSystem(Engine::Device &device, VkRenderPass renderPass,
                                                            std::array<VkDescriptorSetLayout, 2> setLayouts): device(device) {
    CreatePipelineLayout(setLayouts);
    CreatePipeline(renderPass);
    model->CreateQuad(1000.f);
}

void RenderingSystem::GridRenderingSystem::Render(VkCommandBuffer commandBuffer, ECS::Scene& scene) {
    pipeline->bind(commandBuffer);

    PushConstantData push{};
    push.modelMatrix = glm::mat4(1.f);
    push.normalMatrix = glm::mat4(1.f);

    vkCmdPushConstants(commandBuffer,
        pipelineLayout,
        VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
        0,
        sizeof(PushConstantData),
        &push);

    model->bind(commandBuffer);
    model->draw(commandBuffer, scene.sceneDescriptorSet, pipelineLayout);
}


void RenderingSystem::GridRenderingSystem::CreatePipeline(VkRenderPass renderPass) {
    assert(pipelineLayout != nullptr && "Cannot create pipeline before pipeline layout");

    Engine::PipelineConfigInfo pipelineConfig{};
    Engine::Pipeline::vk_DefaultPipelineConfig(pipelineConfig);
    pipelineConfig.renderPass = renderPass;
    pipelineConfig.pipelineLayout = pipelineLayout;
    pipeline = std::make_unique<Engine::Pipeline>(
            device,
            std::string(SHADERS_PATH) + "/grid_vert.spv",
            std::string(SHADERS_PATH) + "/grid_frag.spv",
            pipelineConfig);

}

void RenderingSystem::GridRenderingSystem::CreatePipelineLayout(std::array<VkDescriptorSetLayout, 2> setLayouts) {
    VkPushConstantRange pushConstantRange{};
    pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
    pushConstantRange.offset = 0;
    pushConstantRange.size = sizeof(PushConstantData);


    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = static_cast<uint32_t>(setLayouts.size());
    pipelineLayoutInfo.pSetLayouts = setLayouts.data();
    pipelineLayoutInfo.pushConstantRangeCount = 1;
    pipelineLayoutInfo.pPushConstantRanges = &pushConstantRange;
    if (vkCreatePipelineLayout(device.vk_GetDevice(), &pipelineLayoutInfo, nullptr, &pipelineLayout) != VK_SUCCESS) {
        throw std::runtime_error("failed to create pipeline layout!");
    }
}
