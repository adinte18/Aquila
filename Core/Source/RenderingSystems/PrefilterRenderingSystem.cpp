//
// Created by alexa on 10/04/2025.
//

#include "RenderingSystems/PrefilterRenderingSystem.h"

namespace Engine {

    struct PushConstantData {
        glm::mat4 viewMatrix;
        glm::mat4 projectionMatrix;
        uint32_t mipLevel;
    };

    PrefilterRenderingSystem::PrefilterRenderingSystem(Engine::Device &device, VkRenderPass renderPass,
                                                                VkDescriptorSetLayout layout): device(device) {
        CreatePipelineLayout(layout);
        CreatePipeline(renderPass);
        model = std::make_shared<Engine::Mesh>(device);
        // model->CreateCube();
    }

    void PrefilterRenderingSystem::Render(VkCommandBuffer commandBuffer, VkDescriptorSet descriptorSet, glm::mat4& viewMatrix, uint32_t mipLevel) {
        pipeline->Bind(commandBuffer);

        PushConstantData push{};
        push.viewMatrix = viewMatrix;
        push.projectionMatrix = glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, 10.0f);
        push.mipLevel = mipLevel;

        vkCmdPushConstants(commandBuffer,
            pipelineLayout,
            VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
            0,
            sizeof(PushConstantData),
            &push);

        model->Bind(commandBuffer);
        // model->Draw(commandBuffer, descriptorSet, pipelineLayout);
    }

    void PrefilterRenderingSystem::CreatePipeline(VkRenderPass renderPass) {
        AQUILA_CORE_ASSERT(pipelineLayout != nullptr && "Cannot create pipeline before pipeline layout");

        Engine::PipelineConfigInfo pipelineConfig{};
        Engine::Pipeline::vk_DefaultPipelineConfig(pipelineConfig);
        pipelineConfig.renderPass = renderPass;
        pipelineConfig.pipelineLayout = pipelineLayout;
        pipeline = std::make_unique<Engine::Pipeline>(
                device,
                std::string(SHADERS_PATH) + "/prefilter_vert.spv",
                std::string(SHADERS_PATH) + "/prefilter_frag.spv",
                pipelineConfig);
    }

    void PrefilterRenderingSystem::RecreatePipeline(VkRenderPass renderPass) {
        if (pipeline) {
            pipeline.reset();
        }

        CreatePipeline(renderPass);
    }


    void PrefilterRenderingSystem::CreatePipelineLayout(VkDescriptorSetLayout layout) {
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
        if (vkCreatePipelineLayout(device.GetDevice(), &pipelineLayoutInfo, nullptr, &pipelineLayout) != VK_SUCCESS) {
            throw std::runtime_error("failed to create pipeline layout!");
        }
    }
}
