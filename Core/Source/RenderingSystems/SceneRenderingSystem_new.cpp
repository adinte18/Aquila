#include "RenderingSystems/SceneRenderingSystem_new.h"

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


    SceneRenderingSystem_new::SceneRenderingSystem_new(Device& device, VkRenderPass renderPass)
        : RenderingSystemBase(device)
    {
        CreateDescriptorSetLayout();
        AllocateDescriptorSet();

        // init buffer
        m_Buffer = std::make_unique<Buffer>(device, sizeof(SceneUniformData), 1,VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
        m_Buffer->map();

        auto uniformData = m_Buffer->vk_DescriptorInfo();

        SetUniformData(0, &uniformData);

        CreatePipelineLayout();
        CreatePipeline(renderPass);
    }

    void SceneRenderingSystem_new::CreateDescriptorSetLayout() {
        m_Layout = DescriptorSetLayout::Builder(device)
            .addBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_ALL_GRAPHICS)
            .addBinding(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT)
            .build();
    }

    void SceneRenderingSystem_new::CreatePipeline(VkRenderPass renderPass) {
        PipelineConfigInfo pipelineConfig{};
        Pipeline::vk_DefaultPipelineConfig(pipelineConfig);
        pipelineConfig.renderPass = renderPass;
        pipelineConfig.pipelineLayout = m_PipelineLayout;
        m_Pipeline = std::make_unique<Pipeline>(
            device,
            std::string(SHADERS_PATH) + "/AqPBRvert.spv",
            std::string(SHADERS_PATH) + "/AqPBRfrag.spv",
            pipelineConfig);
    }

    void SceneRenderingSystem_new::CreatePipelineLayout(){
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
        if (vkCreatePipelineLayout(device.vk_GetDevice(), &pipelineLayoutInfo, nullptr, &m_PipelineLayout) != VK_SUCCESS) {
            throw std::runtime_error("failed to create pipeline layout!");
        }
    }

    void SceneRenderingSystem_new::UpdateBuffer(const RenderContext& context){
        SceneUniformData data{};
        data.projection = context.camera->GetProjection();
        data.view = context.camera->GetView();
        data.inverseView = context.camera->GetInverseView();

        m_Buffer->vk_WriteToBuffer(&data);
        m_Buffer->vk_Flush();
    }

    void SceneRenderingSystem_new::Render(const RenderContext& context) {
        auto& scene = context.scene;

        auto& registry = scene->GetRegistry();

        m_Pipeline->Bind(context.commandBuffer);

        UpdateBuffer(context);

        SendDataToGPU();

        vkCmdBindDescriptorSets(context.commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                                m_PipelineLayout, 0, 1, &m_DescriptorSet, 0, nullptr);

        auto view = registry.view<MetadataComponent, MeshComponent, TransformComponent>();

        for (auto entity : view) {
            auto [metaComp, meshComp, transformComp] = view.get<MetadataComponent, MeshComponent, TransformComponent>(entity);

                PushConstantData push{};
                push.modelMatrix = transformComp.GetWorldMatrix();
                push.normalMatrix = transformComp.GetNormalMatrix();

                vkCmdPushConstants(context.commandBuffer,
                    m_PipelineLayout,
                    VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
                    0,
                    sizeof(PushConstantData),
                    &push);

            if (meshComp.data != nullptr && metaComp.Enabled) {
                meshComp.data->Bind(context.commandBuffer);
                meshComp.data->Draw(context.commandBuffer, m_PipelineLayout, m_DescriptorSet);
            }
        }
    }

}