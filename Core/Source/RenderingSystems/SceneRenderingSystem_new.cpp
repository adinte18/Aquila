#include "RenderingSystems/SceneRenderingSystem_new.h"

#include "Engine/Controller.h"
#include "Engine/EditorCamera.h"
#include "Scene/Components/MeshComponent.h"
#include "Scene/Components/MetadataComponent.h"
#include "Scene/Scene.h"
#include "Scene/Components/TransformComponent.h"

namespace Engine {
    struct PushConstantData
    {
        glm::mat4 modelMatrix{1.f};
        glm::mat4 normalMatrix{1.f};
    };


    SceneRenderSystem::SceneRenderSystem(Device& device, VkRenderPass renderPass)
        : RenderingSystemBase(device)
    {
        CreateDescriptorSetLayout();
        AllocateDescriptorSet();

        // init buffer
        m_Buffer = CreateUnique<Buffer>(device, sizeof(SceneUniformData), 1,VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
        m_Buffer->map();

        auto uniformData = m_Buffer->vk_DescriptorInfo();

        SetUniformData(0, &uniformData);

        CreatePipelineLayout();
        CreatePipeline(renderPass);
    }

    void SceneRenderSystem::CreateDescriptorSetLayout() {
        m_Layout = DescriptorSetLayout::Builder(device)
            .addBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_ALL_GRAPHICS)
            .addBinding(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT)
            .build();
    }

    void SceneRenderSystem::CreatePipeline(VkRenderPass renderPass) {
        PipelineConfigInfo pipelineConfig{};
        Pipeline::vk_DefaultPipelineConfig(pipelineConfig);
        pipelineConfig.renderPass = renderPass;
        pipelineConfig.pipelineLayout = m_PipelineLayout;
        m_Pipeline = CreateUnique<Pipeline>(
            device,
            std::string(SHADERS_PATH) + "/AqPBRvert.spv",
            std::string(SHADERS_PATH) + "/AqPBRfrag.spv",
            pipelineConfig);
    }

    void SceneRenderSystem::CreatePipelineLayout(){
        VkPushConstantRange pushConstantRange{};
        pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
        pushConstantRange.offset = 0;
        pushConstantRange.size = sizeof(PushConstantData);

        auto setLayout = m_Layout->GetDescriptorSetLayout();

        VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
        pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pipelineLayoutInfo.setLayoutCount = 1;
        pipelineLayoutInfo.pSetLayouts = &setLayout;
        pipelineLayoutInfo.pushConstantRangeCount = 1;
        pipelineLayoutInfo.pPushConstantRanges = &pushConstantRange;
        if (vkCreatePipelineLayout(device.GetDevice(), &pipelineLayoutInfo, nullptr, &m_PipelineLayout) != VK_SUCCESS) {
            throw std::runtime_error("failed to create pipeline layout!");
        }
    }

    void SceneRenderSystem::UpdateBuffer(EditorCamera& camera){
        SceneUniformData data{};
        data.projection = camera.GetProjection();
        data.view = camera.GetView();
        data.inverseView = camera.GetInverseView();

        m_Buffer->vk_WriteToBuffer(&data);
        m_Buffer->vk_Flush();
    }

    void SceneRenderSystem::Render(const FrameSpec& frameSpec) {
        const auto& ctx = static_cast<const SceneRenderingContext&>(frameSpec);

        auto* scene = Engine::Controller::Get()->GetSceneManager().GetActiveScene();

        auto& registry = scene->GetRegistry();

        m_Pipeline->Bind(frameSpec.commandBuffer);

        // vkCmdBindDescriptorSets(frameSpec.commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
        //                         m_PipelineLayout, 0, 1, &m_DescriptorSets[ctx.frameIndex], 0, nullptr);

        auto view = registry.view<MetadataComponent, MeshComponent, TransformComponent>();

        for (auto entity : view) {
            auto [metaComp, meshComp, transformComp] = view.get<MetadataComponent, MeshComponent, TransformComponent>(entity);

                PushConstantData push{};
                push.modelMatrix = transformComp.GetWorldMatrix();
                push.normalMatrix = transformComp.GetNormalMatrix();

                vkCmdPushConstants(frameSpec.commandBuffer,
                    m_PipelineLayout,
                    VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
                    0,
                    sizeof(PushConstantData),
                    &push);

            if (meshComp.data != nullptr && metaComp.Enabled) {
                meshComp.data->Bind(frameSpec.commandBuffer);
                meshComp.data->Draw(frameSpec.commandBuffer, m_PipelineLayout, m_DescriptorSets[ctx.frameIndex]);
            }
        }
    }

}