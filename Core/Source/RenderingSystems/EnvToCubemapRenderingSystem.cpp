#include <utility>

#include "RenderingSystems/EnvToCubemapRenderingSystem.h"
#include "Engine/Model.h"
#include "Components.h"


struct PushConstantData
{
    glm::mat4 viewMatrix{1.f};
    glm::mat4 projectionMatrix{1.f};
};

RenderingSystem::EnvToCubemapRenderingSystem::EnvToCubemapRenderingSystem(Engine::Device &device, VkRenderPass renderPass,
                                                            VkDescriptorSetLayout layout): device(device) {
    CreatePipelineLayout(layout);
    CreatePipeline(renderPass);
    model = std::make_shared<Engine::Model3D>(device);
    model->CreateCube();
}

void RenderingSystem::EnvToCubemapRenderingSystem::Render(VkCommandBuffer commandBuffer, VkDescriptorSet descriptorSet, glm::mat4& viewMatrix) {
    pipeline->bind(commandBuffer);

    PushConstantData push{};
    push.viewMatrix = viewMatrix;
    push.projectionMatrix = glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, 10.0f);

    vkCmdPushConstants(commandBuffer,
        pipelineLayout,
        VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
        0,
        sizeof(PushConstantData),
        &push);

    model->bind(commandBuffer);
    model->draw(commandBuffer, descriptorSet, pipelineLayout);
}

void RenderingSystem::EnvToCubemapRenderingSystem::CreatePipeline(VkRenderPass renderPass) {
    assert(pipelineLayout != nullptr && "Cannot create pipeline before pipeline layout");

    Engine::PipelineConfigInfo pipelineConfig{};
    Engine::Pipeline::vk_DefaultPipelineConfig(pipelineConfig);
    pipelineConfig.renderPass = renderPass;
    pipelineConfig.pipelineLayout = pipelineLayout;
    pipeline = std::make_unique<Engine::Pipeline>(
            device,
            std::string(SHADERS_PATH) + "/cubemap_vert.spv",
            std::string(SHADERS_PATH) + "/cubemap_frag.spv",
            pipelineConfig);
}

void RenderingSystem::EnvToCubemapRenderingSystem::RecreatePipeline(VkRenderPass renderPass) {
    if (pipeline) {
        pipeline.reset();
    }

    CreatePipeline(renderPass);
}


void RenderingSystem::EnvToCubemapRenderingSystem::CreatePipelineLayout(VkDescriptorSetLayout layout) {
    VkPushConstantRange pushConstantRange{};
    pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
    pushConstantRange.offset = 0;
    pushConstantRange.size = sizeof(PushConstantData);


    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = 1;
    pipelineLayoutInfo.pSetLayouts = &layout;
    pipelineLayoutInfo.pushConstantRangeCount = 1;
    pipelineLayoutInfo.pPushConstantRanges = &pushConstantRange;
    if (vkCreatePipelineLayout(device.vk_GetDevice(), &pipelineLayoutInfo, nullptr, &pipelineLayout) != VK_SUCCESS) {
        throw std::runtime_error("failed to create pipeline layout!");
    }
}
