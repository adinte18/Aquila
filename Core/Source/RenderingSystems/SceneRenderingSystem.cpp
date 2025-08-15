#include <utility>

#include "RenderingSystems/SceneRenderingSystem.h"

#include "Engine/Mesh.h"



namespace Engine {

    struct PushConstantData
    {
        glm::mat4 modelMatrix{1.f}; //identity
        glm::mat4 normalMatrix{1.f};
    };

    SceneRenderingSystem::SceneRenderingSystem(Engine::Device &device, VkRenderPass renderPass,
                                                                std::array<VkDescriptorSetLayout, 2> setLayouts): device(device) {
        CreatePipelineLayout(setLayouts);
        CreatePipeline(renderPass);
    }

    void SceneRenderingSystem::Render(VkCommandBuffer commandBuffer) {
        pipeline->Bind(commandBuffer);

        // sceneContext.GetScene().GetRegistry().view<Engine::Transform, Engine::MeshCmp>()
        //     .each([&sceneContext, this, &commandBuffer](Engine::Transform& transform, Engine::MeshCmp& mesh) {

        //     if (mesh.mesh) {
        //         PushConstantData push{};
        //         push.modelMatrix = transform.TransformMatrix();
        //         push.normalMatrix = transform.NormalMatrix();

        //         vkCmdPushConstants(commandBuffer,
        //             pipelineLayout,
        //             VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
        //             0,
        //             sizeof(PushConstantData),
        //             &push);

        //         mesh.mesh->Bind(commandBuffer);
        //         // mesh.mesh->Draw(commandBuffer, sceneContext.GetSceneDescriptorSet(), pipelineLayout);
        //     }
        // });
    }

    void SceneRenderingSystem::CreatePipeline(VkRenderPass renderPass) {
        AQUILA_CORE_ASSERT(pipelineLayout != nullptr && "Cannot create pipeline before pipeline layout");

        Engine::PipelineConfigInfo pipelineConfig{};
        Engine::Pipeline::vk_DefaultPipelineConfig(pipelineConfig);
        pipelineConfig.renderPass = renderPass;
        pipelineConfig.pipelineLayout = pipelineLayout;
        pipeline = std::make_unique<Engine::Pipeline>(
                device,
                std::string(SHADERS_PATH) + "/shader_vert.spv",
                std::string(SHADERS_PATH) + "/shader_frag.spv",
                pipelineConfig);
    }

    void SceneRenderingSystem::RecreatePipeline(VkRenderPass renderPass) {
        if (pipeline) {
            pipeline.reset();
        }

        CreatePipeline(renderPass);
    }


    void SceneRenderingSystem::CreatePipelineLayout(std::array<VkDescriptorSetLayout, 2> setLayouts) {
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
