#include "RenderingSystems/DepthRenderingSystem.h"

namespace Engine {

    struct PushConstantData
    {
        glm::mat4 modelMatrix{1.f};
        glm::mat4 normalMatrix{1.f};
    };

    DepthRenderingSystem::DepthRenderingSystem(Engine::Device &device, VkRenderPass renderPass,
                                                                std::array<VkDescriptorSetLayout, 2> setLayouts): device(device) {
        CreatePipelineLayout(setLayouts);
        CreatePipeline(renderPass);
    }

    void DepthRenderingSystem::Render(VkCommandBuffer commandBuffer) const {
        pipeline->Bind(commandBuffer);

        // sceneContext.GetScene().GetRegistry().view<Engine::Light>().each([&sceneContext, this, &commandBuffer](Engine::Light& light) {
        //     if (light.type == Engine::Light::LightType::Directional) {

        //         light.UpdateMatrices();

        //         sceneContext.GetScene().GetRegistry().view<Engine::Transform, Engine::MeshCmp>()
        //             .each([&sceneContext, this, &commandBuffer](Engine::Transform& transform, Engine::MeshCmp& mesh) {

        //             if (mesh.mesh) {
        //                 PushConstantData push{};
        //                 push.modelMatrix = transform.TransformMatrix();
        //                 push.normalMatrix = transform.NormalMatrix();

        //                 vkCmdPushConstants(commandBuffer,
        //                     pipelineLayout,
        //                     VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
        //                     0,
        //                     sizeof(PushConstantData),
        //                     &push);

        //                 mesh.mesh->Bind(commandBuffer);
        //                 // mesh.mesh->Draw(commandBuffer, sceneContext.GetSceneDescriptorSet(), pipelineLayout);
        //             }
        //         });
        //     }
        // });
    }

    void DepthRenderingSystem::RecreatePipeline(VkRenderPass renderPass) {
        if (pipeline) {
            pipeline.reset();
        }

        CreatePipeline(renderPass);
    }


    void DepthRenderingSystem::CreatePipeline(VkRenderPass renderPass) {
        AQUILA_CORE_ASSERT(pipelineLayout != nullptr && "Cannot create pipeline before pipeline layout");

        Engine::PipelineConfigInfo pipelineConfig{};
        Engine::Pipeline::vk_DefaultPipelineConfig(pipelineConfig);
        pipelineConfig.renderPass = renderPass;
        pipelineConfig.pipelineLayout = pipelineLayout;
        pipelineConfig.colorBlendAttachment.blendEnable = VK_FALSE;
        pipeline = CreateUnique<Engine::Pipeline>(
                device,
                std::string(SHADERS_PATH) + "/shadows_vert.spv",
                std::string(SHADERS_PATH) + "/shadows_frag.spv",
                pipelineConfig);

    }

    void DepthRenderingSystem::CreatePipelineLayout(std::array<VkDescriptorSetLayout, 2> setLayouts) {
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
        if (vkCreatePipelineLayout(device.GetDevice(), &pipelineLayoutInfo, nullptr, &pipelineLayout) != VK_SUCCESS) {
            throw std::runtime_error("failed to create pipeline layout!");
        }
    }

}
