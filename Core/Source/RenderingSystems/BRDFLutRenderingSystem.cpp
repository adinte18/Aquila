#include <utility>
#include "RenderingSystems/BRDFLutRenderingSystem.h"


namespace Engine {
    BRDFLutRenderingSystem::BRDFLutRenderingSystem(Device &device, VkRenderPass renderPass,
                                                               VkDescriptorSetLayout layout): device(device) {
        CreatePipelineLayout(layout);
        CreatePipeline(renderPass);
    }

    void BRDFLutRenderingSystem::Render(VkCommandBuffer commandBuffer, VkDescriptorSet& descriptorSet) {
        pipeline->Bind(commandBuffer);

        if (descriptorSet == VK_NULL_HANDLE) {
            throw std::runtime_error("Final descriptor set is NULL!");
        }

        vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout,
                                0, 1, &descriptorSet, 0, nullptr);

        vkCmdDraw(commandBuffer, 3, 1, 0, 0);
    }

    void BRDFLutRenderingSystem::RecreatePipeline(VkRenderPass renderPass) {
        if (pipeline) {
            pipeline.reset();
        }

        CreatePipeline(renderPass);
    }


    void BRDFLutRenderingSystem::CreatePipeline(VkRenderPass renderPass) {
        AQUILA_CORE_ASSERT(pipelineLayout != nullptr && "Cannot create pipeline before pipeline layout");

        Engine::PipelineConfigInfo pipelineConfig{};
        Engine::Pipeline::vk_DefaultPipelineConfig(pipelineConfig);
        pipelineConfig.renderPass = renderPass;
        pipelineConfig.pipelineLayout = pipelineLayout;
        pipeline = CreateUnique<Engine::Pipeline>(
                device,
                std::string(SHADERS_PATH) + "/brdflut_vert.spv",
                std::string(SHADERS_PATH) + "/brdflut_frag.spv",
                pipelineConfig);

    }

    void BRDFLutRenderingSystem::CreatePipelineLayout(VkDescriptorSetLayout layout) {
        VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
        pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pipelineLayoutInfo.setLayoutCount = 1;
        pipelineLayoutInfo.pSetLayouts = &layout;
        if (vkCreatePipelineLayout(device.GetDevice(), &pipelineLayoutInfo, nullptr, &pipelineLayout) != VK_SUCCESS) {
            throw std::runtime_error("failed to create pipeline layout!");
        }
    }

}