//
// Created by alexa on 10/04/2025.
//

#ifndef IRRADIANCERENDERINGSYSTEM_H
#define IRRADIANCERENDERINGSYSTEM_H



#include <Engine/Device.h>
#include <Engine/Pipeline.h>
#include <ECS/SceneContext.h>


namespace Engine {
    class IrradianceRenderingSystem {
    public:
        IrradianceRenderingSystem(Device &device, VkRenderPass renderPass, VkDescriptorSetLayout layout);
        ~IrradianceRenderingSystem() { vkDestroyPipelineLayout(device.vk_GetDevice(), pipelineLayout, nullptr); };

        IrradianceRenderingSystem(const IrradianceRenderingSystem&) = delete;
        IrradianceRenderingSystem& operator=(const IrradianceRenderingSystem&) = delete;

        void Render(VkCommandBuffer commandBuffer, VkDescriptorSet descriptorSet, glm::mat4& viewMatrix);
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



#endif //IRRADIANCERENDERINGSYSTEM_H
