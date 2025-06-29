#ifndef PREFILTERRENDERINGSYSTEM_H
#define PREFILTERRENDERINGSYSTEM_H

#include <Engine/Device.h>
#include <Engine/Pipeline.h>
#include <ECS/SceneContext.h>


namespace Engine {
    class PrefilterRenderingSystem {
    public:
        PrefilterRenderingSystem(Device &device, VkRenderPass renderPass, VkDescriptorSetLayout layout);
        ~PrefilterRenderingSystem() { vkDestroyPipelineLayout(device.vk_GetDevice(), pipelineLayout, nullptr); };

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
