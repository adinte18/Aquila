#ifndef PREFILTERRENDERINGSYSTEM_H
#define PREFILTERRENDERINGSYSTEM_H

#include "Engine/Mesh.h"
#include <Engine/Renderer/Device.h>
#include <Engine/Renderer/Pipeline.h>



namespace Engine {
    class PrefilterRenderingSystem {
    public:
        PrefilterRenderingSystem(Device &device, VkRenderPass renderPass, VkDescriptorSetLayout layout);
        ~PrefilterRenderingSystem() { vkDestroyPipelineLayout(device.GetDevice(), pipelineLayout, nullptr); };

        PrefilterRenderingSystem(const PrefilterRenderingSystem&) = delete;
        PrefilterRenderingSystem& operator=(const PrefilterRenderingSystem&) = delete;

        void Render(VkCommandBuffer commandBuffer, VkDescriptorSet descriptorSet, glm::mat4& viewMatrix, uint32_t mipLevel);
        void RecreatePipeline(VkRenderPass renderPass);


    private:
        Device& device;

        Unique<Pipeline> pipeline;
        VkPipelineLayout pipelineLayout;
        Ref<Mesh> model;


        void CreatePipeline(VkRenderPass renderPass);
        void CreatePipelineLayout(VkDescriptorSetLayout layout);

    };
}




#endif //PREFILTERRENDERINGSYSTEM_H
