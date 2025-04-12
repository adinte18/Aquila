#include <utility>

#include "RenderingSystems/SceneRenderingSystem.h"

#include <Engine/Model.h>

#include "Components.h"

struct PushConstantData
{
    glm::mat4 modelMatrix{1.f}; //identity
    glm::mat4 normalMatrix{1.f};
};

RenderingSystem::SceneRenderingSystem::SceneRenderingSystem(Engine::Device &device, VkRenderPass renderPass,
                                                            std::array<VkDescriptorSetLayout, 2> setLayouts): device(device) {
    CreatePipelineLayout(setLayouts);
    CreatePipeline(renderPass);
}

void RenderingSystem::SceneRenderingSystem::Render(VkCommandBuffer commandBuffer, ECS::SceneContext& sceneContext) {
    pipeline->bind(commandBuffer);

    sceneContext.GetScene().GetRegistry().view<ECS::Transform, ECS::Mesh>()
        .each([&sceneContext, this, &commandBuffer](ECS::Transform& transform, ECS::Mesh& mesh) {

        if (mesh.mesh) {
            PushConstantData push{};
            push.modelMatrix = transform.TransformMatrix();
            push.normalMatrix = transform.NormalMatrix();

            vkCmdPushConstants(commandBuffer,
                pipelineLayout,
                VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
                0,
                sizeof(PushConstantData),
                &push);

            mesh.mesh->bind(commandBuffer);
            mesh.mesh->draw(commandBuffer, sceneContext.GetSceneDescriptorSet(), pipelineLayout);
        }
    });
}

void RenderingSystem::SceneRenderingSystem::CreatePipeline(VkRenderPass renderPass) {
    assert(pipelineLayout != nullptr && "Cannot create pipeline before pipeline layout");

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

void RenderingSystem::SceneRenderingSystem::RecreatePipeline(VkRenderPass renderPass) {
    if (pipeline) {
        pipeline.reset();
    }

    CreatePipeline(renderPass);
}


void RenderingSystem::SceneRenderingSystem::CreatePipelineLayout(std::array<VkDescriptorSetLayout, 2> setLayouts) {
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
