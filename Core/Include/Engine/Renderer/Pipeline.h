//
// Created by adinte on 7/5/24.
//

#ifndef VK_APP_VK_PIPELINE_H
#define VK_APP_VK_PIPELINE_H

#include "Engine/Renderer/Device.h"
#include "Engine/Renderer/Shader.h"

namespace Engine{
    struct PipelineConfigInfo {
        PipelineConfigInfo() = default;
        PipelineConfigInfo(const PipelineConfigInfo&) = delete;
        PipelineConfigInfo& operator=(const PipelineConfigInfo&) = delete;


        std::vector<VkVertexInputBindingDescription> bindingDescriptions{};
        std::vector<VkVertexInputAttributeDescription> attributeDescriptions{};
        VkPipelineViewportStateCreateInfo viewportInfo;
        VkPipelineInputAssemblyStateCreateInfo inputAssemblyInfo;
        VkPipelineRasterizationStateCreateInfo rasterizationInfo;
        VkPipelineMultisampleStateCreateInfo multisampleInfo;
        VkPipelineColorBlendAttachmentState colorBlendAttachment;
        VkPipelineColorBlendStateCreateInfo colorBlendInfo;
        VkPipelineDepthStencilStateCreateInfo depthStencilInfo;
        std::vector<VkDynamicState> dynamicStateEnables;
        VkPipelineDynamicStateCreateInfo dynamicStateCreateInfo;
        VkPipelineLayout pipelineLayout = nullptr;
        VkRenderPass renderPass = nullptr;
        uint32_t subpass = 0;
    };


    class Pipeline {
    public:
        Pipeline(Device &device,
                    const std::string &vertFilepath,
                    const std::string &fragFilepath,
                    const PipelineConfigInfo &configInfo);
        ~Pipeline();
        [[nodiscard]] VkPipeline GetPipeline() const;

        Pipeline(const Pipeline&) = delete;
        Pipeline& operator=(const Pipeline&) = delete;

        void Bind(VkCommandBuffer commandBuffer) const;

        static void vk_DefaultPipelineConfig(PipelineConfigInfo& configInfo);

    private:
        void vk_CreateGraphicsPipeline(const std::string &vertFilepath,
                                    const std::string &fragFilepath,
                                    const PipelineConfigInfo &configInfo);

        Device &m_Device;
        VkPipeline m_GraphicsPipeline;
        VkShaderModule m_VertexShaderModule;
        VkShaderModule m_FragmentShaderModule;
    };
}


#endif //VK_APP_VK_PIPELINE_H
